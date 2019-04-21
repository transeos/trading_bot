// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 08/04/18.
//

#ifndef TICK_TRIGGER_H
#define TICK_TRIGGER_H

#include "triggers/Trigger.h"

class TickTrigger : public Trigger {
 private:
  std::unordered_map<trading_pair_t, uint32_t> m_prev_num_trades;

 public:
  TickTrigger() : Trigger(trade_algo_trigger_t::NEW_TICK) {}
  virtual ~TickTrigger() {}

  // virtual
  virtual void init(const std::vector<AlgoFnPtr>& a_foos, const Time a_cur_time,
                    const std::vector<const TradeHistory*>& ap_past_trade_histories);

  // pure virtual
  virtual bool checkForEvent(const Time a_cur_time, const std::vector<const TradeHistory*>& ap_past_trade_histories);
};

#endif  // TICK_TRIGGER_H
