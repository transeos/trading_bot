//
// Created by Subhagato Dutta on 9/17/17.
//

#ifndef CRYPTOTRADER_QUOTE_H
#define CRYPTOTRADER_QUOTE_H

#include <iostream>

class Quote {
  double m_bid_price;
  double m_bid_size;
  double m_ask_price;
  double m_ask_size;

 public:
  Quote(const double m_bid_price, const double m_bid_size, const double m_ask_price, const double m_ask_size)
      : m_bid_price(m_bid_price), m_bid_size(m_bid_size), m_ask_price(m_ask_price), m_ask_size(m_ask_size) {
    assert(m_bid_price && m_bid_size && m_ask_price && m_ask_size);

    if (m_bid_price >= m_ask_price) {
      // TODO: change this to critical warning in future
      CT_WARN << "bid price(" << m_bid_price << ") >= ask price(" << m_ask_price << ")" << std::endl;
    }
  }

  Quote() {
    m_bid_price = 0;
    m_bid_size = 0;
    m_ask_price = 0;
    m_ask_size = 0;
  }

  double getBidPrice() const {
    return m_bid_price;
  }

  double getBidSize() const {
    return m_bid_size;
  }

  double getAskPrice() const {
    return m_ask_price;
  }

  double getAskSize() const {
    return m_ask_size;
  }

  double getTickerPrice() const {
    return (m_bid_price + m_ask_price) / 2.0;
  }

  bool isQuoteInvalid() const {
    return m_bid_price >= m_ask_price;
  }

  friend std::ostream& operator<<(std::ostream& os, const Quote& quote) {
    os << "bid: [price:" << quote.m_bid_price << ", size:" << quote.m_bid_size
       << "] \task: [price:" << quote.m_ask_price << ", size:" << quote.m_ask_size << "]";
    return os;
  }

  friend bool operator==(const Quote& lhs, const Quote& rhs) {
    return ((lhs.m_bid_price == rhs.m_bid_price) && (lhs.m_ask_price == rhs.m_ask_price));
  }
  friend bool operator!=(const Quote& lhs, const Quote& rhs) {
    return (!(lhs == rhs));
  }

  void print() const {
    TradeUtils::displayT(*this);
  }
};

#endif  // CRYPTOTRADER_QUOTE_H
