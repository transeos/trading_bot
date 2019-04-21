//
// Created by subhagato on 11/18/17.
//

#ifndef CRYPTOTRADER_TRADEPERIOD_H
#define CRYPTOTRADER_TRADEPERIOD_H

#include "utils/TimeUtils.h"
#include <deque>
#include <iostream>

class Tick;
class CoinAPITick;
class CMCandleStick;
template <typename T>
class Database;

template <typename T>
class TickPeriodT : public std::deque<T> {
 private:
  Time m_first_ts;
  Time m_last_ts;
  int64_t m_first_tid;
  int64_t m_last_tid;
  bool m_consecutive;
  typename std::deque<T>::iterator m_first_stored_itr;
  typename std::deque<T>::iterator m_last_stored_itr;
  int m_num_saved = 0;

  void reset();

 public:
  TickPeriodT(bool consecutive = true);

  TickPeriodT(int64_t dummy_tid, bool consecutive = true) : TickPeriodT(consecutive) {
    m_first_tid = dummy_tid;
    m_last_tid = dummy_tid;
  }

  Time getFirstTimestamp() const {
    return m_first_ts;
  }

  Time getLastTimestamp() const {
    return m_last_ts;
  }

  int64_t getFirstUniqueId() const {
    return m_first_tid;
  }

  int64_t getLastUniqueId() const {
    return m_last_tid;
  }

  typename std::deque<T>::iterator getFirstStoredItr() const {
    return m_first_stored_itr;
  }

  typename std::deque<T>::iterator getLastStoredItr() const {
    return m_last_stored_itr;
  }

  int append(T& t, FILE* ap_csv_file = NULL);

  int storeToDatabase(Database<T>* db);

  bool loadFromDatabase(Database<T>* db, Time start_time, Time end_time);

  void saveToCSV(FILE* a_csv_file);

  void clear();

  friend bool operator==(const TickPeriodT<T>& lhs, const TickPeriodT<T>& rhs) {
    return (lhs.m_first_tid == rhs.m_first_tid) && (lhs.m_last_tid == rhs.m_last_tid);
  }

  friend bool operator!=(const TickPeriodT<T>& lhs, const TickPeriodT<T>& rhs) {
    return !(lhs == rhs);
  }

  friend bool operator>(const TickPeriodT<T>& lhs, const TickPeriodT<T>& rhs) {
    return (lhs.m_first_tid > rhs.m_last_tid);
  }

  friend bool operator<(const TickPeriodT<T>& lhs, const TickPeriodT<T>& rhs) {
    return (lhs.m_last_tid < rhs.m_first_tid);
  }

  friend std::ostream& operator<<(std::ostream& os, const TickPeriodT<T>& tickperiod) {
    for (auto& tick : tickperiod) {
      os << tick << "\n";
    }
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }

  static void sFixTimestamps(std::deque<T>& trades);

  //    friend bool operator >=(const TickPeriodT<T>& lhs, const TickPeriodT<T>& rhs) {
  //        return (lhs.m_first_tid > rhs.m_first_tid) && (lhs.m_first_tid <= rhs.m_last_tid);
  //    }
  //    friend bool operator <=(const TickPeriodT<T>& lhs, const TickPeriodT<T>& rhs) {
  //        return (lhs.m_last_tid < rhs.m_last_tid) && (lhs.m_last_tid >= rhs.m_first_tid);
  //    }
};

typedef TickPeriodT<Tick> TickPeriod;
typedef TickPeriodT<CMCandleStick> CMCTickPeriod;
typedef TickPeriodT<CoinAPITick> CapiTickPeriod;

#endif  // CRYPTOTRADER_TRADEPERIOD_H
