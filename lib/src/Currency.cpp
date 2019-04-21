//
// Created by Subhagato Dutta on 9/14/17.
//

#include "Currency.h"
#include "utils/JsonUtils.h"
#include "utils/TraderUtils.h"

using namespace std;

string Currency::sCurrencyToString(const currency_t currency, bool lower) {
  const int offset = static_cast<int>(currency_t::UNDEF);

  const int currency_val = static_cast<const int>(currency);
  assert((currency_val >= offset) && ((currency_val - offset) < static_cast<int64_t>(g_currency_strs.size())));

  string c_str = g_currency_strs[currency_val - offset];

  if (lower) transform(c_str.begin(), c_str.end(), c_str.begin(), ::tolower);

  return c_str;
}

currency_t Currency::sStringToCurrency(string currency) {
  const int offset = static_cast<int>(currency_t::UNDEF);

  currency_t retCur = currency_t::UNDEF;
  transform(currency.begin(), currency.end(), currency.begin(), ::toupper);

  for (int idx = (g_currency_strs.size() - 1); idx > -1; --idx) {
    if (g_currency_strs[idx] == currency) {
      retCur = static_cast<currency_t>(idx + offset);
      break;
    }
  }

  return retCur;
}

double Currency::getValueUSD() {
  // TODO: add support for top 20 coins

  switch (m_currency) {
    case currency_t::GBP:
      m_usd_conv = 1.0;
      break;
    case currency_t::EUR:
      m_usd_conv = 1.0;
      break;
    case currency_t::USD:
      m_usd_conv = 1.0;
      break;
    case currency_t::INR:
      m_usd_conv = 1.0;
      break;
    case currency_t::CNY:
      m_usd_conv = 1.0;
      break;
    case currency_t::BTC:
      m_usd_conv = 1.0;
      break;
    case currency_t::BCH:
      m_usd_conv = 1.0;
      break;
    case currency_t::LTC:
      m_usd_conv = 1.0;
      break;
    case currency_t::ETH:
      m_usd_conv = 1.0;
      break;
    case currency_t::ZEC:
      m_usd_conv = 1.0;
      break;
    case currency_t::DASH:
      m_usd_conv = 1.0;
      break;
    case currency_t::XMR:
      m_usd_conv = 1.0;
      break;
    case currency_t::UNDEF:
      m_usd_conv = 1.0;
      break;
    default:
      m_usd_conv = 1.0;
  }

  return m_usd_conv;
}
