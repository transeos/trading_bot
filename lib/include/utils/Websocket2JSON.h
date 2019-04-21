//
// Created by Subhagato Dutta on 12/17/17.
//

#ifndef CRYPTOTRADER_WEBSOCKET2JSON_H
#define CRYPTOTRADER_WEBSOCKET2JSON_H

#define ASIO_STANDALONE

#include "Globals.h"
#include "utils/Logger.h"
#include "utils/TimeUtils.h"
#include <asio.hpp>
#include <chrono>
#include <chrono>
#include <functional>
#include <iostream>
#include <nlohmann/json.hpp>
#include <queue>
#include <sys/types.h>
#include <thread>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

using json = nlohmann::json;

typedef std::map<std::string, std::string> http_header_t;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

class Websocket2JSON {
 private:
  std::queue<json> m_messages;
  std::string m_message_to_send;
  std::string m_server_uri;
  client m_endpoint;
  websocketpp::connection_hdl m_hdl;
  bool m_ping;
  bool m_async_receive;
  bool m_connected;
  std::chrono::seconds m_ping_interval;
  websocketpp::lib::thread m_run_thread;
  websocketpp::lib::shared_ptr<asio::steady_timer> m_timer_ptr;
  client::connection_ptr m_conn;
  std::string m_uri_port;
  std::condition_variable m_wait_on_connect;
  std::condition_variable m_wait_on_disconnect;
  std::condition_variable m_wait_on_reconnect;
  std::mutex m_connection_mtx;
  bool m_auto_reconnect;
  std::function<void(json&)> m_callback_func;
  std::function<json(void)> m_preamble_func;

  void init_client();

  bool reconnect();

  void autoReconnectLoop();

  void on_socket_init(websocketpp::connection_hdl) {
    // COUT<<"On socket_init tid: 0x"<<std::hex<<std::this_thread::get_id()<<std::endl;
  }

  context_ptr on_tls_init(websocketpp::connection_hdl) {
    // COUT<<"On tls_init tid: 0x"<<std::hex<<std::this_thread::get_id()<<std::endl;

    context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv1);

    try {
      ctx->set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
                       asio::ssl::context::no_sslv3 | asio::ssl::context::single_dh_use);
    } catch (std::exception& e) {
      CT_CRIT_WARN << e.what() << std::endl;
    }
    return ctx;
  }

  void on_fail(websocketpp::connection_hdl hdl) {
    // COUT<<"On fail tid: 0x"<<std::hex<<std::this_thread::get_id()<<std::endl;

    client::connection_ptr con = m_endpoint.get_con_from_hdl(hdl);

    COUT << "Fail handler" << std::endl;

    m_connected = false;

    CT_CRIT_WARN << con->get_state() << std::endl;
    CT_CRIT_WARN << con->get_local_close_code() << std::endl;
    CT_CRIT_WARN << con->get_local_close_reason() << std::endl;
    CT_CRIT_WARN << con->get_remote_close_code() << std::endl;
    CT_CRIT_WARN << con->get_remote_close_reason() << std::endl;
    CT_CRIT_WARN << con->get_ec() << " - " << con->get_ec().message() << std::endl;

    std::unique_lock<std::mutex> lck(m_connection_mtx);
    m_wait_on_reconnect.notify_all();
  }

  void on_open(websocketpp::connection_hdl hdl) {
    // COUT<<"On open tid: 0x"<<std::hex<<std::this_thread::get_id()<<std::endl;

    m_hdl = hdl;

    std::unique_lock<std::mutex> lck(m_connection_mtx);

    if (m_connected == false) {
      COUT << "Websocket connected successfully.\n";
      m_connected = true;
      m_wait_on_connect.notify_all();
    } else {
      m_wait_on_reconnect.notify_all();
    }

    lck.unlock();

    if (m_ping) m_timer_ptr = m_endpoint.set_timer(m_ping_interval.count() * 1000, bind(&type::on_timer, this, ::_1));
  }

  void on_interrupt(websocketpp::connection_hdl hdl) {
    m_hdl = hdl;
    COUT << "on interrupt...\n";
  }

  void on_timer(websocketpp::lib::error_code const& ec) {
    if (!m_connected) return;

    if (ec) {
      // there was an error, stop connection
      m_endpoint.get_alog().write(websocketpp::log::alevel::app, "Timer Error: " + ec.message());
      return;
    }

    if (m_connected) {
      if (g_dump_trades_websocket)
        COUT << "on timer sending ping...\n";
      else
        COUT << CGREEN << " * ";

      m_endpoint.ping(m_hdl, "keepalive");

      // m_endpoint->close(m_hdl,websocketpp::close::status::going_away,"");

      m_timer_ptr->expires_from_now(m_ping_interval);
      m_timer_ptr->async_wait(bind(&type::on_timer, this, ::_1));
    }
  }

  void on_pong(websocketpp::connection_hdl hdl, std::string message) {
    // COUT<<message<<": pong received...\n";
  }

  void on_pong_timeout(websocketpp::connection_hdl hdl, std::string message) {
    CT_CRIT_WARN << message << ": pong timeout...\n";
  }

  void on_message(websocketpp::connection_hdl hdl, message_ptr message) {
    json json_message = json::parse(message->get_payload());

    if (!m_async_receive)
      m_messages.push(json_message);
    else {
      m_callback_func(json_message);
    }
  }

  void on_close(websocketpp::connection_hdl) {
    if (m_ping) {
      m_timer_ptr->cancel();
    }

    // m_endpoint->reset();
    // COUT<<"On close tid: 0x"<<std::hex<<std::this_thread::get_id()<<std::endl;

    std::unique_lock<std::mutex> lck(m_connection_mtx);

    if (m_connected)  // reconnect
    {
      CT_CRIT_WARN << "Connection disconnected by server! Reconnecting...\n";
      m_wait_on_reconnect.notify_all();
    } else {
      COUT << CYELLOW << "Socket closed at " << Time::sNow() << std::endl;
      m_wait_on_disconnect.notify_all();
    }
  }

 public:
  typedef Websocket2JSON type;
  typedef std::chrono::duration<int, std::micro> dur_type;

  Websocket2JSON(bool receive_async = true, bool ping = true,
                 std::chrono::seconds ping_interval = std::chrono::seconds(30));

  bool connect(std::string server_uri, int port = 443, http_header_t* request_headers = nullptr);

  void attachPreambleGeneratorFunction(std::function<json(void)> preamble_func);

  void sendPreamble();

  bool send(json& params);

  int receive(json& message);

  bool close();

  void bindCallback(std::function<void(json&)> callback_func);

  ~Websocket2JSON() {
    m_auto_reconnect = false;
    m_wait_on_reconnect.notify_all();
    close();
    m_endpoint.reset();
    // DELETE(m_endpoint);
  }
};

#endif  // CRYPTOTRADER_WEBSOCKET2JSON_H
