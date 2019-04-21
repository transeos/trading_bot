// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 01/01/18
//
//*****************************************************************

#ifndef TRADEALGO_H
#define TRADEALGO_H

#include "exchanges/Exchange.h"
#include "indicators/DiscreteIndicator.h"
#include <iostream>
#include <unordered_set>
#include <vector>

class Trigger;
class TradeAlgo;

typedef void (TradeAlgo::*AlgoFnPtr)(const Time, const std::vector<trade_algo_trigger_t>&);

class TradeAlgo {
 private:
  tradeAlgo_t m_algo_idx;

  // run time related data-structures
  int m_num_event_checks;
  int m_num_events;

  Duration m_avg_event_check_time;
  Duration m_avg_event_time;

  Duration m_max_event_check_time;
  Duration m_max_event_time;

 protected:
  Time m_current_time;

  std::unordered_map<exchange_t, std::vector<CurrencyPair>> m_trading_pairs;

  std::vector<Trigger*> m_triggers;

  std::vector<const TradeHistory*> m_trade_histories;

  // intervals and indicators need to be initialized in the constructor of respective derived classes
  std::set<Duration> m_intervals;
  std::unordered_map<exchange_t, std::unordered_map<Duration, std::unordered_map<CurrencyPair, DiscreteIndicatorA<>*>>>
      m_indicator_map;

  // past orders
  std::vector<order_id_t> m_past_orders;

  // algo states
  std::vector<std::pair<std::string, std::string>> m_fields;

  void initTrigger(const std::vector<AlgoFnPtr>& a_foos, Trigger* ap_trigger, Time a_cur_time);

  void registerIndicator(DiscreteIndicatorA<>* ap_indicator);

 public:
  explicit TradeAlgo(tradeAlgo_t a_tradeAlgo_idx);
  virtual ~TradeAlgo();

  void checkForEvent(Time a_cur_time, bool a_interval_event = false);

  inline const std::set<Duration>& getCandleStickDuration() const {
    return m_intervals;
  }

  inline DiscreteIndicatorA<>* getIndicators(const exchange_t exchange_id, Duration interval,
                                             const CurrencyPair& currency_pair) {
    return m_indicator_map[exchange_id][interval][currency_pair];
  }

  void populateOrder(const order_id_t a_order_id) {
    m_past_orders.push_back(a_order_id);
  }
  order_id_t getLastOrder() const {
    return (m_past_orders.size() ? m_past_orders.back() : "");
  }

  Duration getMinInterval() const;

  void saveHeaderToCSV(FILE* ap_file) const;
  void saveStateToCSV(FILE* ap_file);
  void printStats() const;

  void dumpRunTimeStats() const;

  // virtual
  virtual void init(Time a_cur_time, const std::vector<const TradeHistory*>& ap_past_trade_histories);

  // virtual with assert(0)
  virtual void placeOrders(Time a_cur_time, const std::vector<trade_algo_trigger_t>& a_triggers) {
    assert(0);
  }

  // pure virtual
  virtual void takeTradeDecision(Time a_cur_time, const std::vector<trade_algo_trigger_t>& a_triggers) = 0;

  virtual void updateIndicator(const exchange_t exchange_id, const CurrencyPair& currency_pair) = 0;

  virtual const void* getMemberPtr(int a_field_idx) const = 0;
};

namespace std {
template <>
struct hash<tradeAlgo_t> {
  std::size_t operator()(const tradeAlgo_t& k) const {
    using std::hash;

    // Compute  hash values for tradeAlgo_t as integer:

    return (hash<int>()((int)k));
  }
};
}  // namespace std

#endif  // TRADEALGO_H
