//
// Created by Subhagato Dutta on 9/20/17.
//

#ifndef CRYPTOTRADER_ACCOUNT_H
#define CRYPTOTRADER_ACCOUNT_H

#include "Currency.h"
#include "utils/StringUtils.h"
#include <iomanip>

class Account {
  std::string m_id;
  currency_t m_currency;
  double m_available;
  double m_hold;
  bool m_is_virtual;

 public:
  Account(std::string id, currency_t currency, double available, double hold)
      : m_id(std::move(id)), m_currency(currency), m_available(available), m_hold(hold), m_is_virtual(false) {
    assert((m_available >= 0) && (m_hold >= 0));
  }

  Account(std::string id, const std::string& currency, double available, double balance, double hold)
      : m_id(std::move(id)),
        m_currency(Currency::sStringToCurrency(currency)),
        m_available(available),
        m_hold(hold),
        m_is_virtual(false) {
    assert(balance == getBalance());
    assert((m_available >= 0) && (m_hold >= 0));
  }

  Account(const std::string& currency, double available, double hold = 0, bool virtual_acc = true)
      : m_currency(Currency::sStringToCurrency(currency)),
        m_available(available),
        m_hold(hold),
        m_is_virtual(virtual_acc) {
    if (virtual_acc) {
      m_id = "VIRTUAL-" + Currency::sCurrencyToString(m_currency) + "-" + StringUtils::generateRandomHEXstring(6);
    } else {
      m_id = StringUtils::generateRandomHEXstring(8) + "-" + StringUtils::generateRandomHEXstring(4) + "-" +
             StringUtils::generateRandomHEXstring(4) + "-" + StringUtils::generateRandomHEXstring(4) + "-" +
             StringUtils::generateRandomHEXstring(12);
    }

    assert((m_available >= 0) && (m_hold >= 0));
  }

  Account(const currency_t currency, double available, double hold = 0, bool virtual_acc = true)
      : Account(Currency::sCurrencyToString(currency), available, hold, virtual_acc) {}

  Account() {
    m_id = "";
    m_currency = currency_t::UNDEF;
    m_available = 0;
    m_hold = 0;
    m_is_virtual = false;
  }

  const std::string& getID() const {
    return m_id;
  }

  void setID(const std::string& m_id) {
    Account::m_id = m_id;
  }

  currency_t getCurrency() const {
    return m_currency;
  }

  void setCurrency(currency_t currency) {
    m_currency = currency;
  }

  void setCurrency(std::string currency) {
    m_currency = Currency::sStringToCurrency(currency);
  }

  double getAvailable() const {
    return m_available;
  }

  void setAvailable(double available) {
    assert(available >= 0);
    m_available = available;
  }

  double getBalance() const {
    return (m_available + m_hold);
  }

  double getHold() const {
    return m_hold;
  }

  void setHold(double hold) {
    assert(hold >= 0);
    m_hold = hold;
  }

  void saveHeaderInCSV(FILE* ap_file) const;
  void saveInCSV(FILE* ap_file) const;

  friend std::ostream& operator<<(std::ostream& os, const Account& account) {
    os << std::setprecision(4) << std::fixed;
    os << "[ " << Currency::sCurrencyToString(account.m_currency) << " : " << account.m_id
       << "] : balance: " << account.getBalance() << ", available: " << account.m_available
       << ", hold: " << account.m_hold << "";
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }
};

#endif  // CRYPTOTRADER_ACCOUNT_H
