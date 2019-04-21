// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 17/02/18.
//

#ifndef COINAPITICK_H
#define COINAPITICK_H

#include "DataTypes.h"
#include "ExchangeEnums.h"
#include "utils/TimeUtils.h"
#include "utils/TraderUtils.h"
#include <iostream>
#include <vector>

class CoinAPITick {
 private:
  double m_price;
  double m_size;

  // consists of timestamp, exchange, uuid hash
  int64_t m_flags;

  void computeUniqueID(const Time a_coinapi_time, exchange_t a_exchange_id, int64_t a_hash);

 public:
  CoinAPITick(const Time a_coinapi_time, const double a_price, const double a_size, exchange_t a_exchange_id,
              std::string a_uuid);

  inline CoinAPITick() {
    m_flags = 0;
    m_price = 0;
    m_size = 0;
  }

  inline CoinAPITick(const CoinAPITick& a_tick) {
    m_price = a_tick.getPrice();
    m_size = a_tick.getSize();
    m_flags = a_tick.getUniqueID();
  }

  inline CoinAPITick(const std::string& line, std::vector<int8_t> mapping = {0, 1, 2, 3}) {
    std::vector<std::string> data;
    int num_elements = TradeUtils::parseCSVLine(data, line);
    assert(num_elements == 4);

    // assuming data[0] is ISO Time
    Time timestamp = Time(data[mapping[0]]);

    m_flags = stol(data[mapping[1]]);

    assert((getTimeStamp() / 1000L) == (timestamp / 1000L));

    m_price = stod(data[mapping[2]]);
    m_size = stod(data[mapping[3]]);
  }

  inline int32_t getDate() const {
    return getTimeStamp().days_since_epoch();
  }
  inline int64_t getTime() const {
    return getTimeStamp().micros_since_midnight();
  }
  inline int64_t getUniqueID() const {
    return m_flags;
  }

  inline primary_key_t getPrimaryKey() const {
    return {getTimeStamp().days_since_epoch(), getTimeStamp().micros_since_midnight(), 0};
  }

  inline double getPrice() const {
    return m_price;
  }
  inline void multiplyPrice(const double a_mul_factor) {
    m_price *= a_mul_factor;
  }

  inline double getSize() const {
    return m_size;
  }

  Time getTimeStamp() const;
  inline void setTimeStamp(const Time timestamp) {
    assert((getTimeStamp() / 1000L) == (timestamp / 1000L));
  }

  exchange_t getExchangeId() const;

  inline void* getUniqueIdPtr() {
    return &m_flags;
  }

  void* getMemberPtr(const size_t i);
  void setMemberPtr(const void* value, const size_t i);

  friend std::ostream& operator<<(std::ostream& os, const CoinAPITick& tick) {
    os << std::setprecision(6) << std::fixed;
    os << "[ " << tick.getTimeStamp().toISOTimeString() << " : " << std::setw(5) << tick.getUniqueID();
    os << "] exchange: " << static_cast<int>(tick.getExchangeId());
    os << " price: " << tick.getPrice() << ", size: " << fabs(tick.getSize()) << ", ";
    os << ((tick.getSize() > 0) ? std::string("buy") : std::string("sell")) << "";
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }

  friend bool operator==(const CoinAPITick& lhs, const CoinAPITick& rhs) {
    return lhs.getTimeStamp() == rhs.getTimeStamp();
  }
  friend bool operator>(const CoinAPITick& lhs, const CoinAPITick& rhs) {
    return lhs.getTimeStamp() > rhs.getTimeStamp();
  }
  friend bool operator<(const CoinAPITick& lhs, const CoinAPITick& rhs) {
    return lhs.getTimeStamp() < rhs.getTimeStamp();
  }
  friend bool operator>=(const CoinAPITick& lhs, const CoinAPITick& rhs) {
    return lhs.getTimeStamp() >= rhs.getTimeStamp();
  }
  friend bool operator<=(const CoinAPITick& lhs, const CoinAPITick& rhs) {
    return lhs.getTimeStamp() <= rhs.getTimeStamp();
  }

  static const std::pair<std::string, std::string> m_unique_id;
  static const field_t m_fields;
  static const std::string m_type;
};

#endif  // COINAPITICK_H
