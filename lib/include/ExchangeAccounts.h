//
// Created by subhagato on 12/4/17.
//

#ifndef CRYPTOTRADER_WALLET_H
#define CRYPTOTRADER_WALLET_H

#include "Account.h"
#include "Enums.h"
#include "Globals.h"
#include <nlohmann/json.hpp>
#include <unordered_map>

using namespace nlohmann;

class ExchangeAccounts {
 private:
  std::unordered_map<currency_t, Account> m_accounts;
  std::unordered_map<currency_t, double> m_shares;

  exchange_t m_exchange_id;
  bool m_is_virtual;

  void processShares(const json& a_json);

 public:
  ExchangeAccounts(exchange_t a_exchange_id, bool is_virtual = false, const json& a_json = json())
      : m_exchange_id(a_exchange_id), m_is_virtual(is_virtual) {
    processShares(a_json);
  }

  ExchangeAccounts(exchange_t a_exchange_id, const std::vector<Account>& accounts, bool is_virtual = false)
      : m_exchange_id(a_exchange_id), m_is_virtual(is_virtual) {
    for (auto& account : accounts) m_accounts.insert(std::make_pair(account.getCurrency(), account));
  }

  // considers account share
  ExchangeAccounts(const ExchangeAccounts* ap_wallet, bool is_virtual = false) : m_is_virtual(is_virtual) {
    m_exchange_id = ap_wallet->m_exchange_id;

    for (auto account_iter : ap_wallet->m_accounts) {
      const currency_t currency = account_iter.first;
      const Account& account = account_iter.second;
      ASSERT(currency == account.getCurrency());

      m_accounts[account.getCurrency()] =
          Account(account.getCurrency(), (account.getAvailable() * ap_wallet->getAccountShare(currency)),
                  (account.getHold() * ap_wallet->getAccountShare(currency)), m_is_virtual);
    }
  }

  ~ExchangeAccounts() {}

  bool isVirtual() const {
    return m_is_virtual;
  }

  exchange_t getExchangeId() const {
    return m_exchange_id;
  }

  double getUSDPrice(const currency_t a_currency) const;

  void addAccount(const Account& account);

  void modifyAccount(const Account& account);  // if doesn't exist, new account will be created

  void removeAccount(const currency_t a_currency);

  Account& getAccount(const currency_t a_currency);

  const Account& getAccount(const currency_t a_currency) const;

  double getUSDValue() const;

  bool checkIfAccountExist(const currency_t a_currency) const {
    const auto acc_iter = m_accounts.find(a_currency);
    return (acc_iter != m_accounts.end());
  }
  bool checkIfAccountsExist(std::vector<currency_t>& a_currencies);

  double getAccountShare(const currency_t a_currency) const;

  void printDetails() const;

  void saveHeaderInCSV(FILE* ap_file) const;
  void saveInCSV(FILE* ap_file) const;

  friend std::ostream& operator<<(std::ostream& os, const ExchangeAccounts& wallet);

  void print() const {
    TradeUtils::displayT(*this);
  }
};

#endif  // CRYPTOTRADER_WALLET_H
