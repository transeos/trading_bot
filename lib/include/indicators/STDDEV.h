//
// Created by subhagato on 3/22/18.
//

#pragma once

#include "DiscreteIndicator.h"
#include <deque>
#include <ta-lib/ta_func.h>
#include <tuple>

typedef struct stddev_t { double stddev; } stddev_t;

typedef struct TA_STDDEV_State stddev_state_t;

class STDDEV {
 public:
  std::deque<stddev_t> m_values;
  std::shared_ptr<stddev_state_t> m_state;
  candle_price_t m_select_price;
  static const field_t m_fields;

  STDDEV(int period, candle_price_t select_price = candle_price_t::OPEN) : m_select_price(select_price) {
    stddev_state_t* state_ptr;
    TA_STDDEV_StateInit(&state_ptr, period, 1.0);
    m_state.reset(state_ptr, [](struct TA_STDDEV_State* ptr) { TA_STDDEV_StateFree(&ptr); });
  }

  void append(const Candlestick& candle) {
    double out_val;
    TA_STDDEV_State(m_state.get(), candle.get(m_select_price), &out_val);
    m_values.push_back({out_val});
  }

  static field_t getFields() {
    return {{"STDDEV-value", "double"}};
  };

  friend std::ostream& operator<<(std::ostream& os, const STDDEV& stddev) {
    for (auto& v : stddev.m_values) {
      os << v.stddev << "\n";
    }
    return os;
  }
};