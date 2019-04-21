//
// Created by Subhagato Dutta on 9/14/17.
//

#ifndef CRYPTOTRADER_MARKET_H
#define CRYPTOTRADER_MARKET_H

#include "CurrencyPair.h"
#include "DataTypes.h"
#include "OrderBook.h"
#include "TradeHistory.h"

class Market {
 private:
  CurrencyPair m_currency_pair;
  double m_min_size;
  double m_max_size;
  double m_quote_increment;
  fee_t m_fees;
  double m_spread;

  int64_t m_last_trade_id;
  bool m_websocket_subscribed;
  double m_bid_price;
  double m_ask_price;
  OrderBook* m_order_book_ptr;
  TradeHistory* m_trade_history_ptr;
  std::mutex m_mutex;

 public:
  Market() {
    m_currency_pair = CurrencyPair();
    m_min_size = 0;
    m_max_size = 0;
    m_quote_increment = 0;
    m_fees = {0, 0};
    m_last_trade_id = -1;
    m_websocket_subscribed = false;
    m_bid_price = 0;
    m_ask_price = 0;
    m_order_book_ptr = nullptr;
    m_trade_history_ptr = nullptr;
    m_spread = 0;
  }

  Market(const CurrencyPair currency_pair, const double min_size = 0.01, const double max_size = 10000.00,
         const double quote_increment = 0.01)
      : m_currency_pair(currency_pair), m_min_size(min_size), m_max_size(max_size), m_quote_increment(quote_increment) {
    m_last_trade_id = -1;
    m_websocket_subscribed = false;
    m_bid_price = 0;
    m_ask_price = 0;
    m_order_book_ptr = new OrderBook();
    m_trade_history_ptr = nullptr;
    m_spread = 0;
  }

  Market(const std::string& currency_pair, const double min_size = 0.01, const double max_size = 10000.00,
         const double quote_increment = 0.01)
      : m_min_size(min_size), m_max_size(max_size), m_quote_increment(quote_increment) {
    m_currency_pair = CurrencyPair(currency_pair);
    m_websocket_subscribed = false;
    m_bid_price = 0;
    m_ask_price = 0;
    m_order_book_ptr = new OrderBook();
    m_trade_history_ptr = nullptr;
  }

  TradeHistory* getTradeHistory() const {
    return m_trade_history_ptr;
  }

  void deleteTradeHistory() {
    DELETE(m_trade_history_ptr);
  }

  void setTradeHistory(TradeHistory* trade_history_ptr) {
    Market::m_trade_history_ptr = trade_history_ptr;
  }

  const CurrencyPair& getCurrencyPair() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currency_pair;
  }

  std::string getCurrencyPairStr(bool lower = false) const {
    return m_currency_pair.toString("-", lower);
  }

  friend std::ostream& operator<<(std::ostream& os, const Market& cp) {
    os << cp.m_currency_pair << " [min_size: " << cp.m_min_size << ", max_size: " << cp.m_max_size
       << ", quote_increment: " << cp.m_quote_increment << "]";
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }

  int64_t getLastTradeId() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_last_trade_id;
  }

  void setLastTradeId(int64_t last_trade_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Market::m_last_trade_id = last_trade_id;
  }

  bool isWebsocketSubscribed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_websocket_subscribed;
  }

  void webSocketSubscribed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_websocket_subscribed) m_websocket_subscribed = true;
  }

  void webSocketUnsubscribed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_websocket_subscribed) m_websocket_subscribed = false;
  }

  const Quote getQuote() {
    return m_order_book_ptr->getQuote();
  }

  double getTickerPrice() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return ((m_bid_price + m_ask_price) / 2);
  }

  double getBidPrice() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bid_price;
  }

  void setBidPrice(double bid_price) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Market::m_bid_price = bid_price;
  }

  double getAskPrice() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ask_price;
  }

  void setAskPrice(double ask_price) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Market::m_ask_price = ask_price;
  }

  OrderBook* getOrderBook() const {
    return m_order_book_ptr;
  }

  virtual ~Market() {
    DELETE(m_trade_history_ptr);
    DELETE(m_order_book_ptr);
  }

  void setSpread(const double a_spread) {
    m_spread = a_spread;
  }
  double getSpread() const {
    return m_spread;
  }
};

#endif  // CRYPTOTRADER_MARKET_H
