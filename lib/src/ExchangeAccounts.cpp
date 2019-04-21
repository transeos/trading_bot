// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 21/01/18
//
//*****************************************************************

#include "ExchangeAccounts.h"
#include "Controller.h"
#include "TraderBot.h"

using namespace std;

double ExchangeAccounts::getUSDPrice(const currency_t a_currency) const {
  if (a_currency == currency_t::USD) return 1;

  const Controller* p_Controller = TraderBot::getInstance()->getController();
  if (!p_Controller) {
    // Controller not created yet
    return -1;
  }

  double cur_price = p_Controller->getCurrentPrice(m_exchange_id, a_currency);
  return cur_price;
}

void ExchangeAccounts::addAccount(const Account& account) {
  assert(!checkIfAccountExist(account.getCurrency()));
  m_accounts[account.getCurrency()] = account;
}

void ExchangeAccounts::removeAccount(const currency_t a_currency) {
  assert(checkIfAccountExist(a_currency));
  m_accounts.erase(a_currency);
}

Account& ExchangeAccounts::getAccount(const currency_t a_currency) {
  assert(checkIfAccountExist(a_currency));
  return m_accounts[a_currency];
}

const Account& ExchangeAccounts::getAccount(const currency_t a_currency) const {
  assert(checkIfAccountExist(a_currency));
  return m_accounts.at(a_currency);
}

double ExchangeAccounts::getUSDValue() const {
  double dollar_value = 0;

  for (auto& account_iter : m_accounts) {
    const currency_t currency = account_iter.first;

    double cur_price = getUSDPrice(currency);
    if (cur_price < 0) {
      // Controller price is yet not initialized
      return -1;
    }

    const double acc_share = getAccountShare(currency);
    dollar_value += (account_iter.second.getBalance() * cur_price * acc_share);
  }

  return dollar_value;
}

bool ExchangeAccounts::checkIfAccountsExist(vector<currency_t>& a_currencies) {
  for (auto currency : a_currencies) {
    if (!checkIfAccountExist(currency)) return false;
  }
  return true;
}

void ExchangeAccounts::modifyAccount(const Account& account) {
  m_accounts[account.getCurrency()] = account;
}

void ExchangeAccounts::processShares(const json& a_json) {
  if (a_json == json()) return;

  auto json_iter = a_json.find("account_share");
  if (json_iter == a_json.end()) return;

  for (auto& account_json : json_iter->items()) {
    string currency_str = account_json.key();
    double share = account_json.value().get<double>();

    if (share > 1) ACCOUNT_SHARE_ERROR(a_json);

    m_shares[Currency::sStringToCurrency(currency_str)] = share;
  }
}

double ExchangeAccounts::getAccountShare(const currency_t a_currency) const {
  if (m_shares.find(a_currency) == m_shares.end()) return 1;

  return m_shares.at(a_currency);
}

ostream& operator<<(ostream& os, const ExchangeAccounts& wallet) {
  for (auto& acc_iter : wallet.m_accounts) {
    const currency_t currency = acc_iter.first;
    const Account& acc = acc_iter.second;
    ASSERT(currency == acc.getCurrency());

    os << "[";

    if (wallet.m_is_virtual)
      os << acc.getID();
    else
      os << acc.getID().substr(0, 13) << "]"
         << "][" << Currency::sCurrencyToString(currency);

    os << "] ";
    os << setw(7) << acc.getBalance();
    /*os << "(" << acc.getAvailable() << ")*/

    if (wallet.getAccountShare(currency) != 1) os << " (" << (wallet.getAccountShare(currency) * 100) << "% share)";
    os << ", ";
  }

  os << "Total: ";
  os << setw(7) << wallet.getUSDValue();
  os << ", ";

  return os;
}

void ExchangeAccounts::printDetails() const {
  if (m_is_virtual)
    COUT << "virtual";
  else
    COUT << "real";

  COUT << " ExchangeAccounts in " << Exchange::sExchangeToString(m_exchange_id) << endl;

  for (auto& acc_iter : m_accounts) {
    const currency_t currency = acc_iter.first;
    const Account& acc = acc_iter.second;
    ASSERT(currency == acc.getCurrency());

    COUT << "\t[" << acc.getID() << "]" << CBLUE << "[" << Currency::sCurrencyToString(currency) << "] ";
    COUT << "balance : " << acc.getBalance();
    COUT << ", available : " << acc.getAvailable();
    COUT << ", hold : " << acc.getHold();

    if (getAccountShare(currency) != 1) COUT << " , account share : " << (getAccountShare(currency) * 100) << "%";

    COUT << endl;
  }

  double cur_value = getUSDValue();
  if (cur_value > 0) COUT << "Current value : " << cur_value << endl;
}

void ExchangeAccounts::saveHeaderInCSV(FILE* ap_file) const {
  fprintf(ap_file, ",%s_portfilio($)", Exchange::sExchangeToString(m_exchange_id).c_str());

  for (auto& acc_iter : m_accounts) {
    const currency_t currency = acc_iter.first;
    if (currency == currency_t::USD) continue;
    fprintf(ap_file, ",%s_portfilio(%s)", Exchange::sExchangeToString(m_exchange_id).c_str(),
            Currency::sCurrencyToString(currency).c_str());
  }

  for (auto& acc_iter : m_accounts) {
    const currency_t currency = acc_iter.first;
    const Account& acc = acc_iter.second;
    ASSERT(currency == acc.getCurrency());

    acc.saveHeaderInCSV(ap_file);

    if (getAccountShare(currency) != 1) fprintf(ap_file, ",acc_share_percent");
  }
}

void ExchangeAccounts::saveInCSV(FILE* ap_file) const {
  const double portfolio_val = getUSDValue();
  fprintf(ap_file, ",%lf", portfolio_val);

  for (auto& acc_iter : m_accounts) {
    const currency_t currency = acc_iter.first;
    if (currency == currency_t::USD) continue;
    fprintf(ap_file, ",%lf", (portfolio_val / getUSDPrice(currency)));
  }

  for (auto& acc_iter : m_accounts) {
    const currency_t currency = acc_iter.first;
    const Account& acc = acc_iter.second;
    ASSERT(currency == acc.getCurrency());

    acc.saveInCSV(ap_file);

    if (getAccountShare(currency) != 1) fprintf(ap_file, ",%lf", (getAccountShare(currency) * 100));
  }
}
