//
// Created by subhagato on 11/2/17.
//

#ifndef CRYPTOTRADER_TICK_H
#define CRYPTOTRADER_TICK_H

#include "DataTypes.h"
#include "utils/TraderUtils.h"
#include <iostream>
#include <vector>

class Tick {
 private:
  Time m_timestamp;
  int64_t m_trade_id;
  double m_price;
  double m_size;

 public:
  inline Tick(Time timestamp, int64_t trade_id, double price, double size)
      : m_timestamp(timestamp), m_trade_id(trade_id), m_price(price), m_size(size) {}

  inline Tick() {
    m_timestamp = 0;
    m_trade_id = 0;
    m_price = 0;
    m_size = 0;
  }

  inline Tick(const std::string& line, std::vector<int8_t> mapping = {0, 1, 2, 3}) {
    std::vector<std::string> data;
    int num_elements = TradeUtils::parseCSVLine(data, line);

    if (mapping.size() == 4) {
      // default format
      assert(num_elements == 4);

      m_timestamp = Time(data[mapping[0]]);
      m_trade_id = stol(data[mapping[1]]);
      m_price = stod(data[mapping[2]]);
      m_size = stod(data[mapping[3]]);
    } else if (mapping.size() == 5) {
      // default format
      assert(num_elements == 5);

      m_timestamp = Time(data[mapping[0]]);
      m_trade_id = stol(data[mapping[1]]);
      m_price = stod(data[mapping[2]]);
      m_size = (data[mapping[4]] == "buy") ? stod(data[mapping[3]]) : -stod(data[mapping[3]]);
    }
  };

  inline int32_t getDate() const {
    return m_timestamp.days_since_epoch();
  }
  inline int64_t getTime() const {
    return m_timestamp.micros_since_midnight();
  }
  inline int64_t getUniqueID() const {
    return m_trade_id;
  }
  inline primary_key_t getPrimaryKey() const {
    return {m_timestamp.days_since_epoch(), m_timestamp.micros_since_midnight(), m_trade_id};
  }

  inline double getPrice() const {
    return m_price;
  }
  inline double getSize() const {
    return m_size;
  }

  inline Time getTimeStamp() const {
    return m_timestamp;
  }

  inline void setTimeStamp(Time timestamp) {
    m_timestamp = timestamp;
  }

  inline void* getUniqueIdPtr() {
    return &m_trade_id;
  }

  template <typename T2>
  inline void setUniqueId(T2 unique_id) {
    m_trade_id = static_cast<int64_t>(unique_id);
  };

  void* getMemberPtr(size_t i);

  void setMemberPtr(void* value, size_t i);

  friend std::ostream& operator<<(std::ostream& os, const Tick& tick) {
    os << std::setprecision(6) << std::fixed;
    os << "[ " << tick.getTimeStamp().toISOTimeString() << " : " << std::setw(5) << tick.getUniqueID()
       << "] price : " << tick.getPrice() << ", size: " << fabs(tick.getSize()) << ", "
       << ((tick.getSize() > 0) ? std::string("buy") : std::string("sell")) << "";
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }

  friend bool operator==(const Tick& lhs, const Tick& rhs) {
    return (std::tuple<Time, int64_t>(lhs.m_timestamp, lhs.m_trade_id) ==
            std::tuple<Time, int64_t>(rhs.m_timestamp, rhs.m_trade_id));
  }
  friend bool operator!=(const Tick& lhs, const Tick& rhs) {
    return (std::tuple<Time, int64_t>(lhs.m_timestamp, lhs.m_trade_id) !=
            std::tuple<Time, int64_t>(rhs.m_timestamp, rhs.m_trade_id));
  }
  friend bool operator>(const Tick& lhs, const Tick& rhs) {
    return (std::tuple<Time, int64_t>(lhs.m_timestamp, lhs.m_trade_id) >
            std::tuple<Time, int64_t>(rhs.m_timestamp, rhs.m_trade_id));
  }
  friend bool operator<(const Tick& lhs, const Tick& rhs) {
    return (std::tuple<Time, int64_t>(lhs.m_timestamp, lhs.m_trade_id) <
            std::tuple<Time, int64_t>(rhs.m_timestamp, rhs.m_trade_id));
  }
  friend bool operator>=(const Tick& lhs, const Tick& rhs) {
    return (std::tuple<Time, int64_t>(lhs.m_timestamp, lhs.m_trade_id) >=
            std::tuple<Time, int64_t>(rhs.m_timestamp, rhs.m_trade_id));
  }
  friend bool operator<=(const Tick& lhs, const Tick& rhs) {
    return (std::tuple<Time, int64_t>(lhs.m_timestamp, lhs.m_trade_id) <=
            std::tuple<Time, int64_t>(rhs.m_timestamp, rhs.m_trade_id));
  }

  static const std::pair<std::string, std::string> m_unique_id;
  static const field_t m_fields;
  static const std::string m_type;
};

#endif  // CRYPTOTRADER_TICK_H
