//
// Created by Subhagato Dutta on 9/10/17.
//

#include "RestAPI2JSON.h"
#include "Globals.h"
#include "utils/EncodeDecode.h"
#include <iostream>
#include <thread>

using namespace nlohmann;
using namespace std;

void append_header(http_header_t* headers, const string& str) {
  string header_name;
  string header_value;

  size_t pos = str.find(':');

  if (pos != string::npos) {
    (*headers)[str.substr(0, pos)] = str.substr(pos + 1);
  }
}

size_t writeCallbackBody(void* contents, size_t size, size_t nmemb, void* userp) {
  ((string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

size_t writeCallbackHeader(void* contents, size_t size, size_t nmemb, void* userp) {
  string header((char*)contents, size * nmemb - 2);  // removed \r\n

  append_header((http_header_t*)userp, header);

  return size * nmemb;
}

RestAPI2JSON::RestAPI2JSON(string server_uri, int get_req_per_sec, int post_req_per_sec, string cert_path,
                           bool redirect, int timeout, string username, string password)
    : m_server_uri(server_uri), m_redirect(redirect), m_timeout(timeout), m_username(username), m_password(password) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_curl_handle = NULL;
  mp_curl_headers = NULL;

  resetCurl();

  m_time_between_get_requests = Duration(1000000LL / get_req_per_sec);
  m_time_between_post_requests = Duration(1000000LL / post_req_per_sec);
}

void RestAPI2JSON::resetCurl() {
  if (m_curl_handle) curl_easy_cleanup(m_curl_handle);

  m_curl_handle = curl_easy_init();

  curl_easy_setopt(m_curl_handle, CURLOPT_FOLLOWLOCATION, m_redirect);
  curl_easy_setopt(m_curl_handle, CURLOPT_TIMEOUT, m_timeout);

  curl_easy_setopt(m_curl_handle, CURLOPT_USERAGENT, "curl/7.37.0");

  if (!m_username.empty()) curl_easy_setopt(m_curl_handle, CURLOPT_USERNAME, m_username.c_str());
  if (!m_password.empty()) curl_easy_setopt(m_curl_handle, CURLOPT_PASSWORD, m_password.c_str());

  DELETE(mp_curl_headers);

  m_time_at_last_get_request = Time(0);
  m_time_at_last_post_request = Time(0);
}

json& RestAPI2JSON::getJSON_GET(string query, http_header_t* request_headers, http_header_t* response_headers) {
  std::lock_guard<std::mutex> lock(m_mutex);
  string result;
  string header;
  string header_str;

  Duration time_from_last_request = Time::sNow() - m_time_at_last_get_request;

  if (time_from_last_request < m_time_between_get_requests) {
    Duration(m_time_between_get_requests - time_from_last_request).wait();
  }

  if (m_curl_handle) {
    if (request_headers != nullptr) {
      for (auto& header : *request_headers) {
        header_str = header.first + " " + header.second;
        // COUT<<"header_str = "<<header_str<<endl;
        mp_curl_headers = curl_slist_append(mp_curl_headers, header_str.c_str());
      }

      curl_easy_setopt(m_curl_handle, CURLOPT_HTTPHEADER, mp_curl_headers);
    } else {
      curl_easy_setopt(m_curl_handle, CURLOPT_HTTPHEADER, NULL);
    }

    string url = m_server_uri + query;
    curl_easy_setopt(m_curl_handle, CURLOPT_VERBOSE, 0L);  // debug option
    curl_easy_setopt(m_curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, writeCallbackBody);

    if (response_headers != nullptr) {
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERFUNCTION, writeCallbackHeader);
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERDATA, response_headers);
    } else {
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERFUNCTION, nullptr);
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERDATA, nullptr);
    }

    curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(m_curl_handle, CURLOPT_ACCEPT_ENCODING, "");  // enable all supported built-in compressions

    m_curl_result = curl_easy_perform(m_curl_handle);

    m_time_at_last_get_request = Time::sNow();

    if (request_headers != nullptr) {
      curl_slist_free_all(mp_curl_headers);
      mp_curl_headers = NULL;
    }

    // libcurl internal error handling
    if (m_curl_result != CURLE_OK) {
      // this_thread::sleep_for(chrono::seconds(1));
      resetCurl();
      throw runtime_error("[Error: getJSON_GET] Exception: Libcurl error in curl_easy_perform(), code: " +
                          to_string(m_curl_result));
    }

    if (g_dump_rest_api_responses) {
      COUT << "Response : " << result << "\n";
    }

    try {
      j_response = json::parse(result);
    } catch (detail::parse_error& err)  // NOLINT
    {
      COUT << "[Error: getJSON_GET] Exception: " << err.what() << endl;
      throw runtime_error("[Error: getJSON_GET] Exception: " + string(err.what()));
    }

    return j_response;

  } else {
    throw runtime_error("[Error: getJSON_POST] Exception: CURL not properly initialized. \'m_curl_handle = NULL\'");
  }
}

json& RestAPI2JSON::getJSON_POST(string query, string data, http_header_t* request_headers,
                                 http_header_t* response_headers) {
  std::lock_guard<std::mutex> lock(m_mutex);
  string result;
  string header_request;

  Duration time_from_last_request = Time::sNow() - m_time_at_last_get_request;

  if (time_from_last_request < m_time_between_post_requests) {
    Duration(m_time_between_post_requests - time_from_last_request).wait();
  }

  if (m_curl_handle) {
    string url = m_server_uri + query;

    // Headers
    if (request_headers != nullptr) {
      struct curl_slist* mp_curlHeaders = NULL;
      for (auto header : *request_headers) {
        header_request = header.first + " " + header.second;
        // COUT<<header_request << endl;
        mp_curlHeaders = curl_slist_append(mp_curlHeaders, header_request.c_str());
      }

      curl_easy_setopt(m_curl_handle, CURLOPT_HTTPHEADER, mp_curlHeaders);
    }

    curl_easy_setopt(m_curl_handle, CURLOPT_VERBOSE, 0L);  // debug option
    curl_easy_setopt(m_curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl_handle, CURLOPT_POST, 1);
    curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDSIZE, data.length());
    curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, writeCallbackBody);

    if (response_headers != nullptr) {
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERFUNCTION, writeCallbackHeader);
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERDATA, response_headers);
    } else {
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERFUNCTION, nullptr);
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERDATA, nullptr);
    }

    curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, &result);
    m_curl_result = curl_easy_perform(m_curl_handle);
    m_time_at_last_post_request = Time::sNow();

    if (request_headers != nullptr) {
      curl_slist_free_all(mp_curl_headers);
      mp_curl_headers = NULL;
    }

    curl_easy_setopt(m_curl_handle, CURLOPT_POST, 0);

    // libcurl internal error handling
    if (m_curl_result != CURLE_OK) {
      // this_thread::sleep_for(chrono::seconds(1));
      resetCurl();
      throw runtime_error("[Error: getJSON_POST] Libcurl error in curl_easy_perform(), code: " +
                          to_string(m_curl_result));
    }

    try {
      j_response = json::parse(result);
    } catch (detail::parse_error& err)  // NOLINT
    {
      throw runtime_error("[Error: getJSON_POST] " + string(err.what()));
    }

    return j_response;

  } else {
    throw runtime_error("[Error: getJSON_POST] CURL not properly initialized. \'m_curl_handle = NULL\'");
  }
}

json& RestAPI2JSON::getJSON_DELETE(string query, string data, http_header_t* request_headers,
                                   http_header_t* response_headers) {
  std::lock_guard<std::mutex> lock(m_mutex);
  string result;
  string header_request;

  Duration time_from_last_request = Time::sNow() - m_time_at_last_get_request;

  if (time_from_last_request < m_time_between_post_requests) {
    Duration(m_time_between_post_requests - time_from_last_request).wait();
  }

  if (m_curl_handle) {
    string url = m_server_uri + query;

    // Headers
    if (request_headers != nullptr) {
      struct curl_slist* mp_curlHeaders = NULL;
      for (auto header : *request_headers) {
        header_request = header.first + " " + header.second;
        // COUT<<"header_request = "<<header_request<< endl;
        mp_curlHeaders = curl_slist_append(mp_curlHeaders, header_request.c_str());
      }

      curl_easy_setopt(m_curl_handle, CURLOPT_HTTPHEADER, mp_curlHeaders);
    }

    curl_easy_setopt(m_curl_handle, CURLOPT_VERBOSE, 0L);  // debug option
    curl_easy_setopt(m_curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDSIZE, data.length());
    curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, writeCallbackBody);

    if (response_headers != nullptr) {
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERFUNCTION, writeCallbackHeader);
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERDATA, response_headers);
    } else {
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERFUNCTION, nullptr);
      curl_easy_setopt(m_curl_handle, CURLOPT_HEADERDATA, nullptr);
    }

    curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, &result);
    m_curl_result = curl_easy_perform(m_curl_handle);
    m_time_at_last_post_request = Time::sNow();

    if (request_headers != nullptr) {
      curl_slist_free_all(mp_curl_headers);
      mp_curl_headers = NULL;
    }

    curl_easy_setopt(m_curl_handle, CURLOPT_CUSTOMREQUEST, nullptr);

    // libcurl internal error handling
    if (m_curl_result != CURLE_OK) {
      // this_thread::sleep_for(chrono::seconds(1));
      resetCurl();
      throw runtime_error("[Error: getJSON_DELETE] Libcurl error in curl_easy_perform(), code: " +
                          to_string(m_curl_result));
    }

    try {
      j_response = json::parse(result);
    } catch (detail::parse_error& err)  // NOLINT
    {
      // this_thread::sleep_for(chrono::seconds(1));
      resetCurl();
      throw runtime_error("[Error: getJSON_DELETE] " + string(err.what()));
    }

    // TODO: reset curl for now in delete requests
    resetCurl();
    return j_response;

  } else {
    // this_thread::sleep_for(chrono::seconds(1));
    resetCurl();
    throw runtime_error("[Error: getJSON_DELETE] CURL not properly initialized. \'m_curl_handle = NULL\'");
  }
}
