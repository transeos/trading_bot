//
// Created by Subhagato Dutta on 11/12/17.
//

#include "Candlestick.h"
#include "utils/TraderUtils.h"
#include <numeric>

using namespace std;

const pair<string, string> Candlestick::m_unique_id = {"", ""};
const field_t Candlestick::m_fields = {{"open", "double"},       {"close", "double"},      {"low", "double"},
                                       {"high", "double"},       {"mean", "double"},       {"stddev", "double"},
                                       {"buy_volume", "double"}, {"sell_volume", "double"}};
const string Candlestick::m_type = "Candlestick";

void* Candlestick::getMemberPtr(size_t i) {
  switch (i) {
    case 0:
      return &(m_open);
    case 1:
      return &(m_close);
    case 2:
      return &(m_low);
    case 3:
      return &(m_high);
    case 4:
      return &(m_mean);
    case 5:
      return &(m_stddev);
    case 6:
      return &(m_volume.getBuyVolume());
    case 7:
      return &(m_volume.getSellVolume());
    default:
      return nullptr;
  }
}

void Candlestick::setMemberPtr(void* value, size_t i) {
  switch (i) {
    case 0:
      m_open = *(double*)value;
    case 1:
      m_close = *(double*)value;
    case 2:
      m_low = *(double*)value;
    case 3:
      m_high = *(double*)value;
    case 4:
      m_mean = *(double*)value;
    case 5:
      m_stddev = *(double*)value;
    case 6:
      m_volume = *(double*)value;
    default:
      assert(0);
  }
}

Candlestick::Candlestick(const Time start_time, const vector<double>& price_arr, vector<Volume>& volume_arr)
    : Candlestick() {
  assert(price_arr.size() == volume_arr.size());

  m_timestamp = start_time;

  if (price_arr.empty()) return;

  m_open = price_arr.front();
  m_close = price_arr.back();
  m_volume = accumulate(volume_arr.begin(), volume_arr.end(), Volume(0.0));

  if (m_volume == 0) fill(volume_arr.begin(), volume_arr.end(), Volume(1.0));

  auto result = minmax_element(price_arr.begin(), price_arr.end());

  m_low = *result.first;
  m_high = *result.second;

  m_mean = 0.0;

  for (size_t i = 0; i < price_arr.size(); i++) m_mean += price_arr[i] * static_cast<double>(volume_arr[i]);

  m_mean /= static_cast<double>(m_volume);

  vector<double> volume_wt_arr;

  for (auto v : volume_arr) volume_wt_arr.push_back(static_cast<double>(v));

  m_stddev = sqrt(TradeUtils::weighted_incremental_variance(price_arr, volume_wt_arr));
}
