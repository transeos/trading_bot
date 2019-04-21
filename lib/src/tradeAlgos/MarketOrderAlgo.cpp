// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 01/01/18
//
//*****************************************************************

#include "tradeAlgos/MarketOrderAlgo.h"
#include "Controller.h"
#include "TraderBot.h"
#include "exchanges/Exchange.h"
#include "indicators/MA.h"
#include "triggers/CandleTrigger.h"
#include <cfloat>

using namespace std;

MarketOrderAlgo::MarketOrderAlgo() : TradeAlgo(tradeAlgo_t::BASICMARKET) {
  m_status = 0;
  m_slope_high = 0;
  m_slope_low = 0;

  // multiple intervals have been added for testing.
  m_intervals.insert(1_min);
  m_intervals.insert(2_min);
}

void MarketOrderAlgo::updateIndicator(const exchange_t exchange_id, const CurrencyPair& currency_pair) {
  auto indicator = new DiscreteIndicator<MA>(exchange_id, currency_pair, 1_min, MA(5, true, candle_price_t::MEAN));
  registerIndicator(indicator);
}

void MarketOrderAlgo::init(const Time a_cur_time, const vector<const TradeHistory*>& ap_past_trade_histories) {
  TradeAlgo::init(a_cur_time, ap_past_trade_histories);

  // Triggers have been added multiple times for testing.
  initTrigger({&TradeAlgo::takeTradeDecision}, new CandleTrigger(), a_cur_time);
  initTrigger({&TradeAlgo::takeTradeDecision}, new CandleTrigger(), a_cur_time);
}

void MarketOrderAlgo::takeTradeDecision(const Time a_cur_time, const vector<trade_algo_trigger_t>& a_triggers) {
  assert((a_triggers.size() == 1) && (a_triggers[0] == trade_algo_trigger_t::NEW_CANDLESTICK));

  Controller* p_controller = TraderBot::getInstance()->getController();
  const TradeHistory* p_trade_history = m_trade_histories[0];
  const CurrencyPair currency_pair = m_trading_pairs[p_trade_history->getExchangeId()][0];

  const fee_t fee = TraderBot::getInstance()->getExchange(p_trade_history->getExchangeId())->getFee();

  double cur_price = p_trade_history->getLastCandle(*(m_intervals.begin()), a_cur_time, 1).getMean();

  if (m_status == 0) {
    double prev_price = p_trade_history->getLastCandle(*(m_intervals.begin()), a_cur_time, 2).getMean();

    if (cur_price > prev_price) {
      m_status = 1;
      m_slope_high = cur_price;
      m_slope_low = prev_price;

      bool valid_order = false;
      order_id_t order_id =
          p_controller->placeOrder(p_trade_history->getExchangeId(), currency_pair, DBL_MAX, order_type_t::MARKET,
                                   order_direction_t::BUY, -1, a_cur_time, valid_order);

      if (order_id == "NO_BALANCE") {
        CT_CRIT_WARN << "not enough " << Currency::sCurrencyToString(currency_pair.getQuoteCurrency()) << " balance\n";
      }
    } else {
      m_status = -1;
      m_slope_high = prev_price;
      m_slope_low = cur_price;

      bool valid_order = false;
      order_id_t order_id =
          p_controller->placeOrder(p_trade_history->getExchangeId(), currency_pair, DBL_MAX, order_type_t::MARKET,
                                   order_direction_t::SELL, -1, a_cur_time, valid_order);

      if (order_id == "NO_BALANCE") {
        CT_CRIT_WARN << "not enough " << Currency::sCurrencyToString(currency_pair.getBaseCurrency()) << " balance\n";
      }
    }

    return;
  }

  assert(m_slope_high >= m_slope_low);

  if (m_status > 0) {
    bool sell = false;
    bool stop_loss = false;

    if (m_slope_high <= cur_price)
      m_slope_high = cur_price;
    else if (cur_price < (m_slope_low * 0.95)) {
      sell = true;
      stop_loss = true;
    } else if (m_slope_high > (m_slope_low * (1 + 4 * fee.taker_fee)))
      sell = true;

    if (sell) {
      m_slope_high = cur_price;
      m_slope_low = cur_price;
      m_status = -1;

      if (stop_loss) {
        COUT << CRED << "STOP LOSS : ";
      }

      bool valid_order = false;
      order_id_t order_id =
          p_controller->placeOrder(p_trade_history->getExchangeId(), currency_pair, DBL_MAX, order_type_t::MARKET,
                                   order_direction_t::SELL, -1, a_cur_time, valid_order);

      if (order_id == "NO_BALANCE") {
        CT_CRIT_WARN << "not enough " << Currency::sCurrencyToString(currency_pair.getBaseCurrency()) << " balance\n";
      }
    }
    assert(m_slope_high >= m_slope_low);
  } else {
    bool buy = false;
    bool stop_loss = false;

    if (m_slope_low >= cur_price)
      m_slope_low = cur_price;
    else if (cur_price > (m_slope_high * 0.95)) {
      buy = true;
      stop_loss = true;
    } else if (m_slope_high > (m_slope_low * (1 + 4 * fee.taker_fee)))
      buy = true;

    if (buy) {
      m_slope_high = cur_price;
      m_slope_low = cur_price;
      m_status = 1;

      if (stop_loss) {
        COUT << CRED << "STOP LOSS : ";
      }

      bool valid_order = false;
      order_id_t order_id =
          p_controller->placeOrder(p_trade_history->getExchangeId(), currency_pair, DBL_MAX, order_type_t::MARKET,
                                   order_direction_t::BUY, -1, a_cur_time, valid_order);

      if (order_id == "NO_BALANCE") {
        CT_CRIT_WARN << "not enough " << Currency::sCurrencyToString(currency_pair.getQuoteCurrency()) << " balance\n";
      }
    }
    assert(m_slope_high >= m_slope_low);
  }
}

const void* MarketOrderAlgo::getMemberPtr(const int a_field_idx) const {
  switch (a_field_idx) {
    case 0:
      return &m_status;
    case 1:
      switch (m_status) {
        case 0:
          return "Initial";
        case 1:
          return "Rising";
        case -1:
          return "Falling";
        default:
          assert(0);
      }
    default:
      return NULL;
  }
}
