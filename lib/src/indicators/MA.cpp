//
// Created by subhagato on 1/20/18.
//

#include "indicators/MA.h"

using namespace std;

void MA::append(const Candlestick& candle) {
  double volume = candle.getTotalVolume();
  double price = candle.get(m_select_price);
  double edge_price = 0;
  double edge_volume = 0;

  volume = max(1e-9, volume);

  if (m_history.size() == 0)  // build up the history with same values
  {
    for (uint32_t i = 0; i < m_time_period; i++) {
      m_history.push(make_tuple(price, volume));
      m_total_volume += volume;
    }
    m_moving_average = price;
  }

  m_history.push(make_tuple(price, volume));

  edge_price = get<0>(m_history.front());
  edge_volume = get<1>(m_history.front());

  if (m_volume_weighted) {
    m_moving_average = (m_moving_average * m_total_volume + price * volume - edge_price * edge_volume) /
                       (m_total_volume + volume - edge_volume);
    m_total_volume += (volume - edge_volume);
  } else {
    m_moving_average = (m_moving_average * m_time_period + price - edge_price) / m_time_period;
  }

  if (m_history.size() == (m_time_period + 1)) m_history.pop();

  m_values.push_back({m_moving_average});
}
