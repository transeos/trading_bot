// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 9/9/17
//
//*****************************************************************

#ifndef CRYPTOTRADER_CURRENCY_H
#define CRYPTOTRADER_CURRENCY_H

#include "Enums.h"
#include "utils/TraderUtils.h"
#include <iostream>

class Currency {
 private:
  currency_t m_currency;
  double m_min_size;  // actual currency value
  double m_usd_conv;
  bool m_is_crypto;

 public:
  Currency(const currency_t currency, const double min_size) : m_currency(currency), m_min_size(min_size) {
    if (min_size < 0.01)
      m_is_crypto = true;
    else
      m_is_crypto = false;
  }
  Currency(const std::string& currency, const double min_size) : m_min_size(min_size) {
    m_currency = sStringToCurrency(currency);
    if (min_size < 0.01)
      m_is_crypto = true;
    else
      m_is_crypto = false;
  }
  Currency(const std::string& currency, const bool is_crypto = true) {
    m_currency = sStringToCurrency(currency);
    m_is_crypto = is_crypto;

    if (is_crypto)
      m_min_size = 0.000001;
    else
      m_min_size = 0.01;
  }

  Currency(const currency_t currency) : m_currency(currency) {
    m_min_size = 0;
  }

  static std::string sCurrencyToString(const currency_t currency, bool lower = false);
  static currency_t sStringToCurrency(std::string currency);

  // TODO: this function should be 'const'
  double getValueUSD();

  friend std::ostream& operator<<(std::ostream& os, const Currency& c) {
    os << sCurrencyToString(c.m_currency) << " [min_size: " << c.m_min_size << "]";
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }

  currency_t getSymbol() const {
    return m_currency;
  }
  std::string getSymbolStr(bool lower = false) const {
    return sCurrencyToString(m_currency, lower);
  }
};

inline bool operator==(const Currency& lhs, const currency_t& rhs) {
  return lhs.getSymbol() == rhs;
}

inline bool operator!=(const Currency& lhs, const currency_t& rhs) {
  return lhs.getSymbol() != rhs;
}

inline bool operator==(const currency_t& lhs, Currency& rhs) {
  return lhs == rhs.getSymbol();
}

inline bool operator!=(const currency_t& lhs, Currency& rhs) {
  return lhs != rhs.getSymbol();
}

inline bool operator==(const Currency& lhs, const Currency& rhs) {
  return lhs.getSymbol() == rhs.getSymbol();
}

namespace std {

template <>
struct hash<currency_t> {
  std::size_t operator()(const currency_t& k) const {
    using std::hash;

    // Compute  hash values for currency_t as integer:

    return (hash<int>()((int)k));
  }
};
}

namespace std {
template <>
struct hash<Currency> {
  std::size_t operator()(const Currency& k) const {
    using std::hash;

    // Compute  hash values for currency_t as integer:

    return (hash<int>()((int)k.getSymbol()));
  }
};
}

#endif  // CRYPTOTRADER_CURRENCY_H
