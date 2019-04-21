//
// Created by Subhagato Dutta on 9/14/17.
//

#ifndef CRYPTOTRADER_GDAX_H
#define CRYPTOTRADER_GDAX_H

#include "exchanges/Exchange.h"
#include "exchanges/VirtualExchange.h"

class GDAX : public Exchange, public VirtualExchange {
 private:
  bool init();
  void calculateAuthHeaders(std::string request_path, json request_params = json(),
                            rest_request_t request_type = rest_request_t::GET);

  void subscriptionLoop(CurrencyPair currency_pair);

  bool updateOrder(std::string orderId, Order& order, bool paritial_fill = false);

 public:
  GDAX(const exchange_t a_id, const json& config);
  virtual ~GDAX();

  // casting functions
  virtual VirtualExchange* castVirtualExchange() {
    return this;
  }
  virtual Exchange* castExchange() {
    return this;
  }
  virtual GDAX* castGDAX() {
    return this;
  }

  bool isAuthenticated() const {
    return !m_public_only;
  }

  Quote getRestAPIQuote(const CurrencyPair& currency_pair);

  void websocketCallback(json message);

  TradeHistory* getTradeHistory(CurrencyPair currency_pair);

  bool storeInitialTrades(const CurrencyPair currency_pair);

  int64_t storeRecentTrades(const CurrencyPair currency_pair);

  // pure virtual functions from Exchange
  virtual void updateAccounts(const json& a_json = json());

  virtual double getTicker(const CurrencyPair& currency_pair) const;

  virtual Quote getQuote(const CurrencyPair& currency_pair);

  virtual bool fillCandleSticks(std::vector<Candlestick>& candlesticks, CurrencyPair currency_pair, time_t start,
                                time_t end, int granularity);

  virtual order_id_t order(Order& order);

  virtual bool cancelOrder(order_id_t order_id, bool order_from_current_run = true);
  virtual bool cancelAllOrders();

  virtual bool isOrderComplete(std::string orderId);

  virtual void fetchPastOrderIDs(int how_many, std::vector<order_id_t>& a_orders) const;

  virtual const std::string fetchPastOrderID(int order) const;

  virtual void getPendingOrderIDs(std::vector<order_id_t>& a_orders) const;

  virtual double getLimitPrice(double volume, bool isBid);

  virtual OrderBook* getOrderBook(CurrencyPair currency_pair);

  virtual int64_t fillTrades(
      TickPeriod& trades, const CurrencyPair currency_pair, int64_t newest_trade_id = 0, /* 0 means current */
      int64_t oldest_trade_id = 1 /* 1 means the oldest trade, -N number means go N below newest_trade_id */);

  virtual bool repairDatabase(const CurrencyPair currency_pair, bool full_repair = false);

  virtual std::unordered_map<CurrencyPair, TradeHistory*>& getVirFullHistory() {
    return m_full_history;
  }
  virtual std::unordered_map<CurrencyPair, TradeHistory*>& getVirPartHistory() {
    return m_history_till_simulation_time;
  }

  virtual json generateSubscriptionMessage();
  virtual bool unsubscribeFromTopic();

  virtual bool disconnectWebsocket();
  virtual bool connectWebsocket();

  virtual bool subscribeToTopic();

  void processHeartbeat(json message);
  void processTicker(json message);
  void processMatch(json message);
  void processOrders(json message);
  void processLevel2(json message);

  void rePopulateOrderBook();
};

#endif  // CRYPTOTRADER_GDAX_H
