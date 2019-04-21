// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 08/04/18.
//

#include "triggers/TickTrigger.h"
#include "Tick.h"
#include "TickPeriod.h"

using namespace std;

void TickTrigger::init(const vector<AlgoFnPtr>& a_foos, const Time a_cur_time,
                       const vector<const TradeHistory*>& ap_past_trade_histories) {
  Trigger::init(a_foos, a_cur_time, ap_past_trade_histories);

  for (auto p_trade_history : ap_past_trade_histories) {
    const CurrencyPair& currency_pair = p_trade_history->getCurrencyPair();
    const trading_pair_t trading_pair{p_trade_history->getExchangeId(), currency_pair.getBaseCurrency(),
                                      currency_pair.getQuoteCurrency()};

    const TickPeriod* ap_past_trade_period = p_trade_history->getTickPeriod();

    m_prev_num_trades[trading_pair] = ap_past_trade_period->size();
  }
}

bool TickTrigger::checkForEvent(const Time a_cur_time, const vector<const TradeHistory*>& ap_past_trade_histories) {
  bool new_tick = false;

  for (auto p_trade_history : ap_past_trade_histories) {
    const CurrencyPair& currency_pair = p_trade_history->getCurrencyPair();
    const trading_pair_t trading_pair{p_trade_history->getExchangeId(), currency_pair.getBaseCurrency(),
                                      currency_pair.getQuoteCurrency()};

    assert(m_prev_num_trades.find(trading_pair) != m_prev_num_trades.end());

    const TickPeriod* ap_past_trade_period = p_trade_history->getTickPeriod();

    if (m_prev_num_trades[trading_pair] != ap_past_trade_period->size()) {
      m_prev_num_trades[trading_pair] = ap_past_trade_period->size();

      if (m_prev_num_trades[trading_pair] >= 2) new_tick = true;
    }
  }

  return new_tick;
}
