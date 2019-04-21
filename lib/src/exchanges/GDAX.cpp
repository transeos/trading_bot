//
// Created by Subhagato Dutta on 9/14/17.
//

#include "exchanges/GDAX.h"
#include "Candlestick.h"
#include "Database.h"
#include "Quote.h"
#include "Tick.h"
#include "TraderBot.h"
#include "utils/EncodeDecode.h"
#include "utils/JsonUtils.h"
#include "utils/RestAPI2JSON.h"
#include "utils/TraderUtils.h"
#include <thread>

// Enable it to dump ticks in a CSV file which has not been written to database yet.
#define DUMP_LATEST_TICKS_IN_CSV 0

using namespace std;
using namespace nlohmann;
using namespace placeholders;

static set<string> message_types = {"ticker", "heartbeat", "snapshot", "l2update", "matches", "received",
                                    "open",   "done",      "match",    "change",   "activate"};

GDAX::GDAX(const exchange_t a_id, const json& config) : Exchange(a_id, config) {
  m_topics = {"heartbeat", "level2_50", "ticker", "matches", "user"};

  if (m_apikey.getKey() == "") {
    m_public_only = true;
  } else {
    m_public_only = false;
  }

  init();

  if (!m_public_only) {
    calculateAuthHeaders("/accounts/null");

    json json_response;

    try {
      json_response = m_query_handle->getJSON_GET("/accounts/null", &m_auth_headers);

      if (json_response["message"].get<string>() == "BadRequest") {
        m_public_only = false;
        COUT << CGREEN << "Activated authenticated client. :)" << endl;
      } else {
        m_public_only = true;
        CT_CRIT_WARN << "Invalid API credentials! Public client only :(\n";
      }
    } catch (detail::type_error& err) {
      CT_CRIT_WARN << "Exception: " << err.what() << endl << "\t\tinvalid json response for GDAX api key\n";
    } catch (runtime_error& err) {
      CT_CRIT_WARN << "Exception: " << err.what() << endl << "Json Response Error: unable to connect to GDAX.\n";
    }
  }
}

GDAX::~GDAX() {
  // it cannot be moved to Exchange destructor
  disconnectWebsocket();
}

bool GDAX::init() {
  m_mode = exchange_mode_t::SIMULATION;  // default mode is set to simulation

  m_stream_live_data = false;  // by default live data streaming is disabled

  // Supported currency pairs in the exchange

  m_query_handle = new RestAPI2JSON(m_rest_api_endpoint, 4);

  m_websocket_handle = new Websocket2JSON(true);

  m_websocket_handle->bindCallback(bind(&GDAX::websocketCallback, this, _1));

  // Supported currencies in the exchange

  json json_response;
  // take the first element
  CONTINUOUS_TRY(json_response = m_query_handle->getJSON_GET("/currencies"), 0);

  for (auto j : json_response)  // access by value, the type of i is int
    m_currencies.push_back(Currency(j["id"].get<string>(), stod(j["min_size"].get<string>())));

  // take the first element
  CONTINUOUS_TRY(json_response = m_query_handle->getJSON_GET("/products"), 0);

  for (auto j : json_response)  // access by value, the type of i is int
  {
    CurrencyPair cp(j["id"].get<string>());

    m_markets.insert(
        make_pair(cp, new Market(cp, stod(j["base_min_size"].get<string>()), stod(j["base_max_size"].get<string>()),
                                 stod(j["quote_increment"].get<string>()))));
  }

  initMarkets();

  return true;
}

double GDAX::getTicker(const CurrencyPair& currency_pair) const {
  double ticker_val = 0;

  if (!m_websocket_connected || m_markets.at(currency_pair)->getTickerPrice() == 0) {
    try {
      json json_response =
          m_query_handle->getJSON_GET("/products/" + currency_pair.toString() + "/ticker");  // take the first element
      ticker_val = getJsonValueT<string, double>(json_response, "price");
    } catch (detail::type_error& err) {  // NOLINT
      CT_CRIT_WARN << "Exception: " << err.what() << endl;
      return -1.0;
    }
  } else {
    ticker_val = m_markets.at(currency_pair)->getTickerPrice();
  }

  return ticker_val;
}

Quote GDAX::getRestAPIQuote(const CurrencyPair& currency_pair) {
  try {
    // take the first element
    json json_response = m_query_handle->getJSON_GET("/products/" + currency_pair.toString() + "/book");
    json j_bids = json_response["bids"];
    json j_asks = json_response["asks"];

    double bid_price = stod(j_bids[0][0].get<string>());
    double bid_size = stod(j_bids[0][1].get<string>());
    double ask_price = stod(j_asks[0][0].get<string>());
    double ask_size = stod(j_asks[0][1].get<string>());

    return Quote(bid_price, bid_size, ask_price, ask_size);
  } catch (detail::type_error err) {  // NOLINT
    CT_CRIT_WARN << "Exception: " << err.what() << endl;
  }

  return Quote();
}

Quote GDAX::getQuote(const CurrencyPair& currency_pair) {
  if (m_websocket_connected) return m_markets[currency_pair]->getQuote();

  return getRestAPIQuote(currency_pair);
}

bool GDAX::cancelOrder(order_id_t order_id, bool order_from_current_run) {
  bool ret_val = false;

  if (m_public_only) {
    NO_COINBASE_API_ERROR;
  }

  string cancel_order_str = "/orders/" + order_id;

  calculateAuthHeaders(cancel_order_str, json(), rest_request_t::DELETE);

  json json_response;
  CONTINUOUS_TRY(json_response = m_query_handle->getJSON_DELETE(cancel_order_str, "", &m_auth_headers), 0);

  if (g_dump_order_responses) COUT << json_response.dump(2) << endl;

  if (json_response.find("message") == json_response.end()) {
    if (json_response.size() > 0) {
      if (order_from_current_run) clearOrder(order_id);

      ret_val = true;
    } else {
      CT_CRIT_WARN << "Unable to cancel order: " << order_id << endl;
    }
  } else {
    string response = json_response["message"].get<string>();
    if (response == "Order already done") {
      if (order_from_current_run) clearOrder(order_id);

      ret_val = true;
    } else {
      CT_CRIT_WARN << "unexpected GDAX cancel order response : " << response << endl;
    }
  }

  if (!ret_val) {
    CT_CRIT_WARN << "Unable to cancel order: " << order_id << endl;
  }

  this_thread::sleep_for(chrono::milliseconds(10));
  return ret_val;
}

bool GDAX::cancelAllOrders() {
  if (m_public_only) {
    NO_COINBASE_API_ERROR;
  }

  CT_CRIT_WARN << "Cancelling All orders\n";

  calculateAuthHeaders("/orders", json(), rest_request_t::DELETE);

  json json_response;
  CONTINUOUS_TRY(json_response = m_query_handle->getJSON_DELETE("/orders", "", &m_auth_headers), 0);

  if (g_dump_order_responses) COUT << json_response.dump(2) << endl;

  if (json_response.find("message") == json_response.end()) {
    if (json_response.size() > 0) {
      m_pending_orders.clear();
      return true;
    } else {
      CT_CRIT_WARN << "Unable to cancel all orders\n";
      return false;
    }
  }

  CT_CRIT_WARN << "Unable to cancel all orders\n";
  return false;
}

order_id_t GDAX::order(Order& order) {
  order_id_t order_id = "";

  string order_type = "invalid";
  double amount = order.getAmount();

  switch (order.getOrderType()) {
    case order_type_t::LIMIT:
      order_type = "limit";
      break;
    case order_type_t::MARKET:
      order_type = "market";
      break;
    case order_type_t::STOP:
      assert(0);  // not supported yet
      break;
  }

  COUT << "ticker = " << getTicker(order.getCurrencyPair()) << endl;
  COUT << "amount = " << amount << endl;

  json j_order_params = {{"type", order_type},
                         {"side", (order.getOrderDir() == order_direction_t::BUY) ? "buy" : "sell"},
                         {"product_id", order.getCurrencyPair().toString()}};

  if (order_type == "limit") {
    j_order_params["price"] = order.getTriggerPrice();
    j_order_params["post_only"] = true;
    if (order.getOrderDir() == order_direction_t::BUY) {
      amount = floor((order.getAmount() * 1e8) / order.getTriggerPrice()) / 1e8;
    }
    j_order_params["size"] = amount;
  } else if (order_type == "market") {
    if (order.getOrderDir() == order_direction_t::BUY) {
      j_order_params["funds"] = amount;
    } else {
      j_order_params["size"] = amount;
    }
  }

  if (g_dump_order_responses) COUT << "Order params: " << j_order_params.dump(4) << endl;

  if (m_public_only) {
    NO_COINBASE_API_ERROR;
  }

  calculateAuthHeaders("/orders", j_order_params, rest_request_t::POST);

  if (order.getOrderType() == order_type_t::MARKET) {
    unique_lock<mutex> mutex_lock(m_order_mutex);

    json json_response;
    CONTINUOUS_TRY(json_response = m_query_handle->getJSON_POST("/orders", j_order_params.dump(), &m_auth_headers), 0);

    if (g_dump_order_responses) COUT << "Order response:" << json_response.dump(4) << endl;

    // for some order, "message" is not found in the json resposne
    // if(json_response.find("message") != json_response.end())
    // return "";; //order cancelled

    // TODO: This mutex is not getting unlocked for buy orders.
    // m_order_notify_cv.wait(mutex_lock);

    order_id = json_response["id"].get<string>();
    updateOrder(order_id, order);

  } else {  // Limit order

    json json_response;
    CONTINUOUS_TRY(json_response = m_query_handle->getJSON_POST("/orders", j_order_params.dump(), &m_auth_headers), 0);

    if (json_response.find("message") == json_response.end()) {
      order_id = json_response["id"].get<string>();
      if (json_response["status"].get<string>() == "pending") {
        m_pending_orders[order_id] = &order;
      }
    }
  }

  updateAccounts();  // update accounts with new balance

  order.setOrderId(order_id);
  return order_id;  // placeholder
}

bool GDAX::connectWebsocket() {
  if (m_websocket_connected) return true;
  try {
    m_websocket_handle->connect(m_websocket_endpoint);
  } catch (const exception& e) {
    CT_CRIT_WARN << e.what() << endl;
    return false;
  } catch (websocketpp::lib::error_code e) {
    CT_CRIT_WARN << e.message() << endl;
    return false;
  } catch (...) {
    CT_CRIT_WARN << "other exception\n";
    return false;
  }

  m_websocket_handle->attachPreambleGeneratorFunction(bind(&GDAX::generateSubscriptionMessage, this));
  m_websocket_connected = true;

  return true;
}

bool GDAX::disconnectWebsocket() {
  if (!m_websocket_connected) return true;

  unsubscribeFromTopic();

  this_thread::sleep_for(chrono::milliseconds(300));

  try {
    m_websocket_handle->close();
  } catch (...) {
    CT_CRIT_WARN << "Unable to close websocket\n";
    return false;
  }

  m_websocket_connected = false;

  return true;
}

bool GDAX::subscribeToTopic() {
  if (!m_websocket_connected) {
    WEBSOCKET_CONN_ERROR("subscribe to topic(s)", sExchangeToString(m_id));
  }

  m_websocket_handle->sendPreamble();

  for (auto& currency_pair : m_trading_pairs) m_markets[currency_pair]->webSocketSubscribed();

  return true;
}

bool GDAX::isOrderComplete(const string orderId) {
  return false;  // placeholder
}

void GDAX::fetchPastOrderIDs(const int how_many, vector<order_id_t>& a_orders) const {
  // placeholder
}

const string GDAX::fetchPastOrderID(const int order) const {
  return "";  // placeholder
}

void GDAX::getPendingOrderIDs(vector<order_id_t>& a_orders) const {
  // placeholder
}

double GDAX::getLimitPrice(const double volume, const bool isBid) {
  return 0;  // placeholder
}

OrderBook* GDAX::getOrderBook(const CurrencyPair currency_pair) {
  if (m_markets.find(currency_pair) == m_markets.end()) {
    NO_MARKET_ERROR(currency_pair, sExchangeToString(m_id));
  }

  OrderBook* order_book_ptr = m_markets[currency_pair]->getOrderBook();

  if (!m_markets[currency_pair]->isWebsocketSubscribed()) {
    try {
      const json json_response = m_query_handle->getJSON_GET("/products/" + currency_pair.toString() +
                                                             "/book?level=2");  // take the first element
      json j_bids = json_response["bids"];
      json j_asks = json_response["asks"];

      for (auto j_bid : j_bids) {
        order_book_ptr->addBidPriceLevel(stod(j_bid[0].get<string>()), stod(j_bid[1].get<string>()));
      }

      for (auto j_ask : j_asks) {
        order_book_ptr->addAskPriceLevel(stod(j_ask[0].get<string>()), stod(j_ask[1].get<string>()));
      }

    } catch (detail::type_error err) {  // NOLINT
      CT_CRIT_WARN << "Exception: " << err.what() << endl;
      return nullptr;
    }
  }

  return order_book_ptr;  // placeholder
}

void GDAX::calculateAuthHeaders(string request_path, json request_params, rest_request_t request_type) {
  string timestamp = to_string(time(nullptr));
  string message;
  string hmac_key;
  string signature;
  string signature_b64;

  string request_params_str = (request_params == json()) ? "" : request_params.dump();

  if (request_type == rest_request_t::GET)
    message = timestamp + "GET" + request_path;
  else if (request_type == rest_request_t::POST) {
    message = timestamp + "POST" + request_path + request_params_str;
  } else if (request_type == rest_request_t::DELETE) {
    message = timestamp + "DELETE" + request_path + request_params_str;
  } else {
    CT_CRIT_WARN << "Invalid request.\n";
    return;
  }

  //    COUT<<"message = "<<message<<endl;

  hmac_key = EncodeDecode::getBase64Decoded(m_apikey.getSecret());
  signature = EncodeDecode::getHmacSha256(hmac_key, message);
  signature_b64 = EncodeDecode::getBase64Encoded(signature);

  m_auth_headers["Content-Type:"] = "Application/JSON";
  m_auth_headers["CB-ACCESS-KEY:"] = m_apikey.getKey();
  m_auth_headers["CB-ACCESS-SIGN:"] = signature_b64;
  m_auth_headers["CB-ACCESS-TIMESTAMP:"] = timestamp;
  m_auth_headers["CB-ACCESS-PASSPHRASE:"] = m_apikey.getPassphrase();

  return;
}

void GDAX::updateAccounts(const json& a_json) {
  json j_null;
  Account account;

  if (m_public_only) {
    CT_CRIT_WARN << "Error: Can't fetch account information without correct API credentials!\n";
    return;
  }

  if (!m_accounts) {
    m_accounts = new ExchangeAccounts(m_id, false, a_json);
  }

  m_account_update_mutex.lock();

  calculateAuthHeaders("/accounts");

  json json_response;
  CONTINUOUS_TRY(json_response = m_query_handle->getJSON_GET("/accounts", &m_auth_headers), 0);

  try {
    for (auto j_account : json_response) {
      account.setID(j_account["id"].get<string>());
      account.setCurrency(j_account["currency"].get<string>());
      account.setAvailable(getJsonValueT<string, double>(j_account, "available"));
      account.setHold(getJsonValueT<string, double>(j_account, "hold"));

      assert(abs(account.getBalance() - getJsonValueT<string, double>(j_account, "balance")) < 1e-6);

      if (!checkIfIncludedInTradingPairs(account.getCurrency())) continue;

      m_accounts->modifyAccount(account);
    }
  } catch (detail::type_error& err) {
    COUT << "json_response = " << json_response.dump(4) << endl;

    if (json_response.find("message") != json_response.end()) {
      CT_WARN << json_response["message"].get<string>() << endl;
    }
  }

  m_account_update_mutex.unlock();
}

bool GDAX::fillCandleSticks(vector<Candlestick>& candlesticks, const CurrencyPair currency_pair, time_t start,
                            time_t end, int granularity) {
  if ((end - start) / granularity > 200) {
    CT_CRIT_WARN << "More than 200 candlesticks requested. This is not supported yet.\n";
    return false;
  }

  try {
    string query_string = "/products/" + currency_pair.toString() + "/candles/?" +
                          "start=" + TradeUtils::getISOTimeFromUTC(start, false) +
                          "&end=" + TradeUtils::getISOTimeFromUTC(end, false) +
                          "&granularity=" + to_string(granularity);

    COUT << "Candlestick Query: " << query_string << endl;

    json json_response = m_query_handle->getJSON_GET(query_string);

    for (auto& j_candlestick : json_response) {
      candlesticks.push_back(Candlestick(Time(0, 0, 0, 0, 0, j_candlestick[0], 0), j_candlestick[3], j_candlestick[4],
                                         j_candlestick[1], j_candlestick[2], 0, 0,
                                         Volume(static_cast<double>(j_candlestick[5]))));
    }

    return true;
  } catch (detail::type_error& err) {  // NOLINT
    CT_WARN << "Exception: " << err.what() << endl;
    return false;
  }
}

int64_t GDAX::fillTrades(TickPeriod& trades, const CurrencyPair currency_pair, int64_t newest_trade_id,
                         int64_t oldest_trade_id) {
  FILE* p_csv_file = NULL;
#ifdef DUMP_LATEST_TICKS_IN_CSV
  string csv_cache_file = currency_pair.toString() + "_" + sExchangeToString(m_id) + ".csv";
  p_csv_file = fopen(csv_cache_file.c_str(), "w");
  ASSERT(p_csv_file);
#endif

  http_header_t response_headers;
  int64_t cb_after = 0;
  json json_response;

  double price;
  double size;
  int64_t trade_id = -1;
  Time timestamp;
  int ret;

  if (newest_trade_id > 0 && newest_trade_id < INT64_MAX) cb_after = newest_trade_id;

  int num_trades_added = 0;

  for (;;) {
    if (p_csv_file) fflush(p_csv_file);

    try {
      if (cb_after > 0) {
        json_response = m_query_handle->getJSON_GET(
            "/products/" + currency_pair.toString() + "/trades?after=" + to_string(cb_after), NULL, &response_headers);
      } else {
        json_response = m_query_handle->getJSON_GET("/products/" + currency_pair.toString() + "/trades", NULL,
                                                    &response_headers);  // take the first element
      }
    } catch (exception& err) {  // NOLINT
      CT_WARN << "Exception: " << err.what() << endl;
      if (trade_id == -1)
        cb_after = newest_trade_id;
      else
        cb_after = trade_id;  // stores the last trade id to start from
      this_thread::sleep_for(chrono::milliseconds(500));
      continue;
    }

    try {
      cb_after = stol(response_headers["cb-after"]);
      //      cb_before = stol(response_headers["cb-before"]);
    } catch (exception& err) {
      CT_WARN << "Exception: " << err.what() << endl;
      if (trade_id == -1)
        cb_after = newest_trade_id;
      else
        cb_after = trade_id;  // stores the last trade id to start from
      this_thread::sleep_for(chrono::seconds(1));
      continue;
    }

    // COUT<<"cb_before = "<<cb_before<<endl;
    // COUT<<"cb_after = "<<cb_after<<endl;

    try {
      for (auto j_trade : json_response) {
        // COUT<<j_trade.dump(4) << endl;
        string side;
        getJsonValue(j_trade, "price", price);
        getJsonValue(j_trade, "size", size);
        side = j_trade["side"].get<string>();
        if (side == "sell") size *= -1.;
        trade_id = j_trade["trade_id"].get<uint64_t>();
        timestamp = Time(j_trade["time"].get<string>());

        if (oldest_trade_id > trade_id) goto print_summery;

        // cache these Ticks in a csv file
        Tick new_tick(timestamp, trade_id, price, size);
        ret = trades.append(new_tick, p_csv_file);

        if (ret == -1) {
          cb_after = trades.getFirstUniqueId();
          CT_CRIT_WARN << "first_trade_id = " << trades.getFirstUniqueId() << endl;
          CT_CRIT_WARN << "last_trade_id = " << trades.getLastUniqueId() << endl;
          CT_CRIT_WARN << "current_trade_id = " << trade_id << endl;
          this_thread::sleep_for(chrono::milliseconds(500));
          break;
        }

        num_trades_added++;

        printPendingTradeFillStatus(trades, currency_pair, num_trades_added);

        if (trade_id == 1) goto print_summery;

        if (oldest_trade_id < 0 && num_trades_added == (-oldest_trade_id)) goto print_summery;

        if (oldest_trade_id > 0 && oldest_trade_id >= trade_id) goto print_summery;
      }
    } catch (detail::type_error& err) {  // NOLINT
      CT_WARN << "Exception: " << err.what() << endl;
      cb_after = trade_id;  // stores the last trade id to start from
      this_thread::sleep_for(chrono::seconds(1));
      continue;
    }

    // this_thread::sleep_for(chrono::milliseconds(200));
  }

print_summery:

  if (p_csv_file) fclose(p_csv_file);

  return trade_id;
}

void GDAX::websocketCallback(json message) {
  if (g_exiting) {
    // WARNNIG << "Exiting GDAX::websocketCallback" << endl;
    this_thread::yield();
    return;
  }

  string message_type = message["type"];

  if (message_type == "l2update" || message_type == "l2update_50" || message_type == "snapshot") {
    PARALLEL_PROCESS(&GDAX::processLevel2, this, message);
  } else if (message_type == "heartbeat") {
    PARALLEL_PROCESS(&GDAX::processHeartbeat, this, message);
  } else if (message_type == "ticker") {
    PARALLEL_PROCESS(&GDAX::processTicker, this, message);
  } else if (message_type == "match") {
    PARALLEL_PROCESS(&GDAX::processMatch, this, message);
  } else if (message_type == "received" || message_type == "open" || message_type == "done" ||
             message_type == "match" || message_type == "change" || message_type == "activate") {
    PARALLEL_PROCESS(&GDAX::processOrders, this, message);
  } else {
    // do nothing, no need to have pop-unnecessory
  }

  //  COUT<<message.dump(4)<<endl;
  //  COUT<<<<message["type"].get<string>()<<endl;
}

json GDAX::generateSubscriptionMessage() {
  json json_topic = {{"type", "subscribe"}, {"product_ids", {}}, {"channels", {}}, {"signature", ""},
                     {"key", ""},           {"passphrase", ""},  {"timestamp", ""}};

  for (auto& currency_pair : m_trading_pairs) json_topic["product_ids"].push_back(currency_pair.toString("-"));

  for (auto& topic : m_topics) {
    json_topic["channels"].push_back(topic);
  }

  calculateAuthHeaders("/users/self/verify");

  json_topic["signature"] = m_auth_headers["CB-ACCESS-SIGN:"];
  json_topic["key"] = m_apikey.getKey();
  json_topic["timestamp"] = to_string(time(nullptr));
  json_topic["passphrase"] = m_apikey.getPassphrase();

  // cout<<json_topic.dump(2)<<endl;

  return json_topic;
}

bool GDAX::unsubscribeFromTopic() {
  if (!m_websocket_connected) {
    COUT << "Coinbase Unsubscribe Error: Websocket is not connected.\n";
    return false;
  }

  for (auto& currency_pair : m_trading_pairs) {
    if (m_markets[currency_pair]->isWebsocketSubscribed()) {
      json json_topic = {{"type", "unsubscribe"}, {"product_ids", {}}, {"channels", {}}};

      json_topic["product_ids"].push_back(currency_pair.toString("-"));

      for (auto& topic : m_topics) {
        json_topic["channels"].push_back(topic);
      }

      bool success = false;
      do {
        json_topic["type"] = "unsubscribe";
        try {
          m_websocket_handle->send(json_topic);
          cout << json_topic.dump() << endl;
          success = true;
        } catch (const exception& e) {
          cout << e.what() << endl;
          this_thread::sleep_for(chrono::milliseconds(100));
        } catch (websocketpp::lib::error_code e) {
          cout << e.message() << endl;
          this_thread::sleep_for(chrono::milliseconds(100));
        } catch (...) {
          cout << "other exception" << endl;
          this_thread::sleep_for(chrono::milliseconds(100));
        }

      } while (!success);

      cout << currency_pair << " unsubscribed successfully\n";

      m_markets[currency_pair]->webSocketUnsubscribed();
    }
  }

  return true;
}

void GDAX::processHeartbeat(json message) {
  CurrencyPair currency_pair;
  static Time heartbeat_time = Time::sMax();

  try {
    if ((Time::sNow() - heartbeat_time) > 5_sec) CT_WARN << "Heartbeat is not received in time\n";

    heartbeat_time = Time::sNow();

    currency_pair = message["product_id"].get<string>();

    m_markets[currency_pair]->setLastTradeId(message["last_trade_id"].get<int64_t>());

    if (message.find("time") != message.end()) {
      if (g_dump_trades_websocket)
        COUT << "[" << Time(message["time"].get<string>()).toISOTimeString()
             << "] last_trade_id = " << m_markets[currency_pair]->getLastTradeId() << "\n";
    } else {
      CT_CRIT_WARN << "Time field is missing from heartbeat\n";
    }

  } catch (...) {
    CT_CRIT_WARN << "processing JSON on GDAX::processHeartbeat function.\n";
  }

  // COUT<<'\r' << message["last_trade_id"] << flush;
}

void GDAX::processTicker(json message) {
  CurrencyPair currency_pair;
  double best_bid, best_ask;

  try {
    currency_pair = message["product_id"].get<string>();

    best_ask = getJsonValueT<string, double>(message, "best_ask");
    best_bid = getJsonValueT<string, double>(message, "best_bid");

    m_markets[currency_pair]->setBidPrice(best_bid);
    m_markets[currency_pair]->setAskPrice(best_ask);

    if (message.find("time") != message.end()) {
      if (g_dump_trades_websocket)
        COUT << "[" << Time(message["time"].get<string>()).millis_since_epoch()
             << "] Ticker price:" << m_markets[currency_pair]->getTickerPrice() << endl;
    }

  } catch (...) {
    CT_CRIT_WARN << "processing JSON on GDAX::processTicker function.\n";
  }
}

void GDAX::processMatch(json message) {
  CurrencyPair currency_pair;

  Time timestamp;
  int64_t trade_id;
  double price;
  double size;
  string maker_order_id;
  string taker_order_id;

  try {
    currency_pair = message["product_id"].get<string>();
    timestamp = Time(message["time"].get<string>());
    trade_id = message["trade_id"].get<int64_t>();
    maker_order_id = message["maker_order_id"].get<string>();
    maker_order_id = message["taker_order_id"].get<string>();
    price = getJsonValueT<string, double>(message, "price");
    size = getJsonValueT<string, double>(message, "size");
    size *= (message["side"].get<string>() == "sell" ? -1 : 1);

    // COUT<<"maker_order_id = "<<maker_order_id<<endl;
    // COUT<<"taker_order_id = "<<taker_order_id<<endl;

    Tick trade = Tick(timestamp, trade_id, price, size);

    if (g_dump_trades_websocket) {
      if (trade.getSize() > 0)
        COUT << CRED << trade << endl;
      else
        COUT << CGREEN << trade << endl;
    }

    if (m_stream_live_data) {
      unique_lock<mutex> mutex_lock(m_match_mutex);
      m_ticks_buffer[currency_pair]->push(trade);
      m_match_notify_cv.notify_all();
      mutex_lock.unlock();
    }
  } catch (...) {
    CT_CRIT_WARN << "processing JSON on GDAX::processMatch function.\n";
  }
}

void GDAX::processOrders(json message) {
  CurrencyPair currency_pair;

  Time timestamp;
  order_id_t order_id;
  string message_type;

  try {
    if (g_dump_order_responses) COUT << message.dump(2);

    order_id = message["order_id"].get<string>();
    message_type = message["type"].get<string>();

    if (message_type == "done") {
      if (m_pending_orders.find(order_id) == m_pending_orders.end())  // market order completed
      {
        COUT << CRED << "Remaining size(" << message["product_id"].get<string>() << ":" << message["side"].get<string>()
             << ")=" << message["remaining_size"].get<string>() << endl;

        unique_lock<mutex> mutex_lock(m_order_mutex);
        m_order_notify_cv.notify_all();
        mutex_lock.unlock();
      } else {  // limit order completed
        string reason = message["reason"].get<string>();
        if (reason == "filled") {
          updateOrder(order_id, *m_pending_orders[order_id]);
        } else if (reason == "canceled") {
          updateAccounts();
        } else if (reason == "open") {  // limit order partially filled
          updateOrder(order_id, *m_pending_orders[order_id], true);
        } else {
          assert(0);
        }
      }
    }
  } catch (...) {
    CT_CRIT_WARN << "processing JSON on GDAX::processMatch function.\n";
  }
}

void GDAX::processLevel2(json message) {
  CurrencyPair currency_pair;

  // COUT<<message.dump(4)<<endl;

  try {
    if (message["type"] == "snapshot") {
      // COUT<<"SNAPSHOT:\n";// << message.dump(4) << endl;

      currency_pair = message["product_id"].get<string>();

      json j_bids = message["bids"];
      json j_asks = message["asks"];

      m_markets[currency_pair]->getOrderBook()->lock();

      m_markets[currency_pair]->getOrderBook()->reset();

      for (auto j_bid : j_bids) {
        m_markets[currency_pair]->getOrderBook()->addBidPriceLevel(stod(j_bid[0].get<string>()),
                                                                   stod(j_bid[1].get<string>()));
      }

      for (auto j_ask : j_asks) {
        m_markets[currency_pair]->getOrderBook()->addAskPriceLevel(stod(j_ask[0].get<string>()),
                                                                   stod(j_ask[1].get<string>()));
      }

      m_markets[currency_pair]->getOrderBook()->unlock();

    } else {
      // COUT<<"L2UPDATE: \n" << message.dump(4) << endl;

      currency_pair = message["product_id"].get<string>();

      json j_changes = message["changes"];

      m_markets[currency_pair]->getOrderBook()->lock();

      for (auto j_change : j_changes) {
        if (j_change[0] == "buy") {
          m_markets[currency_pair]->getOrderBook()->addBidPriceLevel(stod(j_change[1].get<string>()),
                                                                     stod(j_change[2].get<string>()));
        } else if (j_change[0] == "sell") {
          m_markets[currency_pair]->getOrderBook()->addAskPriceLevel(stod(j_change[1].get<string>()),
                                                                     stod(j_change[2].get<string>()));
        }
      }

      m_markets[currency_pair]->getOrderBook()->unlock();

      const Quote& quote = m_markets[currency_pair]->getQuote();

      if (quote.isQuoteInvalid()) {  // || !TradeUtils::isTickerApproxEqual(m_markets[currency_pair]->getTickerPrice(),
                                     //                               quote.getTickerPrice())) {
        CT_WARN << "Invalid quote \n";
        rePopulateOrderBook();
      }
    }
  } catch (...) {
    CT_CRIT_WARN << "processing JSON on GDAX::processLevel2 - snapshot function.\n";
  }
}

TradeHistory* GDAX::getTradeHistory(CurrencyPair currency_pair) {
  TradeHistory* th;

  try {
    th = m_markets.at(currency_pair)->getTradeHistory();
  } catch (...) {
    th = nullptr;
  }

  return th;
}

bool GDAX::repairDatabase(const CurrencyPair currency_pair, bool full_repair) {
  Database<Tick>* db = getTradeHistory(currency_pair)->getDb();
  vector<pair<primary_key_t, primary_key_t>> discontinuities;

  primary_key_t min_key;
  primary_key_t max_key;
  primary_key_t last_key;
  int64_t num_entries;
  int64_t num_trades_saved;
  bool repair_failed = false;
  bool neg_discontinuities = false;

  db->fixDatabase(full_repair, true, min_key, max_key, last_key, num_entries, discontinuities);

  if (!discontinuities.empty()) {
    for (auto& discontinuity : discontinuities) {
      if ((discontinuity.second.unique_id - discontinuity.first.unique_id - 1) < 0) {
        neg_discontinuities = true;
        break;
      }
    }

    if (neg_discontinuities) {
      for (auto& discontinuity : discontinuities) {
        db->deleteData(Time(discontinuity.first.date, discontinuity.first.time),
                       Time(discontinuity.second.date, discontinuity.second.time));
      }
      discontinuities.clear();
      db->fixDatabase(full_repair, true, min_key, max_key, last_key, num_entries, discontinuities);
    }
    // If last_trade_saved = 0, data exists on the database
    for (auto& discontinuity : discontinuities) {
      TickPeriod trades;
      this->fillTrades(trades, currency_pair, discontinuity.second.unique_id, discontinuity.first.unique_id + 1);

      num_trades_saved = trades.storeToDatabase(db);

      if (num_trades_saved != (discontinuity.second.unique_id - discontinuity.first.unique_id - 1)) {
        repair_failed = true;
        CT_CRIT_WARN << "Database repair failed.\n";
        break;
      }

      COUT << CGREEN << "Stored " << num_trades_saved << " trades" << CRESET << " into database with trade_ids = ["
           << discontinuity.first.unique_id + 1 << " - " << discontinuity.second.unique_id - 1 << "]\n";
    }
  }

  if (!repair_failed) db->getMetadata()->setConsistentEntryKey(last_key);

  return !repair_failed;
}

bool GDAX::storeInitialTrades(const CurrencyPair currency_pair) {
  Database<Tick>* db = getTradeHistory(currency_pair)->getDb();
  // load last saved data
  int64_t last_trade_saved = db->getOldestUniqueID();

  TickPeriod trades;
  Time time_now, time_last;

  // If last_trade_saved = 0, data exists on the database
  if (last_trade_saved > 1) {
    int64_t num_trades_saved = 0;
    trades.clear();

    time_last = Time::sNow();

    do {
      last_trade_saved = this->fillTrades(trades, currency_pair, last_trade_saved,
                                          -1000);  // fill 1000 trades at a time

      num_trades_saved += trades.storeToDatabase(db);

      printTradeFillStatus(trades, currency_pair);

      trades.clear();

      time_now = Time::sNow();

      Duration time_remaining = (time_now - time_last) * (last_trade_saved / num_trades_saved);

      COUT << CRED << "Time remaining : " << CWHITE << time_remaining << "\n";

    } while (last_trade_saved > 1);

    db->updateOldestEntryMetadata();
  }

  return false;
}

int64_t GDAX::storeRecentTrades(const CurrencyPair currency_pair) {
  Database<Tick>* db = getTradeHistory(currency_pair)->getDb();

  int64_t num_trades_saved = -1;

  TickPeriod trades;

  // COUT<<"last_trade_saved = "<<last_trade_saved;

  if (db->checkIfFilledFromBeginning()) {
    int64_t first_trade_saved = db->getNewestUniqueID();

    while (db->getNumEntries() != db->getNewestUniqueID()) {
      CT_CRIT_WARN << "total_entries (" << db->getNumEntries() << ") != first_trade_saved(" << first_trade_saved
                   << ") \n";

      COUT << CRED << "Repairing database" << endl;
      repairDatabase(currency_pair);

      first_trade_saved = db->getNumEntries();
    }

    this->fillTrades(trades, currency_pair, 0 /* Fill from latest trade */,
                     first_trade_saved + 1 /* Fill till the last trade */);

    num_trades_saved = trades.storeToDatabase(db);

    printTradeFillStatus(trades, currency_pair);
    trades.clear();
  } else {
    COUT << "Storing initial trades...\n";
    storeInitialTrades(currency_pair);
  }

  return num_trades_saved;
}

bool GDAX::updateOrder(string orderId, Order& order, bool paritial_fill) {
  if (m_public_only) {
    NO_COINBASE_API_ERROR;
  }

  COUT << "orderId = " << orderId << endl;

  calculateAuthHeaders("/orders/" + orderId);

  json json_response;
  CONTINUOUS_TRY(json_response = m_query_handle->getJSON_GET("/orders/" + orderId, &m_auth_headers), 0);

  if (g_dump_order_responses) COUT << "Order Update response:" << json_response.dump(4) << endl;

  if (json_response.find("done_at") != json_response.end()) {
    double filled_size = getJsonValueT<string, double>(json_response, "filled_size");
    //    double fill_fees = getJsonValueT<string, double>(json_response, "fill_fees");
    double executed_value = getJsonValueT<string, double>(json_response, "executed_value");

    double specified_funds = -1;
    double price = -1;

    if (json_response["type"].get<string>() == "limit")
      price = getJsonValueT<string, double>(json_response, "price");
    else {
      assert(json_response["type"].get<string>() == "market");
      price = (executed_value / filled_size);

      if (order.getOrderDir() == order_direction_t::BUY)
        specified_funds = getJsonValueT<string, double>(json_response, "specified_funds");
    }

    if (json_response["side"].get<string>() == "buy") {
      assert(order.getOrderDir() == order_direction_t::BUY);

      if (order.getOrderType() == order_type_t::MARKET)
        order.setExecute(specified_funds, price, m_accounts);
      else
        order.setExecute((filled_size * price), price, m_accounts);
    } else {
      assert(order.getOrderDir() == order_direction_t::SELL);
      order.setExecute(filled_size, price, m_accounts);
    }

    updateAccounts();
    dumpAccountStatus(&order);

    // order should get fully executed before reaching here
    if ((order.getOrderType() != order_type_t::MARKET) && !paritial_fill) completeOrder(&order);
  }

  return true;
}

void GDAX::rePopulateOrderBook() {
  static Time last_repopulated = Time(0);

  if ((Time::sNow() - last_repopulated) < 10_sec) {
    return;  // don't repopulate too frequently
  }

  json json_topic = {{"type", "subscribe"}, {"product_ids", {}}, {"channels", {}}};

  json_topic["channels"].push_back("level2_50");

  for (auto& currency_pair : m_trading_pairs) {
    json_topic["product_ids"].push_back(currency_pair.toString("-"));
  }

  m_websocket_handle->send(json_topic);

  last_repopulated = Time::sNow();
}
