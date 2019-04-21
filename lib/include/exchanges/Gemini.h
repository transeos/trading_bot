//
// Created by Subhagato Dutta on 9/14/17.
//

#ifndef CRYPTOTRADER_GEMINI_H
#define CRYPTOTRADER_GEMINI_H

#include "TradeHistory.h"
#include "exchanges/Exchange.h"
#include "exchanges/VirtualExchange.h"

class Gemini : public Exchange, public VirtualExchange {
 private:
  bool init();
  void calculateAuthHeaders(std::string request_path, json request_params = json(),
                            rest_request_t request_type = rest_request_t::GET);

  void subscriptionLoop(CurrencyPair currency_pair);

  Time fillTrades(TickPeriod& trades, const CurrencyPair currency_pair, Time since, Time till, int64_t& last_trade_id,
                  int num_trades_at_a_time = 100);

 public:
  Gemini(const exchange_t a_id, const json& config);
  virtual ~Gemini();

  // casting functions
  virtual VirtualExchange* castVirtualExchange() {
    return this;
  }
  virtual Exchange* castExchange() {
    return this;
  }
  virtual Gemini* castGemini() {
    return this;
  }

  bool isAuthenticated() const {
    return !m_public_only;
  }

  void websocketCallback(json message);

  TradeHistory* getTradeHistory(CurrencyPair currency_pair);

  int64_t storeInitialTrades(const CurrencyPair currency_pair);

  int64_t storeRecentTrades(const CurrencyPair currency_pair);

  // pure virtual functions from Exchange
  virtual void updateAccounts(const json& a_json = json()) {
    assert(0);
  }

  virtual double getTicker(const CurrencyPair& currency_pair) const;

  virtual Quote getQuote(const CurrencyPair& currency_pair);

  virtual bool fillCandleSticks(std::vector<Candlestick>& candlesticks, CurrencyPair currency_pair, time_t start,
                                time_t end, int granularity);

  virtual order_id_t order(Order& order);

  virtual bool cancelOrder(order_id_t order_id, bool order_from_current_run = true) {
    assert(0);
    return false;
  }
  virtual bool cancelAllOrders() {
    assert(0);
    return false;
  }

  virtual bool isOrderComplete(std::string orderId);

  virtual void fetchPastOrderIDs(int how_many, std::vector<order_id_t>& a_orders) const;

  virtual const std::string fetchPastOrderID(int order) const;

  virtual void getPendingOrderIDs(std::vector<order_id_t>& a_orders) const;

  virtual double getLimitPrice(double volume, bool isBid);

  virtual OrderBook* getOrderBook(CurrencyPair currency_pair);

  virtual int64_t fillTrades(
      TickPeriod& trades, const CurrencyPair currency_pair, int64_t newest_trade_id = 0, /* 0 means current */
      int64_t oldest_trade_id = 1 /* 1 means the oldest trade, -N number means go N below newest_trade_id */) {
    // TODO
    assert(0);
  }

  virtual bool repairDatabase(const CurrencyPair currency_pair, bool full_repair = false);

  virtual std::unordered_map<CurrencyPair, TradeHistory*>& getVirFullHistory() {
    return m_full_history;
  }
  virtual std::unordered_map<CurrencyPair, TradeHistory*>& getVirPartHistory() {
    return m_history_till_simulation_time;
  }

  virtual bool connectWebsocket() {
    CT_CRIT_WARN << "Not implemented!\n";
    return false;
  }

  virtual bool disconnectWebsocket() {
    CT_WARN << "Not implemented!\n";
    return false;
  }

  virtual bool subscribeToTopic() {
    CT_CRIT_WARN << "Not implemented!\n";
    return false;
  }

  virtual json generateSubscriptionMessage();
  virtual bool unsubscribeFromTopic();

  virtual void processHeartbeat(json message);
  virtual void processTicker(json message);
  virtual void processMatch(json message);
  virtual void processOrders(json message);
  virtual void processLevel2(json message);
};

#endif  // CRYPTOTRADER_GEMINI_H
