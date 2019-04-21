//
// Created by subhagato on 4/28/18.
//

#pragma once

#include <array>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "CandlePeriod.h"
#include "CurrencyPair.h"
#include "Enums.h"
#include "TickPeriod.h"
#include <ta-lib/ta_common.h>
#include <ta-lib/ta_defs.h>
#include <ta-lib/ta_func.h>
#include <utils/dbUtils.h>

#include "EMA.h"
#include "MA.h"
#include "MACD.h"
#include "RSI.h"
#include "SMA.h"
#include "STDDEV.h"
#include "TEMA.h"

#include "utils/TimeUtils.h"

template <typename C = Candlestick, typename T = Tick>
class DiscreteIndicatorA {  // abstract class for pointers
 protected:
  const exchange_t m_exchange_id;
  const CurrencyPair m_currency_pair;
  Duration m_interval;

 public:
  explicit DiscreteIndicatorA(const exchange_t exchange_id, const CurrencyPair& currency_pair, const Duration interval)
      : m_exchange_id(exchange_id), m_currency_pair(currency_pair), m_interval(interval) {}
  virtual ~DiscreteIndicatorA() {}

  exchange_t getExchangeId() const {
    return m_exchange_id;
  }
  const CurrencyPair& getCurrencyPair() const {
    return m_currency_pair;
  }
  Duration getInterval() const {
    return m_interval;
  }

  virtual void append(const C& candle) = 0;
  virtual void constructFrom(const CandlePeriodT<C, T>& candle_period) = 0;
  virtual void saveToCSV(FILE* filep, bool complete_line) = 0;
  virtual void saveHeaderToCSV(FILE* filep, bool complete_line) = 0;
  virtual Time getLastTimeStamp() = 0;
};

template <typename C = Candlestick, typename T = Tick, typename... Is>
class DiscreteIndicatorT : public DiscreteIndicatorA<C> {
 private:
  Time m_last_candle_timestamp;
  size_t m_num_candles;

  std::tuple<Is...> m_indicator_list;

  template <std::size_t I = 0>
  inline typename std::enable_if<I == sizeof...(Is), void>::type individualAppend(const C& candle) {}

  template <std::size_t I = 0>
      inline typename std::enable_if < I<sizeof...(Is), void>::type individualAppend(const C& candle) {
    std::get<I>(m_indicator_list).append(candle);
    individualAppend<I + 1>(candle);
  }

  template <std::size_t I = 0>
  inline typename std::enable_if<I == sizeof...(Is), void>::type individualPrintValue(FILE* filep) {}

  template <std::size_t I = 0>
      inline typename std::enable_if < I<sizeof...(Is), void>::type individualPrintValue(FILE* filep) {
    auto& value = std::get<I>(m_indicator_list).m_values.back();
    const field_t& field = std::get<I>(m_indicator_list).getFields();

    int i = 0;
    for (auto& f : field) {
      fprintf(filep, ",");
      DbUtils::writeInCSV(filep, f.second, &value + i++);
    }
    individualPrintValue<I + 1>(filep);
  }

  template <std::size_t I = 0>
  inline typename std::enable_if<I == sizeof...(Is), void>::type individualPrintHeader(
      FILE* filep, std::unordered_map<std::string, int>& headers) {}

  template <std::size_t I = 0>
      inline typename std::enable_if <
      I<sizeof...(Is), void>::type individualPrintHeader(FILE* filep, std::unordered_map<std::string, int>& headers) {
    const field_t& field = std::get<I>(m_indicator_list).getFields();

    for (auto& f : field) {
      fprintf(filep, ",%s", f.first.c_str());

      // print in case of multiple entry of same indicator
      if (headers.find(f.first) == headers.end())
        headers[f.first] = 1;
      else {
        headers[f.first] = (headers.at(f.first) + 1);
        fprintf(filep, "(%d)", headers.at(f.first));
      }
    }
    individualPrintHeader<I + 1>(filep, headers);
  }

 public:
  explicit DiscreteIndicatorT(const exchange_t exchange_id, const CurrencyPair& currency_pair, Duration interval,
                              Is... args)
      : DiscreteIndicatorA<C>(exchange_id, currency_pair, interval), m_indicator_list{std::move(args)...} {
    TA_RetCode res = TA_Initialize();
    if (res != TA_SUCCESS) CT_CRIT_WARN << "Error TA_Initialize: " << res << std::endl;
    m_num_candles = 0;
  }

  void append(const C& candle) {
    individualAppend(candle);
    m_last_candle_timestamp = candle.getTimeStamp();
    m_num_candles++;
  }

  template <std::size_t I>
  auto getIndicator() -> decltype(std::get<I>(m_indicator_list))& {
    return std::get<I>(m_indicator_list);
  }

  template <std::size_t I>
  auto getIndicatorValues() -> decltype(std::get<I>(m_indicator_list).m_values)& {
    return std::get<I>(m_indicator_list).m_values;
  }

  template <std::size_t I>
  auto getIndicatorAtIdx(size_t idx = UINT64_MAX) ->
      typename decltype(std::get<I>(m_indicator_list).m_values)::value_type {
    if (idx == UINT64_MAX) idx = m_num_candles - 1;
    return std::get<I>(m_indicator_list).m_values[idx];
  }

  template <std::size_t I>
  auto getIndicatorAtTime(Time timestamp = Time(0)) ->
      typename decltype(std::get<I>(m_indicator_list).m_values)::value_type {
    size_t idx;

    if (timestamp == Time(0))
      idx = m_num_candles - 1;
    else
      idx = m_num_candles - (m_last_candle_timestamp - timestamp.quantize(this->m_interval)) / this->m_interval - 1;

    return std::get<I>(m_indicator_list).m_values[idx];
  }

  Time getLastTimeStamp() {
    return m_last_candle_timestamp;
  }

  size_t getNumCandles() const {
    return m_num_candles;
  }

  void saveToCSV(FILE* filep, const bool complete_line) {
    if (complete_line) {
      DbUtils::writeInCSV(filep, m_last_candle_timestamp, true);
    }

    individualPrintValue(filep);

    if (complete_line) fprintf(filep, "\n");
  }

  void saveHeaderToCSV(FILE* filep, const bool complete_line) {
    if (complete_line) fprintf(filep, "Time");

    std::unordered_map<std::string, int> headers;
    individualPrintHeader(filep, headers);

    if (complete_line) fprintf(filep, "\n");
  }

  void constructFrom(const CandlePeriodT<C, T>& candle_period) {
    for (auto& candle : candle_period) {
      append(candle);
    }
  }

  ~DiscreteIndicatorT() {
    TA_RetCode res = TA_Shutdown();
    if (res != TA_SUCCESS) {
      CT_CRIT_WARN << "Error TA_Shutdown: " << res << std::endl;
    }
  }
};

template <typename... Is>
using DiscreteIndicator = DiscreteIndicatorT<Candlestick, Tick, Is...>;
