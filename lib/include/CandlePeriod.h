//
// Created by subhagato on 11/18/17.
//

#ifndef CRYPTOTRADER_CANDLEPERIOD_H
#define CRYPTOTRADER_CANDLEPERIOD_H

#include "CMCandleStick.h"
#include "Candlestick.h"
#include <deque>
#include <iostream>

class Tick;
class CoinAPITick;
template <typename T>
class TickPeriodT;
template <typename T>
class Database;

template <typename C, typename T>
class CandlePeriodT : public std::deque<C> {
 private:
  Time m_first_ts;
  Time m_last_ts;
  Duration m_interval;
  bool m_stored;
  size_t m_non_zero_sample_count;

  double m_last_price;

 public:
  CandlePeriodT() {
    m_first_ts = 0;
    m_last_ts = 0;
    m_interval = 0;
    m_non_zero_sample_count = 0;
    m_stored = false;
  };

  CandlePeriodT(Duration interval) {
    m_first_ts = 0;
    m_last_ts = 0;
    m_interval = interval;
    m_non_zero_sample_count = 0;
    m_stored = false;
  };

  CandlePeriodT(TickPeriodT<T>& trade_period, Duration interval);
  CandlePeriodT(std::deque<C>& candlesticks, Duration interval);

  void clear();

  bool convertFrom(TickPeriodT<T>& trade_period);

  int appendTick(const T& t);

  Time getFirstTimestamp() const {
    return m_first_ts;
  }

  Time getLastTimestamp() const {
    return m_last_ts;
  }

  int64_t getFirstUniqueId() const {
    return m_first_ts / m_interval;
  }

  int64_t getLastUniqueId() const {
    return m_last_ts / m_interval;
  }

  Duration getInterval() const {
    return m_interval;
  }

  void setInterval(Duration interval) {
    m_interval = interval;
  }

  size_t getNonZeroSampleCount() const {
    return m_non_zero_sample_count;
  }

  void setNonZeroSampleCount(size_t a_non_zero_sample_count) {
    m_non_zero_sample_count = a_non_zero_sample_count;
  }

  double getLastPrice() const {
    return m_last_price;
  }
  void setLastPrice(const double last_price) {
    m_last_price = last_price;
  }

  int append(const C& cs);

  int appendSmallerCandle(const C& cs, Duration interval);

  int storeToDatabase(Database<C>* db);

  bool convertFrom(CandlePeriodT<C, T>& candle_period);

  bool loadFromDatabase(Database<C>* db, Time start_time, Time end_time);

  void saveToCSV(FILE* a_csv_file, const bool complete_line = true);
  void saveHeaderToCSV(FILE* a_csv_file, const bool complete_line = true);

  friend bool operator==(const CandlePeriodT<C, T>& lhs, const CandlePeriodT<C, T>& rhs) {
    return (lhs.m_first_ts == rhs.m_first_ts) && (lhs.m_last_ts == rhs.m_last_ts);
  }

  friend bool operator!=(const CandlePeriodT<C, T>& lhs, const CandlePeriodT<C, T>& rhs) {
    return !(lhs == rhs);
  }

  friend bool operator>(const CandlePeriodT<C, T>& lhs, const CandlePeriodT<C, T>& rhs) {
    return (lhs.m_first_ts > rhs.m_last_ts);
  }

  friend bool operator<(const CandlePeriodT<C, T>& lhs, const CandlePeriodT<C, T>& rhs) {
    return (lhs.m_last_ts < rhs.m_first_ts);
  }

  friend std::ostream& operator<<(std::ostream& os, const CandlePeriodT<C, T>& candleperiod) {
    for (auto& candle : candleperiod) os << candle << "\n";
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }

  CandlePeriodT<C, T>& operator=(TickPeriodT<T>& trade_period);

  //    friend bool operator >=(const CandlePeriodT<C,T>& lhs, const CandlePeriodT<C,T>& rhs) {
  //        return (lhs.m_first_tid > rhs.m_first_tid) && (lhs.m_first_tid <= rhs.m_last_tid);
  //    }
  //    friend bool operator <=(const CandlePeriodT<C,T>& lhs, const CandlePeriodT<C,T>& rhs) {
  //        return (lhs.m_last_tid < rhs.m_last_tid) && (lhs.m_last_tid >= rhs.m_first_tid);
  //    }
};

typedef CandlePeriodT<Candlestick, Tick> CandlePeriod;
typedef CandlePeriodT<CMCandleStick, Tick> CMCCandlePeriod;
typedef CandlePeriodT<Candlestick, CoinAPITick> CapiCandlePeriod;

#endif  // CRYPTOTRADER_CANDLEPERIOD_H
