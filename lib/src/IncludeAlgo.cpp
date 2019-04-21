// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 21/04/19.
//
//*****************************************************************

#include "Controller.h"
#include "tradeAlgos/MarketOrderAlgo.h"
#include "tradeAlgos/LimitOrderAlgo.h"

void Controller::createTradeAlgo(const tradeAlgo_t a_trade_algo_idx) {
  DELETE(mp_TradeAlgo);

  switch (a_trade_algo_idx) {
    case tradeAlgo_t::BASICMARKET: {
      mp_TradeAlgo = new MarketOrderAlgo();
      return;
    }
    case tradeAlgo_t::BASICLIMIT: {
      mp_TradeAlgo = new LimitOrderAlgo();
      return;
    }
    default:
      assert(0);
  }
}
