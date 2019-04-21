//
// Created by Subhagato Dutta on 12/24/17.
//

#ifndef CRYPTOTRADER_CURRENCYPAIR_H
#define CRYPTOTRADER_CURRENCYPAIR_H

#include "Currency.h"
#include "utils/StringUtils.h"
#include "utils/TraderUtils.h"

class CurrencyPair {
 private:
  currency_t m_base_currency;
  currency_t m_quote_currency;

 public:
  CurrencyPair() {}
  CurrencyPair(currency_t baseCurrency, currency_t quoteCurrency)
      : m_base_currency(baseCurrency), m_quote_currency(quoteCurrency) {}
  CurrencyPair(std::string currency_pair) {
    fromString(currency_pair);
  }

  inline currency_t getBaseCurrency() const {
    return m_base_currency;
  }
  inline currency_t getQuoteCurrency() const {
    return m_quote_currency;
  }

  void fromString(std::string currency_pair) {
    std::vector<std::string> c_vect;

    if (currency_pair.find('-') != std::string::npos) {
      c_vect = StringUtils::split(currency_pair, '-');
      m_base_currency = Currency::sStringToCurrency(c_vect[0]);
      m_quote_currency = Currency::sStringToCurrency(c_vect[1]);
    } else if (currency_pair.find('/') != std::string::npos) {
      c_vect = StringUtils::split(currency_pair, '/');
      m_base_currency = Currency::sStringToCurrency(c_vect[0]);
      m_quote_currency = Currency::sStringToCurrency(c_vect[1]);
    } else if (currency_pair.size() == 6) {
      m_base_currency = Currency::sStringToCurrency(currency_pair.substr(0, 3));
      m_quote_currency = Currency::sStringToCurrency(currency_pair.substr(3, 3));
    } else {
      throw std::invalid_argument("Invalid currency-pair string.");
    }
  }

  void operator=(const std::string currency_pair) {
    fromString(currency_pair);
  }

  operator std::string() {
    return toString();
  }

  std::string toString(const std::string separator = "-", bool lower = false) const {
    std::string cp_str;

    cp_str = Currency::sCurrencyToString(this->m_base_currency) + separator +
             Currency::sCurrencyToString(this->m_quote_currency);

    if (lower) std::transform(cp_str.begin(), cp_str.end(), cp_str.begin(), ::tolower);

    return cp_str;
  }

  friend std::ostream& operator<<(std::ostream& os, const CurrencyPair& cp) {
    os << cp.toString();
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }
};

inline bool operator==(const CurrencyPair& lhs, const CurrencyPair& rhs) {
  return (lhs.getQuoteCurrency() == rhs.getQuoteCurrency() && lhs.getBaseCurrency() == rhs.getBaseCurrency());
}

inline bool operator!=(const CurrencyPair& lhs, const CurrencyPair& rhs) {
  return !(lhs == rhs);
}

namespace std {
template <>
struct hash<CurrencyPair> {
  std::size_t operator()(const CurrencyPair& k) const {
    using std::hash;

    // assuming currency_t value is less than 65536 (2^16)

    return (hash<int>()((((int)k.getBaseCurrency()) << 16) + ((int)k.getQuoteCurrency())));
  }
};
}

#endif  // CRYPTOTRADER_CURRENCYPAIR_H
