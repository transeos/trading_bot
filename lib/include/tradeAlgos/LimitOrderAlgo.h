// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 21/01/18
//
//*****************************************************************

#ifndef LIMIT_ORDER_ALGO_H
#define LIMIT_ORDER_ALGO_H

#include "tradeAlgos/TradeAlgo.h"

class LimitOrderAlgo : public TradeAlgo {
 private:
  double m_order_price;

 public:
  LimitOrderAlgo();
  ~LimitOrderAlgo() {}

  // virtual
  virtual void init(const Time a_cur_time, const std::vector<const TradeHistory*>& ap_past_trade_histories);

  // pure virtual
  virtual void takeTradeDecision(const Time a_cur_time, const std::vector<trade_algo_trigger_t>& a_triggers);

  virtual void updateIndicator(const exchange_t exchange_id, const CurrencyPair& currency_pair) {}

  virtual const void* getMemberPtr(const int a_field_idx) const {
    return NULL;
  }
};

#endif  // LIMIT_ORDER_ALGO_H
