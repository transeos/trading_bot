// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 05/05/18.
//

#include "triggers/IntervalTrigger.h"

using namespace std;

void IntervalTrigger::init(const vector<AlgoFnPtr>& a_foos, const Time a_cur_time,
                           const vector<const TradeHistory*>& ap_past_trade_histories) {
  m_prev_time = a_cur_time;
  Trigger::init(a_foos, a_cur_time, ap_past_trade_histories);
}

bool IntervalTrigger::checkForEvent(const Time a_cur_time, const vector<const TradeHistory*>& ap_past_trade_histories) {
  if (a_cur_time < (m_prev_time + m_interval)) return false;

  m_prev_time += m_interval;

  return true;
}
