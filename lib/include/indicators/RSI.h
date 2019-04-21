//
// Created by subhagato on 3/22/18.
//

#pragma once

#include "DiscreteIndicator.h"
#include <ta-lib/ta_func.h>
#include <tuple>

typedef struct rsi_t { double rsi; } rsi_t;

typedef struct TA_RSI_State rsi_state_t;

class RSI {
 public:
  std::deque<rsi_t> m_values;
  std::shared_ptr<rsi_state_t> m_state;
  candle_price_t m_select_price;

  RSI(int period, candle_price_t select_price = candle_price_t::OPEN) : m_select_price(select_price) {
    rsi_state_t* state_ptr;
    TA_RSI_StateInit(&state_ptr, period);
    m_state.reset(state_ptr, [](struct TA_RSI_State* ptr) { TA_RSI_StateFree(&ptr); });
  }

  void append(const Candlestick& candle) {
    double out_val;
    TA_RSI_State(m_state.get(), candle.get(m_select_price), &out_val);
    m_values.push_back({out_val});
  }

  static field_t getFields() {
    return {{"RSI-value", "double"}};
  };

  friend std::ostream& operator<<(std::ostream& os, const RSI& rsi) {
    for (auto& v : rsi.m_values) {
      os << v.rsi << "\n";
    }
    return os;
  }
};
