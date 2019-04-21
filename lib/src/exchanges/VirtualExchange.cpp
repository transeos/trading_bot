//
// Created by Subhagato Dutta on 10/11/17.
//

#include "exchanges/VirtualExchange.h"
#include "Controller.h"
#include "ExchangeAccounts.h"
#include "Tick.h"
#include "TraderBot.h"
#include "exchanges/Exchange.h"
#include <float.h>

using namespace std;

VirtualExchange::VirtualExchange() {
  m_virtual_wallet = NULL;
  m_simulation_time = Time(0);
}

VirtualExchange::~VirtualExchange() {
  for (auto order_queue_iter : m_all_order_queues) {
    order_queues_t& order_queues = order_queue_iter.second;

    while (!order_queues.cur_order_queue.empty()) {
      Order* p_order = order_queues.cur_order_queue.top();
      p_order->discard();
      DELETE(p_order);
      order_queues.cur_order_queue.pop();
    }

    while (!order_queues.min_order_queue.empty()) {
      Order* p_order = order_queues.min_order_queue.top();
      p_order->discard();
      DELETE(p_order);
      order_queues.min_order_queue.pop();
    }

    while (!order_queues.max_order_queue.empty()) {
      Order* p_order = order_queues.max_order_queue.top();
      p_order->discard();
      DELETE(p_order);
      order_queues.max_order_queue.pop();
    }
  }

  assert(m_full_history.size() == m_history_till_simulation_time.size());

  for (auto trade_history_iter : m_full_history) {
    DELETE(trade_history_iter.second);
  }
  m_full_history.clear();

  for (auto trade_history_iter : m_history_till_simulation_time) {
    DELETE(trade_history_iter.second);
  }
  m_history_till_simulation_time.clear();

  DELETE(m_virtual_wallet);
}

order_id_t VirtualExchange::placeVirtualOrder(CurrencyPair currency_pair, double amount, order_type_t type,
                                              order_direction_t direction, double target_price, const Time time_instant,
                                              const double a_cur_price) {
  // update virtual accounts
  const currency_t order_currency =
      (direction == order_direction_t::BUY) ? currency_pair.getQuoteCurrency() : currency_pair.getBaseCurrency();
  Account& acc = m_virtual_wallet->getAccount(order_currency);

  // handle residual amount
  if (direction == order_direction_t::BUY) {
    const residual_amount_t residual_amount = castExchange()->getResidualAmount(currency_pair.getBaseCurrency());

    double remaining_amount = residual_amount.average;
    if (g_random) {
      normal_distribution<double> distribution(residual_amount.average, residual_amount.standard_dev);
      remaining_amount = distribution(m_rand_gen);
    }

    amount -= (remaining_amount * a_cur_price);
  }
  assert((amount <= acc.getAvailable()) && (amount > 0));

  // order place time gets delayed by small amount
  const Time order_place_time = time_instant + TraderBot::getInstance()->getController()->getOrderDelay();
  Order* p_order = new Order(g_order_idx, target_price, type, direction, castExchange()->getExchangeID(), currency_pair,
                             amount, order_place_time, a_cur_price);
  acc.setAvailable(acc.getAvailable() - amount);
  acc.setHold(acc.getHold() + amount);

  if (m_all_order_queues.find(currency_pair) == m_all_order_queues.end())
    m_all_order_queues[currency_pair] = order_queues_t{};

  order_queues_t& order_queues = m_all_order_queues[currency_pair];

  if (p_order->getTiggerType() == trigger_type_t::MIN)
    order_queues.min_order_queue.push(p_order);
  else if (p_order->getTiggerType() == trigger_type_t::MAX)
    order_queues.max_order_queue.push(p_order);
  else
    order_queues.cur_order_queue.push(p_order);

  // dumpVirtualAccountStatus(time_instant, p_order);

  p_order->setOrderId(to_string(g_order_idx));
  return p_order->getOrderId();
}

void VirtualExchange::addVirtualAccount(const currency_t currency, const double amount) {
  if (!m_virtual_wallet) m_virtual_wallet = new ExchangeAccounts(castExchange()->getExchangeID(), true);

  m_virtual_wallet->addAccount(Account("Virtual" + Currency::sCurrencyToString(currency), currency, amount, 0.0));
}

void VirtualExchange::addVirtualAccount(const currency_t currency, const Account account) {
  if (!m_virtual_wallet) m_virtual_wallet = new ExchangeAccounts(castExchange()->getExchangeID(), true);

  m_virtual_wallet->addAccount(account);
}

void VirtualExchange::addWallet(const ExchangeAccounts* ap_wallet) {
  DELETE(m_virtual_wallet);
  m_virtual_wallet = new ExchangeAccounts(ap_wallet, true);
}

void VirtualExchange::addWallet(const vector<Account>& accounts) {
  DELETE(m_virtual_wallet);
  m_virtual_wallet = new ExchangeAccounts(castExchange()->getExchangeID(), accounts, true);
}

void VirtualExchange::executeOrders(const double a_vol_fraction, const CurrencyPair& currency_pair) {
  // TODO: only market and limit orders are supported right now

  const fee_t fee = castExchange()->getFee();

  if (m_all_order_queues.find(currency_pair) == m_all_order_queues.end()) return;

  order_queues_t& order_queues = m_all_order_queues[currency_pair];

  while (!order_queues.cur_order_queue.empty()) {
    Order* p_order = order_queues.cur_order_queue.top();

    if (p_order->getTime() > m_simulation_time) break;

    const TickPeriod* p_past_trade_period = getCurrentTradeHistory(p_order->getCurrencyPair())->getTickPeriod();
    assert(m_simulation_time == p_past_trade_period->back().getTimeStamp());

    executeMarketOrder(p_order, fee.taker_fee);
  }

  if (!order_queues.min_order_queue.empty()) {
    Order* p_order = order_queues.min_order_queue.top();

    const TickPeriod* p_past_trade_period = getCurrentTradeHistory(p_order->getCurrencyPair())->getTickPeriod();
    const Tick& cur_trade = p_past_trade_period->back();
    assert(m_simulation_time == cur_trade.getTimeStamp());

    executeLimitOrder(p_order, cur_trade, fee.maker_fee);

    if (p_order->getAmount() == 0) {
      // order fully executed
      order_queues.min_order_queue.pop();
      // DELETE(p_order);
    }
  }

  if (!order_queues.max_order_queue.empty()) {
    Order* p_order = order_queues.max_order_queue.top();

    const TickPeriod* p_past_trade_period = getCurrentTradeHistory(p_order->getCurrencyPair())->getTickPeriod();
    const Tick& cur_trade = p_past_trade_period->back();
    assert(m_simulation_time == cur_trade.getTimeStamp());

    executeLimitOrder(p_order, cur_trade, fee.maker_fee);

    if (p_order->getAmount() == 0) {
      // order fully executed
      order_queues.max_order_queue.pop();
      // DELETE(p_order);
    }
  }
}

void VirtualExchange::executeOrder(Order* ap_order, const double a_price, const double a_amount, const double a_fee) {
  assert((a_price > 0) && (a_amount > 0) && (a_fee >= 0));

  const CurrencyPair currency_pair = ap_order->getCurrencyPair();

  const double executed_amount = ((a_amount >= ap_order->getAmount()) ? ap_order->getAmount() : a_amount);

  Account& acc1 = ((ap_order->getOrderDir() == order_direction_t::BUY)
                       ? m_virtual_wallet->getAccount(currency_pair.getQuoteCurrency())
                       : m_virtual_wallet->getAccount(currency_pair.getBaseCurrency()));

  Account& acc2 = ((ap_order->getOrderDir() == order_direction_t::BUY)
                       ? m_virtual_wallet->getAccount(currency_pair.getBaseCurrency())
                       : m_virtual_wallet->getAccount(currency_pair.getQuoteCurrency()));

  const double adjusted_amount = ap_order->setExecute(executed_amount, a_price, m_virtual_wallet);

  const double increment =
      ((ap_order->getOrderDir() == order_direction_t::BUY) ? ((adjusted_amount / a_price) / (1 + a_fee))
                                                           : ((adjusted_amount * a_price) / (1 + a_fee)));

  acc1.setHold(acc1.getHold() - adjusted_amount);
  acc2.setAvailable(acc2.getAvailable() + increment);

  dumpVirtualAccountStatus(m_simulation_time, ap_order);
}

void VirtualExchange::executeMarketOrder(Order* ap_order, const double a_fee) {
  const TickPeriod* p_past_trade_period = getCurrentTradeHistory(ap_order->getCurrencyPair())->getTickPeriod();

  // TODO: calculate avg price using data of trades in past 1 min
  double avg_price = p_past_trade_period->back().getPrice();

  executeOrder(ap_order, avg_price, DBL_MAX, a_fee);

  // market order should get fully executed
  assert(ap_order->getAmount() == 0);
  m_all_order_queues[ap_order->getCurrencyPair()].cur_order_queue.pop();

  // DELETE(ap_order);
}

void VirtualExchange::executeLimitOrder(Order* ap_order, const Tick& a_trade, const double a_fee) {
  if (ap_order->getTime() >= a_trade.getTimeStamp()) return;

  double cur_vol = a_trade.getSize();

  // TODO: check if trade volume is 0
  if (cur_vol == 0) return;

  if (cur_vol < 0) cur_vol *= (-1);

  if (ap_order->getOrderDir() == order_direction_t::BUY) {
    if (a_trade.getSize() < 0) return;

    if (a_trade.getPrice() > ap_order->getTriggerPrice()) return;

    cur_vol *= ap_order->getTriggerPrice();
  } else {
    if (a_trade.getSize() > 0) return;

    if (a_trade.getPrice() < ap_order->getTriggerPrice()) return;
  }

  executeOrder(ap_order, ap_order->getTriggerPrice(), cur_vol, a_fee);
}

void VirtualExchange::dumpVirtualAccountStatus(const Time a_time, const Order* ap_order) {
  const TickPeriod* p_past_trade_period = getCurrentTradeHistory(ap_order->getCurrencyPair())->getTickPeriod();

  if (castExchange()->getMode() == exchange_mode_t::BOTH)
    COUT << CBLUE << Time::sNow();
  else
    COUT << CBLUE << a_time;

  COUT << CBLUE << " (" << p_past_trade_period->back().getUniqueID() << "): ";

  COUT << CCYAN << *m_virtual_wallet;

  ap_order->printLastStatus();

  COUT << endl;
}

void VirtualExchange::cancelLimitVirtualBuyOrders(const CurrencyPair& currency_pair) {
  if (m_all_order_queues.find(currency_pair) == m_all_order_queues.end()) return;

  order_queues_t& order_queues = m_all_order_queues[currency_pair];

  if (!order_queues.min_order_queue.empty()) {
    Order* p_order = order_queues.min_order_queue.top();
    assert((p_order->getOrderDir() == order_direction_t::BUY) && (p_order->getTiggerType() == trigger_type_t::MIN));
    cancelLimitVirtualOrder(p_order);

    order_queues.min_order_queue.pop();
  }
}

void VirtualExchange::cancelLimitVirtualSellOrders(const CurrencyPair& currency_pair) {
  if (m_all_order_queues.find(currency_pair) == m_all_order_queues.end()) return;

  order_queues_t& order_queues = m_all_order_queues[currency_pair];

  if (!order_queues.max_order_queue.empty()) {
    Order* p_order = order_queues.max_order_queue.top();
    assert((p_order->getOrderDir() == order_direction_t::SELL) && (p_order->getTiggerType() == trigger_type_t::MAX));
    cancelLimitVirtualOrder(p_order);

    order_queues.max_order_queue.pop();
  }
}

void VirtualExchange::cancelLimitVirtualOrder(Order* ap_order) {
  m_vir_cancelled_orders.push_back(ap_order);

  ap_order->cancelLimitOrder(m_virtual_wallet);
  // order cancelled
  assert(ap_order->getAmount() == 0);
  // DELETE(ap_order);
}

void VirtualExchange::cancelVirtualOrders(const CurrencyPair& currency_pair) {
  if (m_all_order_queues.find(currency_pair) == m_all_order_queues.end()) return;

  order_queues_t& order_queues = m_all_order_queues[currency_pair];

  // Let the virtual market orders get executed.
  while (!order_queues.cur_order_queue.empty()) {
    Order* p_order = order_queues.cur_order_queue.top();
    executeMarketOrder(p_order, castExchange()->getFee().taker_fee);
  }

  assert(order_queues.cur_order_queue.empty());

  cancelLimitVirtualSellOrders(currency_pair);
  cancelLimitVirtualBuyOrders(currency_pair);
}

void VirtualExchange::printOrders() {
  for (auto order_queue_iter : m_all_order_queues) {
    const CurrencyPair& currency_pair = order_queue_iter.first;
    order_queues_t& order_queues = order_queue_iter.second;

    auto cur_order_queue = order_queues.cur_order_queue;
    if (!cur_order_queue.empty())
      COUT << "=== " << currency_pair << " Market orders (" << cur_order_queue.size() << ") ===" << endl;
    while (!cur_order_queue.empty()) {
      cur_order_queue.top()->pr();
      cur_order_queue.pop();
    }

    auto min_order_queue = order_queues.min_order_queue;
    if (!min_order_queue.empty())
      COUT << "=== " << currency_pair << " Buy limit orders (" << min_order_queue.size() << ") ===" << endl;
    while (!min_order_queue.empty()) {
      min_order_queue.top()->pr();
      min_order_queue.pop();
    }

    auto max_order_queue = order_queues.max_order_queue;
    if (!max_order_queue.empty())
      COUT << "=== " << currency_pair << " Sell limit orders (" << max_order_queue.size() << ") ===" << endl;
    while (!max_order_queue.empty()) {
      max_order_queue.top()->pr();
      max_order_queue.pop();
    }
  }
}

const Order* VirtualExchange::getVirtualOrder(const order_id_t a_order_id) const {
  for (auto order_queue_iter : m_all_order_queues) {
    order_queues_t& order_queues = order_queue_iter.second;

    auto min_order_queue = order_queues.min_order_queue;
    while (!min_order_queue.empty()) {
      const Order* order = min_order_queue.top();
      if (order->getOrderId() == a_order_id) return order;
      min_order_queue.pop();
    }

    auto max_order_queue = order_queues.max_order_queue;
    while (!max_order_queue.empty()) {
      const Order* order = max_order_queue.top();
      if (order->getOrderId() == a_order_id) return order;
      max_order_queue.pop();
    }

    auto cur_order_queue = order_queues.cur_order_queue;
    while (!cur_order_queue.empty()) {
      const Order* order = cur_order_queue.top();
      if (order->getOrderId() == a_order_id) return order;
      cur_order_queue.pop();
    }
  }

  return NULL;
}

Quote VirtualExchange::getVirtualQuote(const CurrencyPair& a_currency_pair) {
  const TickPeriod* p_full_trade_period = getFullTradeHistory(a_currency_pair)->getTickPeriod();
  const TickPeriod* p_past_trade_period = getCurrentTradeHistory(a_currency_pair)->getTickPeriod();

  double quote_price = 0;
  double quote_vol = 0;

  bool quote_status = false;
  bool buy_quote = false;

  for (size_t tick_idx = p_past_trade_period->size(); tick_idx < p_full_trade_period->size(); ++tick_idx) {
    if (quote_status) break;

    const Tick& tick = *(p_full_trade_period->begin() + tick_idx);
    const double vol = tick.getSize();
    const double price = tick.getPrice();

    if (!vol) {
      CT_CRIT_WARN << "Zero volume tick : " << tick << endl;
      continue;
    }

    if (!quote_price) quote_price = price;

    if (quote_price == price) {
      if (vol > 0) {
        quote_vol += vol;
        buy_quote = true;
      } else
        quote_vol += ((-1) * vol);
    } else
      quote_status = true;
  }

  if (!quote_status) {
    // quote volume set 10^8

    if (!quote_price) {
      const Tick& tick = p_full_trade_period->back();

      // last tick taken as quote price
      quote_price = tick.getPrice();
      quote_vol = 1e8;
      buy_quote = (tick.getSize() > 0);
    } else
      quote_vol = 1e8;
  }

  const double spread = castExchange()->getSpread(a_currency_pair);

  // add/subtruct expected spread from quote price
  // bid and ask volumes are kept same.
  if (buy_quote) {
    const double askPrice = (ceil((quote_price + spread) * 1e2) / 1e2);
    return Quote(quote_price, quote_vol, askPrice, quote_vol);
  } else {
    const double bidPrice = (floor((quote_price - spread) * 1e2) / 1e2);
    return Quote(bidPrice, quote_vol, quote_price, quote_vol);
  }
}
