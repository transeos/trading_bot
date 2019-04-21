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

#include "tradeAlgos/LimitOrderAlgo.h"
#include "Controller.h"
#include "TraderBot.h"
#include "exchanges/Exchange.h"
#include "triggers/CandleTrigger.h"
#include <float.h>

using namespace std;

LimitOrderAlgo::LimitOrderAlgo() : TradeAlgo(tradeAlgo_t::BASICLIMIT) {
  m_fields = {};

  m_order_price = 0;

  m_intervals.insert(1_min);

  m_triggers.push_back(new CandleTrigger());
}

void LimitOrderAlgo::init(const Time a_cur_time, const vector<const TradeHistory*>& ap_past_trade_histories) {
  TradeAlgo::init(a_cur_time, ap_past_trade_histories);

  m_triggers[0]->init({&TradeAlgo::takeTradeDecision}, a_cur_time, ap_past_trade_histories);
}

void LimitOrderAlgo::takeTradeDecision(const Time a_cur_time, const vector<trade_algo_trigger_t>& a_triggers) {
  Controller* p_controller = TraderBot::getInstance()->getController();
  const TradeHistory* p_trade_history = m_trade_histories[0];

  const Duration& duration = *(m_intervals.begin());
  const CurrencyPair currency_pair = m_trading_pairs[p_trade_history->getExchangeId()][0];

  double cur_price = p_trade_history->getLastCandle(duration, a_cur_time).getMean();
  double prev_price = p_trade_history->getLastCandle(duration, a_cur_time, 1).getMean();

  if (cur_price < prev_price) {
    bool valid_order = false;
    if (p_controller->placeOrder(p_trade_history->getExchangeId(), currency_pair, DBL_MAX, order_type_t::LIMIT,
                                 order_direction_t::BUY, (cur_price * 0.99), a_cur_time, valid_order) != "NO_BALANCE") {
      m_order_price = (cur_price * 0.99);
    }
  } else {
    bool valid_order = false;
    if (p_controller->placeOrder(p_trade_history->getExchangeId(), currency_pair, DBL_MAX, order_type_t::LIMIT,
                                 order_direction_t::SELL, (cur_price * 1.01), a_cur_time,
                                 valid_order) != "NO_BALANCE") {
      m_order_price = (cur_price * 1.01);
    }
  }

  // STOP loss
  if (m_order_price < (cur_price * 0.95)) {
    p_controller->cancelLimitOrders(p_trade_history->getExchangeId(), currency_pair, false);

    bool valid_order = false;
    if (p_controller->placeOrder(p_trade_history->getExchangeId(), currency_pair, DBL_MAX, order_type_t::LIMIT,
                                 order_direction_t::SELL, (cur_price * 1.01), a_cur_time,
                                 valid_order) != "NO_BALANCE") {
      m_order_price = (cur_price * 1.01);
    } else
      p_controller->cancelLimitOrders(p_trade_history->getExchangeId(), currency_pair, true);
  }

  if (m_order_price > (cur_price * 1.05)) {
    p_controller->cancelLimitOrders(p_trade_history->getExchangeId(), currency_pair, true);

    bool valid_order = false;
    if (p_controller->placeOrder(p_trade_history->getExchangeId(), currency_pair, DBL_MAX, order_type_t::LIMIT,
                                 order_direction_t::BUY, (cur_price * 0.99), a_cur_time, valid_order) != "NO_BALANCE") {
      m_order_price = (cur_price * 0.99);
    } else
      p_controller->cancelLimitOrders(p_trade_history->getExchangeId(), currency_pair, false);
  }
}
