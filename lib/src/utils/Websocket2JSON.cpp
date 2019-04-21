//
// Created by Subhagato Dutta on 12/17/17.
//

#include "Websocket2JSON.h"

using namespace std;

void Websocket2JSON::init_client() {
  // COUT<<"On init_client tid: 0x"<<hex<<this_thread::get_id()<<endl;

  // m_endpoint = new client();
  // m_endpoint->set_access_channels(websocketpp::log::alevel::all); //all
  // m_endpoint->clear_access_channels(websocketpp::log::alevel::frame_payload); //frame_payload
  // m_endpoint->set_error_channels(websocketpp::log::elevel::all); //all
  m_endpoint.clear_access_channels(websocketpp::log::alevel::all);

  // Initialize ASIO
  m_endpoint.init_asio();

  // Register our handlers
  m_endpoint.set_socket_init_handler(bind(&type::on_socket_init, this, ::_1));
  m_endpoint.set_tls_init_handler(bind(&type::on_tls_init, this, ::_1));
  m_endpoint.set_message_handler(bind(&type::on_message, this, ::_1, ::_2));
  m_endpoint.set_open_handler(bind(&type::on_open, this, ::_1));
  m_endpoint.set_interrupt_handler(bind(&type::on_interrupt, this, ::_1));
  m_endpoint.set_close_handler(bind(&type::on_close, this, ::_1));
  m_endpoint.set_fail_handler(bind(&type::on_fail, this, ::_1));
  m_endpoint.set_pong_handler(bind(&type::on_pong, this, ::_1, ::_2));
  m_endpoint.set_pong_timeout_handler(bind(&type::on_pong_timeout, this, ::_1, ::_2));
  m_endpoint.set_pong_timeout(2000);
  m_connected = false;
}

Websocket2JSON::Websocket2JSON(bool async_receive, bool ping, chrono::seconds ping_interval)
    : m_ping(ping), m_async_receive(async_receive), m_ping_interval(ping_interval) {
  init_client();
  m_auto_reconnect = true;
  thread reconnect_thread = websocketpp::lib::thread(&Websocket2JSON::autoReconnectLoop, this);
  reconnect_thread.detach();
}

bool Websocket2JSON::connect(string server_uri, int port, http_header_t* request_headers) {
  // COUT<<"On connect tid: 0x"<<hex<<this_thread::get_id()<<endl;

  if (m_connected) return false;

  websocketpp::lib::error_code ec;

  string uri_port;

  if (server_uri.find(":") == string::npos)
    uri_port = server_uri + ":" + to_string(port);
  else
    uri_port = server_uri;

  m_uri_port = uri_port;

  m_conn = m_endpoint.get_connection(m_uri_port, ec);

  if (ec) {
    m_endpoint.get_alog().write(websocketpp::log::alevel::app, ec.message());
    return false;
  }

  // con->set_proxy("http://localhost:8443");

  m_endpoint.connect(m_conn);

  thread run_thread = websocketpp::lib::thread(&client::run, &m_endpoint);

  run_thread.detach();

  unique_lock<mutex> lck(m_connection_mtx);

  if (m_wait_on_connect.wait_for(lck, chrono::seconds(5)) == cv_status::timeout) {
    throw runtime_error("[Error: Websocket2JSON::connect] Exception: Connection timeout..");
  }

  return true;
}

bool Websocket2JSON::send(json& params) {
  m_endpoint.send(m_hdl, params.dump(), websocketpp::frame::opcode::text);
  return true;
}

int Websocket2JSON::receive(json& message) {
  if (!m_messages.empty()) {
    message = m_messages.front();
    m_messages.pop();
    return m_messages.size();  // return number of messages left
  } else {
    return -1;
  }
}

bool Websocket2JSON::close() {
  if (m_connected) {
    unique_lock<mutex> lck(m_connection_mtx);

    m_connected = false;

    m_endpoint.close(m_hdl, websocketpp::close::status::going_away, "");

    if (m_wait_on_disconnect.wait_for(lck, chrono::seconds(5)) == cv_status::timeout) {
      throw runtime_error("[Error: Websocket2JSON::disconnect] Exception: Disconnect timeout..");
    }

    lck.unlock();

    return true;
  }

  return false;
}

void Websocket2JSON::bindCallback(function<void(json&)> callback_func) {
  m_callback_func = callback_func;
}

bool Websocket2JSON::reconnect() {
  int retry_timeout = 30;  // retry timeout is 1 minute (30 sec  + 30 sec)

  COUT << "On reconnect tid: 0x" << hex << this_thread::get_id() << endl;

  websocketpp::lib::error_code ec;

  this_thread::sleep_for(chrono::seconds(retry_timeout));

  COUT << "here1\n";

  m_endpoint.reset();

  COUT << "here2\n";

  m_conn = m_endpoint.get_connection(m_uri_port, ec);

  COUT << "here3\n";

  if (ec) {
    m_endpoint.get_alog().write(websocketpp::log::alevel::app, ec.message());
    COUT << "Websocket Error: Unable to get a connection\n";
    return false;
  }

  COUT << "here4\n";

  this_thread::sleep_for(chrono::seconds(retry_timeout));

  COUT << "here5\n";

  while (true) {
    m_connected = false;

    m_endpoint.connect(m_conn);

    thread run_thread = websocketpp::lib::thread(&client::run, &m_endpoint);

    run_thread.detach();

    unique_lock<mutex> lck(m_connection_mtx);

    if (m_wait_on_connect.wait_for(lck, chrono::seconds(retry_timeout)) == cv_status::timeout) {
      retry_timeout *= 2;
      COUT << "Websocket Error: Unable to reconnect to Websocket. Retrying after " << retry_timeout << " seconds\n";
      continue;
    }

    if (!m_connected) {
      COUT << "Websocket Error: Connection reset by peer. Retrying after " << retry_timeout << " seconds\n";
      this_thread::sleep_for(chrono::seconds(retry_timeout));
      retry_timeout *= 2;
      m_endpoint.reset();
      continue;
    }

    this_thread::sleep_for(chrono::seconds(5));

    break;
  }

  COUT << "here6\n";

  if (m_preamble_func) sendPreamble();  // this restores streaming all topics

  return true;
}

void Websocket2JSON::autoReconnectLoop() {
  unique_lock<mutex> lck(m_connection_mtx);

  while (true) {
    m_wait_on_reconnect.wait(lck);
    if (!m_auto_reconnect) break;

    COUT << "Wait on reconnect triggered...\n";
    lck.unlock();
    reconnect();
  }

  // COUT<<"Websocket2JSON("<<this<<")::autoReconnectLoop exited!\n";
}

void Websocket2JSON::attachPreambleGeneratorFunction(function<json(void)> preamble_func) {
  m_preamble_func = preamble_func;
}

void Websocket2JSON::sendPreamble() {
  json message;

  if (m_preamble_func) {
    message = m_preamble_func();
    send(message);
  } else {
    CT_CRIT_WARN << "No preamble specified.\n";
  }
}
