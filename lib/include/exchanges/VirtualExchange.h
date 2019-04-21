//
// Created by Subhagato Dutta on 10/11/17.
//

#ifndef CRYPTOTRADER_VIRTUALEXCHANGE_H
#define CRYPTOTRADER_VIRTUALEXCHANGE_H

#include "DataTypes.h"
#include "Globals.h"
#include "Order.h"
#include "TradeHistory.h"
#include "utils/TraderUtils.h"
#include <queue>
#include <random>
#include <unordered_map>

class Exchange;
class Account;
class ExchangeAccounts;
class Order;
class Quote;

typedef struct {
  std::priority_queue<Order*, std::vector<Order*>, TradeUtils::GreaterPT<Order>> max_order_queue;
  std::priority_queue<Order*, std::vector<Order*>, TradeUtils::LessPT<Order>> min_order_queue;
  std::priority_queue<Order*, std::vector<Order*>, TradeUtils::GreaterPT<Order>> cur_order_queue;
} order_queues_t;

class VirtualExchange {
 private:
  std::unordered_map<CurrencyPair, order_queues_t> m_all_order_queues;
  std::vector<const Order*> m_vir_cancelled_orders;

  ExchangeAccounts* m_virtual_wallet;

  Time m_simulation_time;

  std::default_random_engine m_rand_gen;

  void executeMarketOrder(Order* ap_order, const double a_fee);
  void executeLimitOrder(Order* ap_order, const Tick& a_trade, const double a_fee);

 protected:
  std::unordered_map<CurrencyPair, TradeHistory*> m_full_history;
  std::unordered_map<CurrencyPair, TradeHistory*> m_history_till_simulation_time;

 public:
  VirtualExchange();
  ~VirtualExchange();

  // casting functions
  virtual Exchange* castExchange() = 0;

  ExchangeAccounts* getVirtualWallet() {
    return m_virtual_wallet;
  }
  const ExchangeAccounts* getVirtualWallet() const {
    return m_virtual_wallet;
  }
  void addWallet(const ExchangeAccounts* ap_wallet);
  void addWallet(const std::vector<Account>& accounts);

  void addVirtualAccount(const currency_t currency, const double amount);
  void addVirtualAccount(const currency_t currency, const Account account);

  void run(const Time a_start_time, const Time a_end_time, const double a_transaction_cost);

  void executeOrder(Order* ap_order, const double a_price, const double a_amount, const double a_fee);

  void cancelVirtualOrders(const CurrencyPair& currency_pair);
  void cancelLimitVirtualBuyOrders(const CurrencyPair& currency_pair);
  void cancelLimitVirtualSellOrders(const CurrencyPair& currency_pair);
  void cancelLimitVirtualOrder(Order* ap_order);

  void dumpVirtualAccountStatus(const Time a_time, const Order* ap_order);

  const Order* lastCancelledVirtualOrder() const {
    return (m_vir_cancelled_orders.size() ? m_vir_cancelled_orders.back() : NULL);
  }

  void printOrders();

  TradeHistory* getCurrentTradeHistory(const CurrencyPair a_currency_pair) {
    TradeHistory* p_TradeHistory = m_history_till_simulation_time[a_currency_pair];
    ASSERT(p_TradeHistory);
    return p_TradeHistory;
  }

  TradeHistory* getFullTradeHistory(const CurrencyPair a_currency_pair) {
    TradeHistory* p_TradeHistory = m_full_history[a_currency_pair];
    ASSERT(p_TradeHistory);
    return p_TradeHistory;
  }

  Time getSimulationTime() const {
    return m_simulation_time;
  }
  void setSimulationTime(const Time a_time) {
    m_simulation_time = a_time;
  }

  void executeOrders(const double a_vol_fraction, const CurrencyPair& currency_pair);

  order_id_t placeVirtualOrder(CurrencyPair currency_pair, double amount, order_type_t type,
                               order_direction_t direction, double target_price, const Time time_instant,
                               const double a_cur_price);

  const Order* getVirtualOrder(const order_id_t a_order_id) const;

  Quote getVirtualQuote(const CurrencyPair& a_currency_pair);
};

#endif  // CRYPTOTRADER_VIRTUALEXCHANGE_H
