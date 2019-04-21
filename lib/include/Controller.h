// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 21/04/19.
//
//*****************************************************************

#pragma once

#include "exchanges/Exchange.h"

class Time;
class CurrencyPair;
class TradeAlgo;

class Controller {
 private:
  exchange_mode_t m_mode;

  TradeAlgo* mp_TradeAlgo;

  Time m_start_time;
  Time m_end_time;
  Duration m_history;

  mutable Time m_controller_time;

  std::map<exchange_t, std::unordered_map<currency_t, double>> m_prices;
  std::map<exchange_t, Exchange*> m_exchanges;

  double m_starting_portfolio_value;
  std::map<exchange_t, std::unordered_map<currency_t, double>> m_initial_prices;

  // fraction of order which we are able to consume
  double m_sim_vol_fraction;

  Duration m_sim_max_order_place_delay;

  // no tick counter in sec
  int m_time_passed_last_tick;

  // mutex
  std::mutex m_algo_event_mutex;

  // stats summary file
  FILE* m_summary_file;

  // order log
  FILE* m_order_log;

  bool getNextTradeForSimulation();

  void adjustTimeAndCheckForIntervalEvents(const Time& a_tick_time);

  void updateTick(const TradeHistory* p_full_trade_history, TradeHistory* p_delayed_trade_history);
  void handleTickEvent(const exchange_t a_exchange_id, const CurrencyPair& currency_pair);

  std::vector<TradeHistory*> getPastTradeHistories() const;
  std::vector<const TradeHistory*> getConstPastTradeHistories() const;

  void addIndicators();

  void dumpFinalStats() const;

  bool checkIfCurrentRunEnded() const;

  void executeOrders(const exchange_t a_exchange_id, const CurrencyPair& currency_pair);

  inline VirtualExchange* getVirtualExchange(const exchange_t a_exchange_id) const {
    ASSERT(m_exchanges.find(a_exchange_id) != m_exchanges.end());
    return m_exchanges.at(a_exchange_id)->castVirtualExchange();
  }

  void populateFromJson(const json& a_json);

  void createTradeAlgo(const tradeAlgo_t a_trade_algo_idx);

  void initLogs() const;

  void saveStatsSummaryHeaderInCSV() const;

  void adjustStartTime();

  void saveControllerHeaderToCSV(FILE* a_file) const;
  void saveControllerDataToCSV(FILE* a_file) const;

  void printCurrentStats() const;

  double getAdjustedAmount(order_direction_t direction, const currency_t quote_currency,
                           const double orig_amount) const;

 public:
  Controller(const json& a_json);
  ~Controller();

  void init(const json& a_json);

  exchange_mode_t getMode() {
    return m_mode;
  }

  void run();

  Duration getOrderDelay() const;

  double getCurrentPrice(const exchange_t a_exchange_id, const currency_t a_currency) const;

  order_id_t placeOrder(const exchange_t a_exchange_id, CurrencyPair currency_pair, double amount, order_type_t type,
                        order_direction_t direction, double target_price, const Time time_instant, bool& a_valid_order);
  order_id_t refreshLimitOrder(const exchange_t a_exchange_id, CurrencyPair currency_pair, double amount,
                               order_direction_t direction, double target_price, const Time time_instant,
                               bool& a_valid_order);

  void cancelLimitOrders(const exchange_t a_exchange_id, const CurrencyPair& currency_pair, const bool buy);
  void cancelOrders(const exchange_t a_exchange_id);

  Time getControllerTime(const exchange_t a_exchange_id) const;
  Time getControllerTime() const;

  Time getStartTime() const {
    return m_start_time;
  }
  Time getEndTime() const {
    return m_end_time;
  }
  Duration getHistoryDuration() const {
    return m_history;
  }

  double getPortfolioValue() const;
  double getVirtualPortfolioValue() const;
  double getPortfolioValue(const exchange_t a_exchange_id) const;

  void saveHeaderInCSV(FILE* ap_file, const exchange_t a_exchange_id) const;
  void saveStatsInCSV(FILE* ap_file, const exchange_t a_exchange_id);

  void doRealTimeTrading(const TradeHistory* ap_trade_history);

  const Order* getOrder(const exchange_t a_exchange_id, const order_id_t a_order_id) const;
  const Order* lastCancelledOrder(const exchange_t exchange_id) const;

  Quote getQuote(const exchange_t a_exchange_id, const CurrencyPair& a_currency_pair) const;
  double getBidPrice(const exchange_t a_exchange_id, const CurrencyPair& a_currency_pair) const;
  double getAskPrice(const exchange_t a_exchange_id, const CurrencyPair& a_currency_pair) const;

  void saveStatsSummaryInCSV();

  const Account& getAccount(const exchange_t a_exchange_id, const currency_t a_currency) const;

  const Exchange* getExchange(const exchange_t a_exchange_id) const {
    ASSERT(m_exchanges.find(a_exchange_id) != m_exchanges.end());
    return m_exchanges.at(a_exchange_id);
  }

  void LockTrading();
  void UnlockTrading();

  void logOrderCancellation(const exchange_t a_exchange_id, const order_id_t& order_id) const;

  static void cancelPendingOrdersFromPreviousRun();
};
