// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 05/05/18.
//

#pragma once

#include "triggers/Trigger.h"

class IntervalTrigger : public Trigger {
 private:
  Duration m_interval;
  Time m_prev_time;

 public:
  IntervalTrigger(const Duration a_interval) : Trigger(trade_algo_trigger_t::NEW_INTERVAL), m_interval(a_interval) {}
  virtual ~IntervalTrigger() {}

  // virtual
  virtual void init(const std::vector<AlgoFnPtr>& a_foos, const Time a_cur_time,
                    const std::vector<const TradeHistory*>& ap_past_trade_histories);

  virtual Duration getInterval() const {
    return m_interval;
  }

  // pure virtual
  virtual bool checkForEvent(const Time a_cur_time, const std::vector<const TradeHistory*>& ap_past_trade_histories);
};
