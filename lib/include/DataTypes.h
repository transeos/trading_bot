//
// Created by subhagato on 2/8/18.
//

#ifndef CRYPTOTRADER_DATATYPES_HPP
#define CRYPTOTRADER_DATATYPES_HPP

#include "Enums.h"
#include "utils/TimeUtils.h"
#include <iostream>

typedef struct primary_key_t {
  int32_t date;
  int64_t time;
  int64_t unique_id;

  Time getTimeStamp() {
    return Time(date, time);
  }

  primary_key_t(int32_t date = 0, int64_t time = 0, int64_t unique_id = 0)
      : date(date), time(time), unique_id(unique_id) {}

  bool operator>(const primary_key_t& p) const {
    return (Time(date, time) >= Time(p.date, p.time)) && (unique_id > p.unique_id);
  };
  bool operator>=(const primary_key_t& p) const {
    return (Time(date, time) >= Time(p.date, p.time)) && (unique_id >= p.unique_id);
  };
  bool operator==(const primary_key_t& p) const {
    return (date == p.date) && (time == p.time) && (unique_id == p.unique_id);
  };
  bool operator<=(const primary_key_t& p) const {
    return (Time(date, time) <= Time(p.date, p.time)) && (unique_id <= p.unique_id);
  };
  bool operator<(const primary_key_t& p) const {
    return (Time(date, time) <= Time(p.date, p.time)) && (unique_id < p.unique_id);
  };
  bool operator!=(const primary_key_t& p) const {
    return !(*this == p);
  };

} primary_key_t;

typedef std::vector<std::pair<std::string, std::string>> field_t;

inline std::ostream& operator<<(std::ostream& os, const primary_key_t& primary_key) {
  os << "[" << primary_key.date << ", " << primary_key.time << ", " << primary_key.unique_id << " ]";
  return os;
}

static void printPrimaryKey(const primary_key_t& a_primary_key) {
  TradeUtils::displayT(a_primary_key);
}

typedef struct {
  double price;
  double volume;
} price_volume_t;

typedef struct {
  double maker_fee;
  double taker_fee;
} fee_t;

typedef struct {
  double volume;
  double price;
} order_exe_t;

typedef std::string order_id_t;

typedef struct {
  Time last_updated;
  double cached_price;
} conversion_t;

typedef struct {
  double average;
  double standard_dev;

} residual_amount_t;

typedef struct trading_pair_t {
  exchange_t exchange_id;
  currency_t base_currency;
  currency_t quote_currency;

  bool operator==(const trading_pair_t& p) const {
    return ((exchange_id == p.exchange_id) && (base_currency == p.base_currency) &&
            (quote_currency == p.quote_currency));
  }

} trading_pair_t;

namespace std {
template <>
struct hash<trading_pair_t> {
  std::size_t operator()(const trading_pair_t& k) const {
    using std::hash;

    // assuming currency_t value is less than 4096 (2^12) and exchange_t value is less than 256 (2^8)
    return (hash<int>()((((int)k.exchange_id) << 24) + (((int)k.base_currency) << 12) + ((int)k.quote_currency)));
  }
};
}

#endif  // CRYPTOTRADER_DATATYPES_HPP
