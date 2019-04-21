//
// Created by subhagato on 4/29/18.
//

#pragma once

#include "DiscreteIndicator.h"
#include <deque>
#include <ta-lib/ta_func.h>

typedef struct tema_t { double tema; } tema_t;

typedef struct TA_TEMA_State tema_state_t;

class TEMA {
 public:
  std::deque<tema_t> m_values;
  std::shared_ptr<tema_state_t> m_state;
  candle_price_t m_select_price;

  TEMA(int period, candle_price_t select_price = candle_price_t::OPEN) : m_select_price(select_price) {
    tema_state_t* state_ptr;
    TA_TEMA_StateInit(&state_ptr, period);
    m_state.reset(state_ptr, [](tema_state_t* ptr) { TA_TEMA_StateFree(&ptr); });
  }

  void append(const Candlestick& candle) {
    double out_val;
    TA_TEMA_State(m_state.get(), candle.get(m_select_price), &out_val);
    m_values.push_back({out_val});
  }

  static field_t getFields() {
    return {{"TEMA-value", "double"}};
  };
};
