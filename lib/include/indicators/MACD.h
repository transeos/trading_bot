//
// Created by subhagato on 3/22/18.
//

#pragma once

#include "DiscreteIndicator.h"
#include <Candlestick.h>
#include <Enums.h>
#include <deque>
#include <memory>
#include <ta-lib/ta_func.h>

typedef struct macd_t {
  double macd;
  double macd_signal;
  double macd_hist;
} macd_t;

typedef struct TA_MACD_State macd_state_t;

class MACD {
 public:
  std::deque<macd_t> m_values;
  std::shared_ptr<macd_state_t> m_state;
  candle_price_t m_select_price;

  MACD(int fast_period, int slow_period, int signal_period, candle_price_t select_price = candle_price_t::OPEN)
      : m_select_price(select_price) {
    macd_state_t* state_ptr;
    TA_MACD_StateInit(&state_ptr, fast_period, slow_period, signal_period);
    m_state.reset(state_ptr, [](macd_state_t* ptr) { TA_MACD_StateFree(&ptr); });
  }

  void append(const Candlestick& candle) {
    double macd, macd_signal, macd_hist;
    TA_MACD_State(m_state.get(), candle.get(m_select_price), &macd, &macd_signal, &macd_hist);
    m_values.push_back({macd, macd_signal, macd_hist});
  }

  static field_t getFields() {
    return {{"MACD-value", "double"}, {"MACD-signal", "double"}, {"MACD-hist", "double"}};
  };
};