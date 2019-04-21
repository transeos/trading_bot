//
// Created by Subhagato Dutta on 9/10/17.
//

#ifndef CRYPTOTRADER_RESTAPI2JSON_H
#define CRYPTOTRADER_RESTAPI2JSON_H

#include "Globals.h"
#include "utils/TimeUtils.h"
#include <curl/curl.h>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

typedef std::map<std::string, std::string> http_header_t;

class RestAPI2JSON {
  json j_response;
  std::string m_server_uri;
  CURL* m_curl_handle;
  CURLcode m_curl_result;
  struct curl_slist* mp_curl_headers;
  Duration m_time_between_get_requests;
  Duration m_time_between_post_requests;
  Time m_time_at_last_get_request;
  Time m_time_at_last_post_request;

  bool m_redirect;
  int m_timeout;
  std::string m_username;
  std::string m_password;

  std::mutex m_mutex;

 public:
  RestAPI2JSON(std::string server_uri, int get_req_per_sec = INT32_MAX, int post_req_per_sec = INT32_MAX,
               std::string cert_path = "", bool redirect = true, int timeout = 5, std::string username = "",
               std::string password = "");

  json& getJSON_GET(std::string query, http_header_t* request_headers = nullptr,
                    http_header_t* response_headers = nullptr);
  json& getJSON_POST(std::string query, std::string data, http_header_t* request_headers = nullptr,
                     http_header_t* response_headers = nullptr);
  json& getJSON_DELETE(std::string query, std::string data, http_header_t* request_headers = nullptr,
                       http_header_t* response_headers = nullptr);

  ~RestAPI2JSON() {
    curl_easy_cleanup(m_curl_handle);
    DELETE(mp_curl_headers);
  }

  void resetCurl();
};

#endif  // CRYPTOTRADER_RESTAPI2JSON_H
