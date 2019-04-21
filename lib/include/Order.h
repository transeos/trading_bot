//
// Created by Subhagato Dutta on 10/22/17.
//

#ifndef CRYPTOTRADER_ORDER_H
#define CRYPTOTRADER_ORDER_H

#include "CurrencyPair.h"
#include "DataTypes.h"
#include "utils/Logger.h"
#include "utils/TimeUtils.h"

class ExchangeAccounts;

class Order {
 private:
  int m_idx;

  double m_trigger_price;
  trigger_type_t m_tigger_type;
  order_type_t m_order_type;
  order_direction_t m_order_direction;

  exchange_t m_exchange_id;
  CurrencyPair m_currency_pair;
  double m_amount;
  Time m_order_time;
  double m_price_at_order_time;

  std::vector<order_exe_t> m_order_exes;

  bool m_discarded;
  double m_cancelled_amount;

  order_id_t m_order_id;

 public:
  Order(const int a_order_id, double trigger_price, order_type_t order_type, order_direction_t order_direction,
        const exchange_t exchange_id, const CurrencyPair a_currency_pair, const double a_amount,
        const Time a_order_time, const double a_price_at_order_time);

  ~Order() {
    if (!m_discarded) {
      assert(m_amount == 0);
      // printOrderSummary();
    }
  }

  trigger_type_t getTiggerType() const {
    return m_tigger_type;
  }

  order_type_t getOrderType() const {
    return m_order_type;
  }

  int getOrderIdx() const {
    return m_idx;
  }

  order_direction_t getOrderDir() const {
    return m_order_direction;
  }

  exchange_t getExchangeId() const {
    return m_exchange_id;
  }

  CurrencyPair getCurrencyPair() const {
    return m_currency_pair;
  }

  double setExecute(const double amount, const double a_price, const ExchangeAccounts* a_wallet);

  double getAmount() const {
    return m_amount;
  }

  double getCancelledAmount() const {
    return m_cancelled_amount;
  }

  // handles floating approximation
  void adjustAmount(const double a_hold_amount);

  double getTriggerPrice() const {
    return m_trigger_price;
  }

  void populate(const double a_volume, const double a_price) {
    m_order_exes.push_back(order_exe_t{a_volume, a_price});
  }

  void cancelLimitOrder(ExchangeAccounts* a_wallet);

  void discard();

  void setOrderId(const order_id_t a_order_id) {
    m_order_id = a_order_id;
  }
  order_id_t getOrderId() const {
    return m_order_id;
  }

  void printLastStatus() const;

  void printOrderSummary() const;

  void pr() const;

  inline Time getTime() const {
    return m_order_time;
  }

  friend bool operator>(const Order& o1, const Order& o2) {
    return std::tie(o1.m_trigger_price, o1.m_order_time) > std::tie(o2.m_trigger_price, o2.m_order_time);
  }
  friend bool operator<(const Order& o1, const Order& o2) {
    return std::tie(o1.m_trigger_price, o1.m_order_time) < std::tie(o2.m_trigger_price, o2.m_order_time);
  }
  friend bool operator>=(const Order& o1, const Order& o2) {
    return std::tie(o1.m_trigger_price, o1.m_order_time) >= std::tie(o2.m_trigger_price, o2.m_order_time);
  }
  friend bool operator<=(const Order& o1, const Order& o2) {
    return std::tie(o1.m_trigger_price, o1.m_order_time) <= std::tie(o2.m_trigger_price, o2.m_order_time);
  }

  friend std::ostream& operator<<(std::ostream& os, const Order& order) {
    os << "Id: " << order.m_idx;
    os << ", Tigger price: " << order.m_trigger_price;
    os << ", Time: " << order.m_order_time;
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }
};

#endif  // CRYPTOTRADER_ORDER_H
