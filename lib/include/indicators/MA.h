//
// Created by subhagato on 1/20/18.
//

#pragma once

#include "DiscreteIndicator.h"
#include <queue>

typedef struct ma_t { double ma; } ma_t;

class MA {
 public:
  std::deque<ma_t> m_values;

  MA(int period, bool volume_weighted, candle_price_t select_price = candle_price_t::OPEN)
      : m_select_price(select_price), m_time_period(period), m_volume_weighted(volume_weighted) {
    m_total_volume = 0.0;
    m_moving_average = 0.0;
  }

  static field_t getFields() {
    return {{"MA-value", "double"}};
  };

  void append(const Candlestick& candle);

  friend std::ostream& operator<<(std::ostream& os, const MA& ma) {
    for (auto& v : ma.m_values) {
      os << std::setprecision(8) << v.ma << "\n";
    }
    return os;
  }

 private:
  std::queue<std::tuple<double, double>> m_history;
  double m_total_volume;
  double m_moving_average;
  candle_price_t m_select_price;
  uint32_t m_time_period;
  bool m_volume_weighted;
};
