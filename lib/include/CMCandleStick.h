// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 25/11/17
//
//*****************************************************************

#ifndef CMC_TICK_H
#define CMC_TICK_H

#include "DataTypes.h"
#include "Enums.h"
#include "utils/JsonUtils.h"
#include "utils/TraderUtils.h"
#include <iostream>
#include <unordered_map>
#include <utils/Volume.h>
#include <vector>

class CMCandleStick {
 private:
  Time m_timestamp;

  /* actual members stored in database*/
  double m_price_usd;
  double m_price_btc;
  double m_market_cap_usd;
  double m_volume_usd;

  /* derived members not stored in database*/
  double m_open;
  double m_close;
  double m_low;
  double m_high;
  double m_mean;
  double m_stddev;

 public:
  CMCandleStick(int64_t timestamp, double market_cap_usd, double price_btc, double price_usd, double volume_usd)
      : m_price_usd(price_usd), m_price_btc(price_btc), m_market_cap_usd(market_cap_usd), m_volume_usd(volume_usd) {
    m_timestamp = Time(timestamp * 1000LL).quantize(m_interval);
    setPrice(m_price_usd);
  }

  CMCandleStick() {
    m_timestamp = 0;
    m_market_cap_usd = 0;
    m_price_btc = 0;
    m_price_usd = 0;
    m_volume_usd = 0;

    setPrice(0);
  }

  CMCandleStick(const std::string& line, std::vector<int8_t> mapping = {0, 1, 2, 3, 4}) {
    std::vector<std::string> data;
    int num_elements = TradeUtils::parseCSVLine(data, line);
    assert(num_elements == 5);

    m_timestamp = Duration(0, 0, 0, 0, stoi(data[mapping[0]]), 0);  // assuming data[12] has seconds timestamp

    m_market_cap_usd = stod(data[mapping[1]]);
    m_price_btc = stod(data[mapping[2]]);
    m_price_usd = stod(data[mapping[3]]);
    m_volume_usd = stod(data[mapping[4]]);

    setPrice(m_price_usd);
  }

  int32_t getDate() const {
    return m_timestamp.days_since_epoch();  // returns number of days from 1970-1-1
  }

  int64_t getTime() const {
    return m_timestamp.micros_since_midnight();  // returns second timestamp
  }

  int64_t getUniqueID() const {
    return m_timestamp / m_interval;
  }  // divide between time and duration is defined in the TimeUtils class

  Time getTimeStamp() const {
    return m_timestamp;
  }

  primary_key_t getPrimaryKey() const {
    return {m_timestamp.days_since_epoch(), m_timestamp.micros_since_midnight(), m_timestamp / m_interval};
  }

  double getPrice() const {
    return m_price_usd;
  }  // get price by default gives the mean price

  void setPrice(double price) {
    m_price_usd = price;
    m_open = m_price_usd;
    m_close = m_price_usd;
    m_low = m_price_usd;
    m_high = m_price_usd;
    m_mean = m_price_usd;
    m_stddev = 0;
  }

  Volume getVolume() const {
    return m_volume_usd;
  }

  void setVolume(Volume volume) {
    m_volume_usd = static_cast<double>(volume);
  }

  double getOpen() const {
    return m_open;
  }

  double getClose() const {
    return m_close;
  }

  double getLow() const {
    return m_low;
  }

  double getHigh() const {
    return m_high;
  }

  double getMean() const {
    return m_mean;
  }

  double getStdDev() const {
    return m_stddev;
  }

  void setOpen(double open) {
    m_open = open;
  }

  void setClose(double close) {
    m_close = close;
  }

  void setLow(double low) {
    m_low = low;
  }

  void setHigh(double high) {
    m_high = high;
  }

  void setMean(double mean) {
    m_mean = mean;
    m_price_usd = mean;
  }

  void setStdDev(double stddev) {
    m_stddev = stddev;
  }

  void setTimeStamp(Time timestamp) {
    m_timestamp = timestamp;
  }

  void* getUniqueIdPtr() {
    return &m_timestamp;
  }

  void* getMemberPtr(size_t i);

  double get(candle_price_t price) {
    switch (price) {
      case candle_price_t::OPEN:
        return m_open;
      case candle_price_t::CLOSE:
        return m_close;
      case candle_price_t::HIGH:
        return m_high;
      case candle_price_t::LOW:
        return m_low;
      case candle_price_t::MEAN:
        return m_mean;
    }
  }

  friend std::ostream& operator<<(std::ostream& os, const CMCandleStick& a_CMCandleStick) {
    os << std::setprecision(6) << std::fixed;
    os << "[ " << a_CMCandleStick.getTimeStamp().toISOTimeString() << " : " << std::setw(5)
       << a_CMCandleStick.getUniqueID() << "] price : " << a_CMCandleStick.getPrice();
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }

  friend bool operator==(const CMCandleStick& lhs, const CMCandleStick& rhs) {
    return lhs.m_timestamp == rhs.m_timestamp;
  }

  friend bool operator!=(const CMCandleStick& lhs, const CMCandleStick& rhs) {
    return lhs.m_timestamp != rhs.m_timestamp;
  }

  friend bool operator>(const CMCandleStick& lhs, const CMCandleStick& rhs) {
    return lhs.m_timestamp > rhs.m_timestamp;
  }

  friend bool operator<(const CMCandleStick& lhs, const CMCandleStick& rhs) {
    return lhs.m_timestamp < rhs.m_timestamp;
  }

  friend bool operator>=(const CMCandleStick& lhs, const CMCandleStick& rhs) {
    return lhs.m_timestamp >= rhs.m_timestamp;
  }

  friend bool operator<=(const CMCandleStick& lhs, const CMCandleStick& rhs) {
    return lhs.m_timestamp <= rhs.m_timestamp;
  }

  static const std::pair<std::string, std::string> m_unique_id;
  static const field_t m_fields;
  static const std::string m_type;
  static const Duration m_interval;
};

#endif  // CMC_TICK_H
