//
// Created by subhagato on 3/22/18.
//

#pragma once

#include "DiscreteIndicator.h"
#include <ta-lib/ta_func.h>
#include <tuple>

typedef struct ema_t { double ema; } ema_t;

typedef struct TA_EMA_State ema_state_t;

class EMA {
 public:
  std::deque<ema_t> m_values;
  std::shared_ptr<ema_state_t> m_state;
  candle_price_t m_select_price;

  EMA(int period, candle_price_t select_price = candle_price_t::OPEN) : m_select_price(select_price) {
    ema_state_t* state_ptr;
    TA_EMA_StateInit(&state_ptr, period);
    m_state.reset(state_ptr, [](struct TA_EMA_State* ptr) { TA_EMA_StateFree(&ptr); });
  }

  void append(const Candlestick& candle) {
    double out_val;
    TA_EMA_State(m_state.get(), candle.get(m_select_price), &out_val);
    m_values.push_back({out_val});
  }

  static field_t getFields() {
    return {{"EMA-value", "double"}};
  };

  friend std::ostream& operator<<(std::ostream& os, const EMA& ema) {
    for (auto& v : ema.m_values) {
      os << v.ema << "\n";
    }
    return os;
  }
};

// const field_t EMA::m_fields = {{"EMA-value", "double"}};