//
// Created by Subhagato Dutta on 9/9/17.
//

#ifndef CRYPTOTRADER_EXCHANGE_H
#define CRYPTOTRADER_EXCHANGE_H

#include "APIKey.h"
#include "CandlePeriod.h"
#include "Enums.h"
#include "ExchangeAccounts.h"
#include "Market.h"
#include "TickPeriod.h"
#include "TradeHistory.h"
#include "set"
#include "utils/Websocket2JSON.h"

class VirtualExchange;
class GDAX;
class Gemini;
class Quote;
class Order;
class Candlestick;
class OrderBook;
class Tick;
class RestAPI2JSON;

class Exchange {
 private:
  Time m_controller_time;

  void populateMinOrderAmount();
  void poupulateResidualAmountsData();

  std::vector<const Order*> m_cancelled_orders;

 protected:
  exchange_t m_id;
  exchange_mode_t m_mode;
  APIKey m_apikey;
  bool m_public_only;
  std::vector<Currency> m_currencies;
  ExchangeAccounts* m_accounts;
  std::string m_rest_api_endpoint;
  std::unordered_map<CurrencyPair, Market*> m_markets;
  std::unordered_map<CurrencyPair, TradeHistory*> m_histories_till_ctrl_time;
  std::vector<CurrencyPair> m_trading_pairs;
  bool m_stream_live_data;
  json m_config;
  std::unordered_map<std::string, Order*> m_pending_orders;
  std::set<std::string> m_topics;
  RestAPI2JSON* m_query_handle;
  Websocket2JSON* m_websocket_handle;
  http_header_t m_auth_headers;
  std::string m_websocket_endpoint;
  bool m_websocket_connected;

  // mutex
  std::condition_variable m_match_notify_cv;
  std::mutex m_match_mutex;
  std::condition_variable m_order_notify_cv;
  std::mutex m_order_mutex;
  std::mutex m_account_update_mutex;

  std::unordered_map<CurrencyPair, std::queue<Tick>*> m_ticks_buffer;
  std::unordered_map<currency_t, double> m_min_order_amounts;
  std::unordered_map<currency_t, residual_amount_t> m_residual_amounts;

  fee_t m_fee;

  bool m_consecutive;

  void initMarkets();

  void clearOrder(const order_id_t order_id);

 public:
  Exchange(const exchange_t a_id, const json& a_config);
  ~Exchange();

  static std::string sExchangeToString(const exchange_t exchange_id, bool lower = false);
  static exchange_t sStringToExchange(std::string a_exchange);

  // casting functions
  virtual VirtualExchange* castVirtualExchange() = 0;
  virtual GDAX* castGDAX() {
    return NULL;
  }
  virtual Gemini* castGemini() {
    return NULL;
  }

  // pure virtual functions
  virtual void updateAccounts(const json& a_json = json()) = 0;

  virtual double getTicker(const CurrencyPair& currency_pair) const = 0;

  virtual Quote getQuote(const CurrencyPair& currency_pair) = 0;

  virtual bool fillCandleSticks(std::vector<Candlestick>& candlesticks, const CurrencyPair currency_pair, time_t start,
                                time_t end, int granularity) = 0;

  virtual bool cancelOrder(order_id_t order_id, bool order_from_current_run = true) = 0;
  virtual bool cancelAllOrders() = 0;

  virtual order_id_t order(Order& order) = 0;

  virtual bool isOrderComplete(const std::string orderId) = 0;

  virtual void fetchPastOrderIDs(const int how_many, std::vector<order_id_t>& a_orders) const = 0;

  // TODO: change to std:string&
  virtual const std::string fetchPastOrderID(const int order) const = 0;

  virtual void getPendingOrderIDs(std::vector<order_id_t>& a_orders) const = 0;

  virtual double getLimitPrice(const double volume, const bool isBid) = 0;

  virtual OrderBook* getOrderBook(const CurrencyPair currency_pair) = 0;

  virtual int64_t fillTrades(
      TickPeriod& trades, const CurrencyPair currency_pair, int64_t newest_trade_id = 0, /* 0 means current */
      int64_t oldest_trade_id = 1 /* 1 means the oldest trade, -N number means go N below newest_trade_id */) = 0;

  virtual bool repairDatabase(const CurrencyPair currency_pair, bool full_repair = false) = 0;

  virtual std::unordered_map<CurrencyPair, TradeHistory*>& getVirFullHistory() = 0;
  virtual std::unordered_map<CurrencyPair, TradeHistory*>& getVirPartHistory() = 0;

  virtual bool connectWebsocket() = 0;
  virtual bool disconnectWebsocket() = 0;
  virtual bool subscribeToTopic() = 0;

  virtual json generateSubscriptionMessage() = 0;

  bool cancelLimitOrders(const CurrencyPair& currency_pair, const bool buy);
  bool cancelPendingOrders(const CurrencyPair& currency_pair);

  bool completeOrder(Order* ap_order);

  order_id_t placeOrder(CurrencyPair currency_pair, double amount, order_type_t type, order_direction_t direction,
                        double target_price, const Time time_instant, const double current_price);

  const json& getConfig() const {
    return m_config;
  }

  void setConfig(const json& config) {
    m_config = config;
  }

  size_t getCurrencies(std::vector<Currency>& currencies) const {
    currencies = m_currencies;
    return currencies.size();
  }

  size_t getCurrencyPairs(std::vector<CurrencyPair>& currency_pairs) const;

  exchange_t getExchangeID() const {
    return m_id;
  }
  exchange_mode_t getMode() const {
    return m_mode;
  }

  std::string getExchangeIDString(bool lower = false) const {
    return sExchangeToString(m_id, lower);
  }

  const TickPeriod* getTrades(const CurrencyPair currency_pair) const {
    ASSERT(m_markets.find(currency_pair) != m_markets.end());
    return m_markets.at(currency_pair)->getTradeHistory()->getTickPeriod();
  }
  const TradeHistory* getTradeHistory(const CurrencyPair a_currency_pair) const {
    ASSERT(m_markets.find(a_currency_pair) != m_markets.end());
    return m_markets.at(a_currency_pair)->getTradeHistory();
  }
  TradeHistory* getTradeHistory(const CurrencyPair a_currency_pair) {
    ASSERT(m_markets.find(a_currency_pair) != m_markets.end());
    return m_markets.at(a_currency_pair)->getTradeHistory();
  }
  double getSpread(const CurrencyPair& a_currency_pair) const {
    ASSERT(m_markets.find(a_currency_pair) != m_markets.end());
    return m_markets.at(a_currency_pair)->getSpread();
  }

  TradeHistory* getDelayedTradeHistory(const CurrencyPair a_currency_pair) {
    ASSERT(m_histories_till_ctrl_time.find(a_currency_pair) != m_histories_till_ctrl_time.end());
    return m_histories_till_ctrl_time.at(a_currency_pair);
  }

  bool saveToCSV(const std::string& a_exported_data_name, const Time a_start_time, const Time a_end_time,
                 const std::string& a_database_dir);

  void restoreFromCSV(const std::string& a_csv_file_dir);

  double getUsableBalance(currency_t currency) const;
  double getTotalBalance(currency_t currency) const;

  void dumpAccountStatus(const Order* ap_order);

  void printTradeFillStatus(const TickPeriod& a_trades, const CurrencyPair& a_currency_pair) const;
  void printPendingTradeFillStatus(const TickPeriod& a_trades, const CurrencyPair& a_currency_pair,
                                   int a_num_trades_added) const;

  const ExchangeAccounts* getExchangeAccounts() const {
    return m_accounts;
  }

  void initForTrading(const Duration a_history_duration, const Time a_start_time = Time::sNow(),
                      const Time a_end_time = Time::sMax(), const bool called_from_controller = true);

  void setMode(const std::vector<CurrencyPair>& trading_pairs, exchange_mode_t mode = exchange_mode_t::REAL);

  void fillRealtimeTrades();

  const std::vector<CurrencyPair>& getTradingPairs() const {
    return m_trading_pairs;
  }

  double getMinOrderAmount(const currency_t a_currency) const {
    return (m_min_order_amounts.find(a_currency) == m_min_order_amounts.end()) ? 0 : m_min_order_amounts.at(a_currency);
  }

  Time getControllerTime() const {
    return m_controller_time;
  }
  void setControllerTime(const Time a_time) {
    m_controller_time = a_time;
  }

  bool checkIfIncludedInTradingPairs(const currency_t a_currency);

  residual_amount_t getResidualAmount(const currency_t a_currency) const {
    return (m_residual_amounts.find(a_currency) == m_residual_amounts.end() ? residual_amount_t{0, 0}
                                                                            : m_residual_amounts.at(a_currency));
  }

  fee_t getFee() const {
    return m_fee;
  }

  const Order* getOrder(const order_id_t a_order_id) const {
    return ((m_pending_orders.find(a_order_id) == m_pending_orders.end()) ? NULL : m_pending_orders.at(a_order_id));
  }

  const std::unordered_map<CurrencyPair, Market*>& getMarkets() const {
    return m_markets;
  }

  double getBidPrice(const CurrencyPair& currency_pair) const;
  double getAskPrice(const CurrencyPair& currency_pair) const;

  const Order* lastCancelledOrder() const {
    return (m_cancelled_orders.size() ? m_cancelled_orders.back() : NULL);
  }
};

namespace std {
template <>
struct hash<exchange_t> {
  std::size_t operator()(const exchange_t& k) const {
    using std::hash;

    // Compute  hash values for exchange_t as integer:

    return (hash<int>()((int)k));
  }
};
}

#endif  // CRYPTOTRADER_EXCHANGE_H
