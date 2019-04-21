//
// Created by Subhagato Dutta on 10/22/17.
//

#include "Order.h"
#include "Controller.h"
#include "ExchangeAccounts.h"
#include "TraderBot.h"

using namespace std;

Order::Order(const int a_order_id, double trigger_price, order_type_t order_type, order_direction_t order_direction,
             const exchange_t exchange_id, const CurrencyPair a_currency_pair, const double a_amount,
             const Time a_order_time, const double a_price_at_order_time)
    : m_idx(a_order_id),
      m_trigger_price(trigger_price),
      m_order_type(order_type),
      m_order_direction(order_direction),
      m_exchange_id(exchange_id),
      m_currency_pair(a_currency_pair),
      m_amount(a_amount),
      m_order_time(a_order_time),
      m_price_at_order_time(a_price_at_order_time) {
  assert((m_amount > 0) && (m_price_at_order_time > 0) &&
         ((m_trigger_price > 0) || (m_order_type == order_type_t::MARKET)));

  if ((order_direction == order_direction_t::BUY && order_type == order_type_t::LIMIT) ||
      (order_direction == order_direction_t::SELL && order_type == order_type_t::STOP)) {
    m_tigger_type = trigger_type_t::MIN;
  } else if ((order_direction == order_direction_t::SELL && order_type == order_type_t::LIMIT) ||
             (order_direction == order_direction_t::BUY && order_type == order_type_t::STOP)) {
    m_tigger_type = trigger_type_t::MAX;
  } else if (order_type == order_type_t::MARKET) {
    m_tigger_type = trigger_type_t::CURRENT;
  }

  m_discarded = false;
  m_cancelled_amount = 0;

  if (m_tigger_type == trigger_type_t::MIN)
    assert(m_trigger_price < m_price_at_order_time);
  else if (m_tigger_type == trigger_type_t::MAX)
    assert(m_trigger_price > m_price_at_order_time);

  m_order_id = "";
}

double Order::setExecute(const double amount, const double a_price, const ExchangeAccounts* a_wallet) {
  const Account& acc =
      ((m_order_direction == order_direction_t::BUY) ? a_wallet->getAccount(m_currency_pair.getQuoteCurrency())
                                                     : a_wallet->getAccount(m_currency_pair.getBaseCurrency()));

  assert(amount > 0);
  double adjusted_amount = amount;

  if (a_wallet->isVirtual()) {
    adjustAmount(acc.getHold());
    if (amount > m_amount) {
      assert((amount - m_amount) < 0.000001);
      adjusted_amount = m_amount;
    }

    assert(acc.getHold() >= adjusted_amount);
  } else {
    double leftover = (m_amount - adjusted_amount);
    if (leftover < 0) leftover *= (-1);

    if (((m_order_direction == order_direction_t::BUY) && (leftover < 0.0001)) ||
        ((m_order_direction == order_direction_t::SELL) && (leftover < 0.000001))) {
      adjusted_amount = m_amount;
    }
  }

  m_amount -= adjusted_amount;
  assert(m_amount >= 0);

  if (m_order_direction == order_direction_t::BUY)
    populate((adjusted_amount / a_price), a_price);
  else
    populate(adjusted_amount, a_price);

  return adjusted_amount;
}

void Order::cancelLimitOrder(ExchangeAccounts* a_wallet) {
  assert(m_order_type == order_type_t::LIMIT);

  Account& acc =
      ((m_order_direction == order_direction_t::BUY) ? a_wallet->getAccount(m_currency_pair.getQuoteCurrency())
                                                     : a_wallet->getAccount(m_currency_pair.getBaseCurrency()));

  adjustAmount(acc.getHold());

  if (acc.getHold() < 0.000001) return;

  assert(acc.getHold() >= m_amount);

  // update vitual accounts
  if (a_wallet->isVirtual()) {
    acc.setAvailable(acc.getAvailable() + m_amount);
    acc.setHold(acc.getHold() - m_amount);
  }

  m_cancelled_amount = m_amount;
  m_amount = 0;

  // printOrderSummary();
}

void Order::discard() {
  if (m_amount == 0) {
    // order has been executed
    return;
  }

  assert(!m_discarded);
  m_discarded = true;
}

void Order::adjustAmount(const double a_hold_amount) {
  // handle floating approximation
  if (a_hold_amount < m_amount) {
    double diff = (m_amount - a_hold_amount);
    assert(diff < 0.000001);
    m_amount = a_hold_amount;
  }
}

void Order::printLastStatus() const {
  COUT << m_currency_pair << " : ";
  if (m_order_exes.size() == 0) {
    if (m_order_direction == order_direction_t::BUY) {
      COUT << CMAGENTA << "buy";
      COUT << " order placed using " << m_amount << " "
           << Currency::sCurrencyToString(m_currency_pair.getQuoteCurrency());
    } else {
      COUT << CGREEN << "sell";
      COUT << " order placed using " << m_amount << " "
           << Currency::sCurrencyToString(m_currency_pair.getBaseCurrency());
    }

    COUT << " at " << m_price_at_order_time;

    if (m_trigger_price > 0) {
      COUT << "(trigger:" << m_trigger_price << ", Id:" << m_idx << ")";
    }
  } else {
    if (m_order_direction == order_direction_t::BUY) {
      COUT << CMAGENTA << "bought ";
    } else {
      COUT << CGREEN << "sold ";
    }

    order_exe_t last_trade_info = m_order_exes[m_order_exes.size() - 1];
    COUT << last_trade_info.volume << " at " << last_trade_info.price << " (Id:)" << m_idx;
  }
}

void Order::printOrderSummary() const {
  COUT << m_order_time << " : order place time : ";
  if (m_order_type == order_type_t::LIMIT) {
    COUT << "Limit";
  } else if (m_order_type == order_type_t::MARKET) {
    COUT << "Market";
  } else if (m_order_type == order_type_t::STOP) {
    COUT << "Stop";
  } else
    assert(0);

  COUT << " (Id: " << m_idx << ") : ";

  if (m_order_direction == order_direction_t::BUY) {
    COUT << CMAGENTA << "buy";
  } else {
    COUT << CGREEN << "sell";
  }

  COUT << " order placed at " << m_price_at_order_time;

  double total_amount = 0;
  double total_weighted_amount = 0;

  for (const auto& trade_info : m_order_exes) {
    total_amount += trade_info.volume;
    total_weighted_amount += (trade_info.volume * trade_info.price);
  }

  if ((m_order_direction == order_direction_t::BUY) && total_amount) {
    COUT << " using " << total_weighted_amount << " "
         << Currency::sCurrencyToString(m_currency_pair.getQuoteCurrency());
  } else if (total_amount) {
    COUT << " using " << total_amount << " " << Currency::sCurrencyToString(m_currency_pair.getBaseCurrency());
  }

  if (total_amount == 0) {
    assert(m_order_exes.size() == 0);
    COUT << " is " << CYELLOW << "cancelled" << CRESET << " now" << endl;
  } else {
    COUT << ". It is completed now." /* << (total_weighted_amount / total_amount)*/ << endl;
  }

  Controller* p_Controller = TraderBot::getInstance()->getController();
  if (p_Controller) {
    p_Controller->saveStatsSummaryInCSV();
  }
}

void Order::pr() const {
  cout << "Id: " << m_idx;
  cout << ", Tigger price: " << m_trigger_price;
  cout << ", Time: " << m_order_time << endl;
}
