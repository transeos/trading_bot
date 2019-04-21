//
// Created by Subhagato Dutta on 9/14/17.
//

#include "exchanges/Gemini.h"
#include "Database.h"
#include "Quote.h"
#include "Tick.h"
#include "utils/EncodeDecode.h"
#include "utils/ErrorHandling.h"
#include "utils/JsonUtils.h"
#include "utils/RestAPI2JSON.h"
#include "utils/TraderUtils.h"
#include <csv.h>
#include <thread>

using namespace std;
using namespace nlohmann;
using namespace placeholders;

static set<string> message_types = {"ticker", "heartbeat", "snapshot", "l2update", "matches", "received",
                                    "open",   "done",      "match",    "change",   "activate"};

Gemini::Gemini(const exchange_t a_id, const json& config) : Exchange(a_id, config) {
  m_topics = {"heartbeat", "level2", "ticker", "matches", "user"};

  init();
}

Gemini::~Gemini() {
  disconnectWebsocket();
}

bool Gemini::init() {
  m_mode = exchange_mode_t::SIMULATION;  // default mode is set to simulation

  m_stream_live_data = false;  // by default live data streaming is disabled

  // Supported currency pairs in the exchange

  m_query_handle = new RestAPI2JSON(m_rest_api_endpoint, 2);

  m_websocket_handle = new Websocket2JSON(true);

  m_websocket_handle->bindCallback(bind(&Gemini::websocketCallback, this, _1));

  // Supported currencies in the exchange

  try {
    const json json_response = m_query_handle->getJSON_GET("/symbols");  // take the first element

    for (auto j : json_response)  // access by value, the type of i is int
    {
      string symbol = j.get<string>();
      CurrencyPair currency_pair(symbol);

      if (symbol == "btcusd") {
        m_currencies.push_back(Currency("BTC", 1e-8));
        m_markets.insert(make_pair(currency_pair, new Market(currency_pair, 1e-5, 1000, 1e-8)));
      } else if (symbol == "ethusd") {
        m_currencies.push_back(Currency("ETH", 1e-6));
        m_markets.insert(make_pair(currency_pair, new Market(currency_pair, 1e-3, 1000, 1e-6)));
      } else if (symbol == "ethbtc") {
        m_markets.insert(make_pair(currency_pair, new Market(currency_pair, 1e-3, 1000, 1e-6)));
      }
    }

  } catch (detail::type_error err) {  // NOLINT
    CT_CRIT_WARN << "Exception: " << err.what() << endl;
    return false;
  } catch (runtime_error err) {  // NOLINT
    CT_CRIT_WARN << "Exception: " << err.what() << endl;
    return false;
  }

  initMarkets();

  return true;
}

double Gemini::getTicker(const CurrencyPair& currency_pair) const {
  double ticker_val = 0;
  double bid_price;
  double ask_price;

  if (!m_websocket_connected) {
    try {
      json json_response =
          m_query_handle->getJSON_GET("/pubticker/" + currency_pair.toString("", true));  // take the first element
      bid_price = getJsonValueT<string, double>(json_response, "bid");
      ask_price = getJsonValueT<string, double>(json_response, "ask");
      ticker_val = (bid_price + ask_price) / 2;
    } catch (detail::type_error& err) {  // NOLINT
      CT_CRIT_WARN << "Exception: " << err.what() << endl;
      return -1.0;
    }
  } else {
    ticker_val = m_markets.at(currency_pair)->getTickerPrice();
  }

  return ticker_val;
}

Quote Gemini::getQuote(const CurrencyPair& currency_pair) {
  double bid_price = 0;
  double bid_size = 0;
  double ask_price = 0;
  double ask_size = 0;

  if (m_markets.find(currency_pair) == m_markets.end()) {
    NO_MARKET_ERROR(currency_pair, sExchangeToString(m_id));
  }

  if (!m_markets[currency_pair]->isWebsocketSubscribed()) {
    try {
      const json json_response =
          m_query_handle->getJSON_GET("/book/" + currency_pair.toString("", false) +
                                      "?limit_bids=1&limit_asks=1");  // ask for the top of order book
      json j_bid = json_response["bids"];
      json j_ask = json_response["asks"];

      bid_price = stod(j_bid[0]["price"].get<string>());
      bid_size = stod(j_bid[0]["amount"].get<string>());

      ask_price = stod(j_ask[0]["price"].get<string>());
      ask_size = stod(j_ask[0]["amount"].get<string>());

    } catch (detail::type_error err) {  // NOLINT
      CT_CRIT_WARN << "Exception: " << err.what() << endl;
    }
  }

  return Quote(bid_price, bid_size, ask_price, ask_size);
}

order_id_t Gemini::order(Order& order) {
#if 0  // simple order testing
    COUT<<"Placing limit order->\n";

    json j_order_params = {
            {"type", "limit"},
            {"side", "buy"},
            {"product_id", currency_pair.toString()},
            {"price", 8000.0},
            {"size", 0.001}
    };

    if (m_public_only) {
        ERROR("Can't place order without correct API credentials!");
    }

    calculateAuthHeaders("/orders", j_order_params, rest_request_t::POST);

    const json json_response = m_query_handle->getJSON_POST("/orders", j_order_params.dump(), &m_auth_headers);

    COUT<<"Order details:"<<json_response.dump(4)<<endl;
#endif
  return "";  // placeholder
}

bool Gemini::isOrderComplete(const string orderId) {
  return false;  // placeholder
}

void Gemini::fetchPastOrderIDs(const int how_many, vector<order_id_t>& a_orders) const {
  // placeholder
}

const string Gemini::fetchPastOrderID(const int order) const {
  return "";  // placeholder
}

void Gemini::getPendingOrderIDs(vector<order_id_t>& a_orders) const {
  // placeholder
}

double Gemini::getLimitPrice(const double volume, const bool isBid) {
  return 0;  // placeholder
}

OrderBook* Gemini::getOrderBook(const CurrencyPair currency_pair) {
  if (m_markets.find(currency_pair) == m_markets.end()) {
    NO_MARKET_ERROR(currency_pair, sExchangeToString(m_id));
  }

  OrderBook* order_book_ptr = m_markets[currency_pair]->getOrderBook();

  if (!m_markets[currency_pair]->isWebsocketSubscribed()) {
    try {
      const json json_response =
          m_query_handle->getJSON_GET("/book/" + currency_pair.toString("", false));  // take the first element
      json j_bids = json_response["bids"];
      json j_asks = json_response["asks"];

      for (auto j_bid : j_bids) {
        order_book_ptr->addBidPriceLevel(stod(j_bid["price"].get<string>()), stod(j_bid["amount"].get<string>()));
      }

      for (auto j_ask : j_asks) {
        order_book_ptr->addAskPriceLevel(stod(j_ask["price"].get<string>()), stod(j_ask["amount"].get<string>()));
      }

    } catch (detail::type_error err) {  // NOLINT
      CT_CRIT_WARN << "Exception: " << err.what() << endl;
      return nullptr;
    }
  }

  return order_book_ptr;  // placeholder
}

void Gemini::calculateAuthHeaders(string request_path, json request_params, rest_request_t request_type) {
  // TODO: implement private endpoint

  return;
}

bool Gemini::fillCandleSticks(vector<Candlestick>& candlesticks, const CurrencyPair currency_pair, time_t start,
                              time_t end, int granularity) {
  return false;  // Not supported in Gemini
}

Time Gemini::fillTrades(TickPeriod& trades, const CurrencyPair currency_pair, Time since, Time till,
                        int64_t& last_trade_id, int num_trades_at_a_time) {
  json json_response;
  double price;
  double size;
  int64_t trade_id;
  Time timestamp;
  int64_t num_trades_added = 0;
  bool first_trade = true;

  assert(num_trades_at_a_time <= 500);

  for (;;) {
    try {
      json_response = m_query_handle->getJSON_GET(
          "/trades/" + currency_pair.toString("", false) + "?timestamp=" + to_string(since.millis_since_epoch()) +
          "&limit_trades=" + to_string(num_trades_at_a_time));  // fetch num_trades trades at a time

    } catch (runtime_error& err) {  // NOLINT
      CT_WARN << "Exception: " << err.what() << endl;
      this_thread::sleep_for(chrono::milliseconds(500));
      continue;
    }

    try {
      for (auto j_trade_itr = json_response.rbegin(); j_trade_itr < json_response.rend(); j_trade_itr++) {
        // COUT<<j_trade.dump(4) << endl;
        string side;
        json j_trade = *j_trade_itr;
        getJsonValue(j_trade, "price", price);
        getJsonValue(j_trade, "amount", size);
        side = j_trade["type"].get<string>();
        if (side == "sell")
          size = getJsonValueT<string, double>(j_trade, "amount");
        else if (side == "buy")  // buy for Gemini is oposite of GDAX
          size = -1 * getJsonValueT<string, double>(j_trade, "amount");
        else
          COUT << "Ignoring auction trades.\n";

        trade_id = j_trade["tid"].get<uint64_t>();
        timestamp = Time(j_trade["timestampms"].get<uint64_t>() * 1000LL);

        if (trade_id <= last_trade_id) {
          // COUT<<"Skipping trade with trade_id = "<<trade_id<<", because it's before "<<last_trade_id<<endl;
          continue;
        }

        if (timestamp > till) break;

        // COUT<<Tick(timestamp, trade_id, price, size)<<endl;

        Tick new_tick(timestamp, trade_id, price, size);
        int ret = trades.append(new_tick);

        if (first_trade) {
          // COUT<<"First : " << trades.front() << endl;
          // COUT<<"Last : " << trades.back() << endl;
          first_trade = false;
        }

        if (ret == -1) {
          this_thread::sleep_for(chrono::milliseconds(500));
          break;
        } else {
          last_trade_id = trade_id;
          num_trades_added++;
        }

        printPendingTradeFillStatus(trades, currency_pair, num_trades_added);
      }

      since = timestamp;
    } catch (exception& err) {  // NOLINT
      CT_WARN << "Exception: " << err.what() << endl;
      this_thread::sleep_for(chrono::seconds(1));
      continue;
    }

    if (static_cast<int64_t>(json_response.size()) < num_trades_at_a_time) break;

    if (since > till) break;
  }

  // CT_INFO << CBLUE<<"Added " << num_trades_added << " trades for " << currency_pair.toString() << " pair."<<endl;

  // if (num_trades_added > 0)
  //  COUT<<"Last : " << trades.front() << endl;

  return trades.back().getTimeStamp();
}

void Gemini::websocketCallback(json message) {
  // TODO: to be supported
}

json Gemini::generateSubscriptionMessage() {
  // TODO: to be supported
  json json_topic;
  return json_topic;
}

bool Gemini::unsubscribeFromTopic() {
  // TODO: to be supported
  return true;
}

void Gemini::processHeartbeat(json message) {
  // TODO: to be supported
}

void Gemini::processTicker(json message) {
  // TODO: to be supported
}

void Gemini::processMatch(json message) {
  // TODO: to be supported
}

void Gemini::processOrders(json message) {
  // TODO: to be supported
}

void Gemini::processLevel2(json message) {
  // TODO: to be supported
}

TradeHistory* Gemini::getTradeHistory(CurrencyPair currency_pair) {
  // TODO: to be supported

  TradeHistory* th;

  try {
    th = m_markets.at(currency_pair)->getTradeHistory();
  } catch (...) {
    th = nullptr;
  }

  return th;
}

bool Gemini::repairDatabase(const CurrencyPair currency_pair, bool full_repair) {
  // TODO: to be supported
  Database<Tick>* db = getTradeHistory(currency_pair)->getDb();

  primary_key_t min_key;
  primary_key_t max_key;
  primary_key_t last_key;
  int64_t num_entries;
  vector<pair<primary_key_t, primary_key_t>> discontinuity_id_pairs;

  db->fixDatabase(full_repair, true, min_key, max_key, last_key, num_entries, discontinuity_id_pairs);

  db->getMetadata()->setConsistentEntryKey(last_key);

  return true;
}

double determineSide(int64_t order_id, int64_t last_order_id, string side, string last_side, string order_type,
                     string last_order_type, string execution_options, string last_execution_options,
                     double remaining_size, double last_remaining_size) {
  double factor = 1.0;
  bool maker = false;

  if (order_type == "market" && last_order_type == "limit") {
    maker = false;  // taker trade
  } else if (order_type == "limit" && last_order_type == "market") {
    maker = true;  // maker trade
  } else if (order_type == "limit" && last_order_type == "limit") {
    if (execution_options == "maker-or-cancel") {
      maker = true;  // maker trade
    } else if (last_execution_options == "maker-or-cancel") {
      maker = false;  // taker trade
    } else if (execution_options == "immediate-or-cancel") {
      maker = false;
    } else if (last_execution_options == "immediate-or-cancel") {
      maker = true;
    } else {  // not taking remaining size as consideration
      maker = (order_id < last_order_id);
    }

  } else {
    maker = false;
  }

  factor = (side == "sell") ? 1 : -1;
  factor *= (maker) ? -1 : 1;

  return factor;
}

int64_t Gemini::storeInitialTrades(const CurrencyPair currency_pair) {
  Database<Tick>* db = getTradeHistory(currency_pair)->getDb();

  int64_t event_id;
  string event_date;
  string event_time;
  string event_millis;
  string symbol;
  string side, last_side;
  string order_type, last_order_type;
  string execution_options, last_execution_options;
  double price;
  double size;
  double remaining_size, last_remaining_size = -1.0;
  int64_t num_trades_saved = 0;

  Time csv_initial_date = Time(m_config["historic_data_start_dates"][currency_pair.toString()], "", 0);
  Time csv_date;
  const string csv_foldername = g_trader_home + "/" + m_config["historic_data_folder"].get<string>();

  // create folder if required
  TradeUtils::createDir(csv_foldername);

  if (db->checkIfFilledFromBeginning() == false)
    csv_date = csv_initial_date;
  else {
    Tick newest_row;
    db->getNewestRow(newest_row);
    csv_date = newest_row.getTimeStamp().quantize(Duration(1));
  }

  while (true) {
    string csv_filename = "/" + currency_pair.toString("") + "_order_fills_" + csv_date.toDateString("") + ".csv";

    COUT << csv_foldername + csv_filename << endl;
    if (!fstream(csv_foldername + csv_filename).good()) break;

    io::CSVReader<12> csv_reader(csv_foldername + csv_filename);

    csv_reader.read_header(io::ignore_extra_column, "Event ID", "Event Date", "Event Time", "Event Millis", "Order ID",
                           "Order Type", "Execution Options", "Symbol", "Side",
                           "Fill Price (" + Currency::sCurrencyToString(currency_pair.getQuoteCurrency()) + ")",
                           "Fill Quantity (" + Currency::sCurrencyToString(currency_pair.getBaseCurrency()) + ")",
                           "Remaining Quantity (" + Currency::sCurrencyToString(currency_pair.getBaseCurrency()) + ")");

    int64_t trade_id, last_trade_id = 0;
    int64_t order_id, last_order_id = 0;
    Time timestamp;
    CurrencyPair csv_currency_pair;
    double side_factor = 1.0;
    TickPeriod trades(false);
    uint64_t csv_lines_count = 0;

    while (csv_reader.read_row(event_id, event_date, event_time, event_millis, order_id, order_type, execution_options,
                               symbol, side, price, size, remaining_size)) {
      trade_id = event_id;
      timestamp = Time(event_date, event_time, stod(event_millis) / 1000.0) + 5_hour;
      csv_currency_pair = CurrencyPair(symbol);
      assert(csv_currency_pair == currency_pair);

      if (last_trade_id == trade_id)  // 2 trades are received
      {
        side_factor = determineSide(order_id, last_order_id, side, last_side, order_type, last_order_type,
                                    execution_options, last_execution_options, remaining_size, last_remaining_size);

        Tick new_tick(timestamp, trade_id, price, size * side_factor);
        trades.append(new_tick);
      } else if (csv_lines_count % 2) {
        CT_WARN << "Pairing trade not found. Skipping trade.." << endl;
        last_trade_id = trade_id;
        continue;
      }

      last_trade_id = trade_id;
      last_remaining_size = remaining_size;
      last_order_id = order_id;
      last_side = side;
      last_execution_options = execution_options;
      last_order_type = order_type;
      csv_lines_count++;
    }

    COUT << "trades.size() = " << trades.size() << " csv_lines_count = " << csv_lines_count << endl;
    assert(trades.size() == csv_lines_count / 2);

    num_trades_saved += trades.storeToDatabase(db);

    if (csv_date == csv_initial_date) db->updateOldestEntryMetadata();

    COUT << trades << endl;

    csv_date += Duration(1, 0, 0, 0);  // read next file
  }

  return num_trades_saved;
}

int64_t Gemini::storeRecentTrades(const CurrencyPair currency_pair) {
  Database<Tick>* db = getTradeHistory(currency_pair)->getDb();

  int64_t num_trades_saved = -1;

  // COUT<<"last_trade_saved = "<<last_trade_saved;

  if (db->checkIfFilledFromBeginning()) {
    Tick latest_tick;
    db->getNewestRow(latest_tick);

    Time oldest_timestamp = latest_tick.getTimeStamp();
    Time current_timestamp = Time::sNow();
    int64_t last_trade_id = latest_tick.getUniqueID();

    if (oldest_timestamp < (current_timestamp - Duration(6, 23, 0, 0))) {
      CT_INFO << "Not all past data is populated." << endl;
      return storeInitialTrades(currency_pair);
    }

    TickPeriod trades(false);  // not consecutive

    fillTrades(trades, currency_pair, oldest_timestamp, current_timestamp, last_trade_id, 500);

    num_trades_saved = trades.storeToDatabase(db);

    printTradeFillStatus(trades, currency_pair);
    trades.clear();

  } else {
    num_trades_saved = storeInitialTrades(currency_pair);
  }

  return num_trades_saved;
}
