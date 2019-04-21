// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 08/04/18.
//

#include "triggers/CandleTrigger.h"
#include "CandlePeriod.h"
#include "Candlestick.h"

using namespace std;

void CandleTrigger::init(const vector<AlgoFnPtr>& a_foos, const Time a_cur_time,
                         const vector<const TradeHistory*>& ap_past_trade_histories) {
  Trigger::init(a_foos, a_cur_time, ap_past_trade_histories);

  for (auto p_trade_history : ap_past_trade_histories) {
    const CurrencyPair& currency_pair = p_trade_history->getCurrencyPair();
    const trading_pair_t trading_pair{p_trade_history->getExchangeId(), currency_pair.getBaseCurrency(),
                                      currency_pair.getQuoteCurrency()};

    m_prev_num_candle_sticks[trading_pair] = {};

    map<Duration, int> nunCandlesHash;
    p_trade_history->populateNumCandles(nunCandlesHash);
    for (auto iter : nunCandlesHash) {
      Duration duration = iter.first;
      m_prev_num_candle_sticks[trading_pair][duration] = iter.second;
    }
  }
}

bool CandleTrigger::checkForEvent(const Time a_cur_time, const vector<const TradeHistory*>& ap_past_trade_histories) {
  bool new_candle = false;

  for (auto p_trade_history : ap_past_trade_histories) {
    const CurrencyPair& currency_pair = p_trade_history->getCurrencyPair();
    const trading_pair_t trading_pair{p_trade_history->getExchangeId(), currency_pair.getBaseCurrency(),
                                      currency_pair.getQuoteCurrency()};

    assert(m_prev_num_candle_sticks.find(trading_pair) != m_prev_num_candle_sticks.end());
    map<Duration, int>& candle_sticks = m_prev_num_candle_sticks[trading_pair];

    map<Duration, int> nunCandlesHash;
    p_trade_history->populateNumCandles(nunCandlesHash);
    for (auto iter : nunCandlesHash) {
      Duration duration = iter.first;
      int numCandle = iter.second;

      assert(candle_sticks.find(duration) != candle_sticks.end());

      if (numCandle != candle_sticks[duration]) {
        ASSERT(numCandle > candle_sticks[duration]);

        candle_sticks[duration] = numCandle;

        if (candle_sticks[duration] < 2) continue;

        new_candle = true;
      }
    }
  }

  return new_candle;
}
