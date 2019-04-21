// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 20/01/18
//
//*****************************************************************

#ifndef MARKET_ORDER_ALGO_H
#define MARKET_ORDER_ALGO_H

#include "tradeAlgos/TradeAlgo.h"

class MarketOrderAlgo : public TradeAlgo {
 private:
  int m_status;

  double m_slope_high;
  double m_slope_low;

 public:
  MarketOrderAlgo();
  ~MarketOrderAlgo() {}

  // virtual
  virtual void init(const Time a_cur_time, const std::vector<const TradeHistory*>& ap_past_trade_histories);

  // pure virtual
  virtual void takeTradeDecision(const Time a_cur_time, const std::vector<trade_algo_trigger_t>& a_triggers);

  virtual void updateIndicator(const exchange_t exchange_id, const CurrencyPair& currency_pair);

  virtual const void* getMemberPtr(const int a_field_idx) const;
};

#endif  // MARKET_ORDER_ALGO_H
