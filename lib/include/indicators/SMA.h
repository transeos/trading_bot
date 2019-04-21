//
// Created by subhagato on 4/29/18.
//

#pragma once

#include "DiscreteIndicator.h"
#include <deque>
#include <ta-lib/ta_func.h>

typedef struct sma_t { double sma; } sma_t;

typedef struct TA_SMA_State sma_state_t;

class SMA {
 public:
  std::deque<sma_t> m_values;
  std::shared_ptr<sma_state_t> m_state;
  candle_price_t m_select_price;

  SMA(int period, candle_price_t select_price = candle_price_t::OPEN) : m_select_price(select_price) {
    sma_state_t* state_ptr;
    TA_SMA_StateInit(&state_ptr, period);
    m_state.reset(state_ptr, [](struct TA_SMA_State* ptr) { TA_SMA_StateFree(&ptr); });
  }

  void append(const Candlestick& candle) {
    double out_val;
    TA_SMA_State(m_state.get(), candle.get(m_select_price), &out_val);
    m_values.push_back({out_val});
  }

  static field_t getFields() {
    return {{"SMA-value", "double"}};
  };
};
