// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 08/04/18.
//

#ifndef TRIGGER_H
#define TRIGGER_H

#include "TradeHistory.h"
#include "tradeAlgos/TradeAlgo.h"
#include <iostream>
#include <vector>

class Trigger {
 private:
  trade_algo_trigger_t m_trigger;

 protected:
  std::vector<AlgoFnPtr> m_foos;

 public:
  Trigger(const trade_algo_trigger_t a_trigger) : m_trigger(a_trigger) {}
  virtual ~Trigger() {}

  trade_algo_trigger_t getTriggerType() const {
    return m_trigger;
  }

  const std::vector<AlgoFnPtr>& getFoos() const {
    return m_foos;
  }

  // virtual
  virtual void init(const std::vector<AlgoFnPtr>& a_foos, const Time a_cur_time,
                    const std::vector<const TradeHistory*>& ap_past_trade_histories) {
    m_foos = a_foos;
  }

  virtual Duration getInterval() const {
    assert(0);
  }

  // pure virtual
  virtual bool checkForEvent(const Time a_cur_time,
                             const std::vector<const TradeHistory*>& ap_past_trade_histories) = 0;
};

namespace std {
template <>
struct hash<trade_algo_trigger_t> {
  std::size_t operator()(const trade_algo_trigger_t& k) const {
    using std::hash;

    // Compute  hash values for trade_algo_trigger_t as integer:

    return (hash<int>()((int)k));
  }
};
}

#endif  // TRIGGER_H
