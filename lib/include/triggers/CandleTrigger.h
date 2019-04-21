// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 08/04/18.
//

#ifndef CANDLE_TRIGGER_H
#define CANDLE_TRIGGER_H

#include "triggers/Trigger.h"

class CandleTrigger : public Trigger {
 private:
  std::unordered_map<trading_pair_t, std::map<Duration, int>> m_prev_num_candle_sticks;

 public:
  CandleTrigger() : Trigger(trade_algo_trigger_t::NEW_CANDLESTICK) {}
  virtual ~CandleTrigger() {}

  // virtual
  virtual void init(const std::vector<AlgoFnPtr>& a_foos, const Time a_cur_time,
                    const std::vector<const TradeHistory*>& ap_past_trade_histories);

  // pure virtual
  virtual bool checkForEvent(const Time a_cur_time, const std::vector<const TradeHistory*>& ap_past_trade_histories);
};

#endif  // CANDLE_TRIGGER_H
