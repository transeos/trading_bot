//
// Created by Subhagato Dutta on 9/13/17.
//

#ifndef CRYPTOTRADER_ORDERBOOK_H
#define CRYPTOTRADER_ORDERBOOK_H

#include <Quote.h>
#include <iomanip>
#include <map>
#include <mutex>

class OrderBook {
 private:
  std::map<double, double> m_bid_list;
  std::map<double, double> m_ask_list;
  double m_spread;
  std::mutex m_mutex;

  void setSpread() {  // this function is always called inside mutex guard, so no extra guard is applied

    if (m_bid_list.size() > 0 && m_ask_list.size() > 0)
      m_spread = m_ask_list.begin()->first - m_bid_list.rbegin()->first;
    else
      m_spread = 0;
  }

 public:
  OrderBook() {
    m_spread = 0;
  }

  OrderBook(const std::map<double, double>& bid_list, const std::map<double, double>& ask_list) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bid_list = bid_list;
    m_ask_list = ask_list;
    setSpread();
  }

  OrderBook(const std::vector<std::pair<double, double>>& bid_list,
            const std::vector<std::pair<double, double>>& ask_list) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& bid_level : bid_list) m_bid_list.insert(bid_level);

    for (auto& ask_level : ask_list) m_ask_list.insert(ask_level);

    setSpread();
  }

  void reset() {
    m_bid_list.clear();
    m_ask_list.clear();

    m_spread = 0;
  }

  double getSpread() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_spread;
  }

  void lock() {
    m_mutex.lock();
  }

  void unlock() {
    m_mutex.unlock();
  }

  void addBidPriceLevel(double price, double size) {
    if (size == 0.0) {
      if (m_bid_list.erase(price) == 0) m_ask_list.erase(price);
    } else {
      m_bid_list[price] = size;
    }
    setSpread();
  }

  void addAskPriceLevel(double price, double size) {
    if (size == 0.0) {
      if (m_ask_list.erase(price) == 0) m_bid_list.erase(price);
    } else {
      m_ask_list[price] = size;
    }
    setSpread();
  }

  Quote getQuote() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_bid_list.size() > 0 && m_ask_list.size() > 0)
      return Quote(m_bid_list.rbegin()->first, m_bid_list.rbegin()->second, m_ask_list.begin()->first,
                   m_ask_list.begin()->second);
    else
      return Quote();
  }

  double getBestBid() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bid_list.rbegin()->first;
  }

  double getBestAsk() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ask_list.begin()->first;
  }

  size_t getBidList(std::map<double, double>& bid_list) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bid_list = m_bid_list;
    return m_bid_list.size();
  }

  size_t getAskList(std::map<double, double>& ask_list) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ask_list = m_ask_list;
    return ask_list.size();
  }

  friend std::ostream& operator<<(std::ostream& os, OrderBook& ob) {
    std::map<double, double> bid_list, ask_list;

    os << std::fixed;

    auto ask_itr = ask_list.begin();
    auto bid_itr = bid_list.rbegin();

    os << "spread: " << std::setw(38) << std::setprecision(2) << ob.getSpread() << std::endl;

    for (int i = 0; ask_itr != ask_list.end() && bid_itr != bid_list.rend(); ask_itr++, bid_itr++, i++) {
      os << "ask: [price:" << std::setw(10) << std::setprecision(2) << ask_itr->first << ", size:" << std::setw(10)
         << std::setprecision(6) << ask_itr->second << "]\t";
      os << "bid: [price:" << std::setw(10) << std::setprecision(2) << bid_itr->first << ", size:" << std::setw(10)
         << std::setprecision(6) << bid_itr->second << "]\n";

      if (i == 50) break;
    }

    return os;
  }

  void print() {
    TradeUtils::displayT(*this);
  }
};

#endif  // CRYPTOTRADER_ORDERBOOK_H
