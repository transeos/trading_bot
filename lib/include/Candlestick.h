//
// Created by subhagato on 11/2/17.
//

#ifndef CRYPTOTRADER_CANDLESTICK_H
#define CRYPTOTRADER_CANDLESTICK_H

#include "DataTypes.h"
#include "Enums.h"
#include "utils/TimeUtils.h"
#include <iostream>
#include <utils/Volume.h>
#include <vector>

class Candlestick {
 private:
  Time m_timestamp;
  double m_open;
  double m_close;
  double m_low;
  double m_high;
  double m_mean;
  double m_stddev;
  Volume m_volume;

 public:
  Candlestick() {
    m_timestamp = 0;
    m_open = 0;
    m_close = 0;
    m_low = 0;
    m_high = 0;
    m_mean = 0;
    m_stddev = 0;
    m_volume = 0;
  }

  Candlestick(const Time& m_timestamp, double m_open, double m_close, double m_low, double m_high, double m_mean,
              double m_stddev, Volume m_volume)
      : m_timestamp(m_timestamp),
        m_open(m_open),
        m_close(m_close),
        m_low(m_low),
        m_high(m_high),
        m_mean(m_mean),
        m_stddev(m_stddev),
        m_volume(m_volume) {}

  inline Candlestick(const std::string& line, std::vector<int8_t> mapping = {0, 1, 2, 3, 4, 5, 6, 7}) {
    // parse CSV line
    assert(0);
  };

  Candlestick(const Time start_time, const std::vector<double>& price_arr, std::vector<Volume>& volume_arr);

  inline int32_t getDate() const {
    return m_timestamp.days_since_epoch();
  }
  inline int64_t getTime() const {
    return m_timestamp.micros_since_midnight();
  }

  // dummy function, called while writing in csv
  inline int64_t getUniqueID() const {
    return 0;
  }

  inline primary_key_t getPrimaryKey() const {
    return {m_timestamp.days_since_epoch(), m_timestamp.micros_since_midnight(), 0};
  }

  inline Time getTimeStamp() const {
    return m_timestamp;
  }
  inline void setTimeStamp(Time timestamp) {
    m_timestamp = timestamp;
  }

  // dummy function
  inline void* getUniqueIdPtr() {
    assert(0);
    return nullptr;
  }

  inline double getOpen() const {
    return m_open;
  }
  inline double getClose() const {
    return m_close;
  }
  inline double getLow() const {
    return m_low;
  }
  inline double getHigh() const {
    return m_high;
  }
  inline double getMean() const {
    return m_mean;
  }
  inline double getStdDev() const {
    return m_stddev;
  }
  inline Volume getVolume() const {
    return m_volume;
  }
  inline double getTotalVolume() const {
    return m_volume.getVolume();
  }
  inline double getBuyVolume() const {
    return m_volume.getBuyVolume();
  }
  inline double getSellVolume() const {
    return m_volume.getSellVolume();
  }

  inline void setOpen(double open) {
    m_open = open;
  }
  inline void setClose(double close) {
    m_close = close;
  }
  inline void setLow(double low) {
    m_low = low;
  }
  inline void setHigh(double high) {
    m_high = high;
  }
  inline void setMean(double mean) {
    m_mean = mean;
  }
  inline void setStdDev(double stddev) {
    m_stddev = stddev;
  }
  inline void setVolume(Volume volume) {
    m_volume = volume;
  }

  inline void setPrice(double price) {
    m_open = price;
    m_close = price;
    m_low = price;
    m_high = price;
    m_mean = price;
    m_stddev = 0;
  }

  void* getMemberPtr(size_t i);
  void setMemberPtr(void* value, size_t i);

  double get(candle_price_t price) const {
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
      default:
        return 0;
    }
  }

  friend std::ostream& operator<<(std::ostream& os, const Candlestick& candlestick) {
    os << std::setprecision(6) << std::fixed;
    os << "[ " << candlestick.getTimeStamp().toISOTimeString() << " ] ";
    os << " open: " << candlestick.getOpen();
    os << ", close: " << candlestick.getClose();
    os << ", low: " << candlestick.getLow();
    os << ", high: " << candlestick.getHigh();
    os << ", mean: " << candlestick.getMean();
    os << ", stddev: " << candlestick.getStdDev();
    os << ", volume: " << candlestick.getTotalVolume();

    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }

  friend bool operator==(const Candlestick& lhs, const Candlestick& rhs) {
    return lhs.getTimeStamp() == rhs.getTimeStamp();
  }
  friend bool operator>(const Candlestick& lhs, const Candlestick& rhs) {
    return lhs.getTimeStamp() > rhs.getTimeStamp();
  }
  friend bool operator<(const Candlestick& lhs, const Candlestick& rhs) {
    return lhs.getTimeStamp() < rhs.getTimeStamp();
  }
  friend bool operator>=(const Candlestick& lhs, const Candlestick& rhs) {
    return lhs.getTimeStamp() >= rhs.getTimeStamp();
  }
  friend bool operator<=(const Candlestick& lhs, const Candlestick& rhs) {
    return lhs.getTimeStamp() <= rhs.getTimeStamp();
  }

  static const std::pair<std::string, std::string> m_unique_id;
  static const field_t m_fields;
  static const std::string m_type;
};

#endif  // CRYPTOTRADER_CANDLESTICK_H
