// -*- C++ -*-
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 02/03/18.
//

#include "CoinAPIHistory.h"
#include "CMCHistory.h"
#include "CoinAPITick.h"
#include "CoinMarketCap.h"
#include "TraderBot.h"
#include "exchanges/Exchange.h"
#include "utils/RestAPI2JSON.h"

using namespace std;

CoinAPIHistory::CoinAPIHistory() {
  m_fixer_io_api_handle = new RestAPI2JSON("http://api.fixer.io", 1);

  // initialize last price update time
  m_conversions[currency_t::USDT] = conversion_t{Time(0), 1};
  m_conversions[currency_t::EUR] = conversion_t{Time(0), 1.23};
  m_conversions[currency_t::KRW] = conversion_t{Time(0), 0.00093};
  m_conversions[currency_t::JPY] = conversion_t{Time(0), 0.0095};
}

CoinAPIHistory::~CoinAPIHistory() {
  DELETE(m_fixer_io_api_handle);

  for (auto iter : m_trade_histories) {
    DELETE(iter.second);
  }
}

void CoinAPIHistory::clearData() {
  for (auto iter : m_trade_histories) iter.second->clearData();
}

void CoinAPIHistory::syncToDatabase() {
  for (auto iter : m_trade_histories) {
    CapiTradeHistory* p_trade_history = iter.second;
    ASSERT(p_trade_history);

    CapiTickPeriod* p_tick_period = p_trade_history->getTickPeriod();
    if (p_tick_period->size()) {
      COUT << CRED << Exchange::sExchangeToString(p_trade_history->getExchangeId());
      COUT << CGREEN << ": Saving " << CYELLOW << iter.first << CGREEN << " data from ";
      COUT << CGREEN << p_tick_period->front().getTimeStamp() << " to " << p_tick_period->back().getTimeStamp();
      COUT << endl;
    }

    p_trade_history->syncToDatabase();
    p_trade_history->clearData();
  }
}

void CoinAPIHistory::saveCandlestickDataToCSV(const string& a_csv_file_dir) {
  for (auto iter : m_trade_histories) iter.second->saveCandlestickDataToCSV(a_csv_file_dir);
}

void CoinAPIHistory::saveTickDataToCSV(const string& a_csv_file_dir) {
  for (auto iter : m_trade_histories) iter.second->saveTickDataToCSV(a_csv_file_dir);
}

void CoinAPIHistory::restoreFromCSV(const string& a_csv_file_dir) {
  for (auto iter : m_trade_histories) iter.second->restoreFromCSV(a_csv_file_dir);
}

bool CoinAPIHistory::saveToCSV(const string& a_exported_data_name, const Time a_start_time, const Time a_end_time,
                               const string& a_database_dir) {
  for (auto iter : m_trade_histories) {
    CapiTradeHistory* p_trade_history = iter.second;
    if (!p_trade_history) continue;

    if (p_trade_history->getFullSymbolStr() == a_exported_data_name) {
      p_trade_history->saveToCSV(a_database_dir, a_start_time, a_end_time);
      return true;
    }
  }
  return false;
}

bool CoinAPIHistory::appendTrade(const CoinAPITick& a_tick, const CurrencyPair a_currency_pair) {
  CurrencyPair modified_currency = a_currency_pair;
  CoinAPITick modified_tick = a_tick;

  switch (a_currency_pair.getQuoteCurrency()) {
    case currency_t::USD:
      break;
    case currency_t::USDT:
    case currency_t::EUR:
    case currency_t::KRW:
    case currency_t::JPY:
      modified_currency = CurrencyPair(a_currency_pair.getBaseCurrency(), currency_t::USD);
      modified_tick.multiplyPrice(getUSDConversionVal(a_currency_pair.getQuoteCurrency()));
      break;
    default:
      return false;  // TODO: ignore other quote currencies for now
  }

  CapiTradeHistory* p_trade_history = m_trade_histories[modified_currency];
  ASSERT(p_trade_history);

  bool success = p_trade_history->appendTrade(modified_tick);
  return success;
}

void CoinAPIHistory::addCandlePeriod(Duration a_interval) {
  for (auto iter : m_trade_histories) iter.second->addCandlePeriod(a_interval);
}

bool CoinAPIHistory::loadFromDatabase(Time start_time, Time end_time) {
  // clear previous data first
  clearData();

  for (auto iter : m_trade_histories) {
    iter.second->loadFromDatabase(start_time, end_time);
  }

  return true;
}

void CoinAPIHistory::addEntry(const CurrencyPair a_currency_pair) {
  const CurrencyPair modified_currency(a_currency_pair.getBaseCurrency(), currency_t::USD);

  CapiTradeHistory* p_trade_history = m_trade_histories[modified_currency];
  if (!p_trade_history) {
    p_trade_history = new CapiTradeHistory(exchange_t::COINAPI, modified_currency, false);
    m_trade_histories[modified_currency] = p_trade_history;
  }
}

double CoinAPIHistory::getUSDConversionVal(const currency_t a_currency) {
  TraderBot* p_trader_bot = TraderBot::getInstance();

  conversion_t& conversion_data = m_conversions[a_currency];

  bool cache_updated = false;
  Time last_update = conversion_data.last_updated;
  Time cur_time = Time::sNow();

  switch (a_currency) {
    case currency_t::USD:
      return 1;
    case currency_t::USDT: {
      if ((cur_time - last_update) > Duration(0, 0, 1, 0)) {
        // get price from coin market cap in every 1 min
        CoinMarketCap* p_cmc = p_trader_bot->getCoinMarketCap();

        const CMCandleStick candle_stick = p_cmc->getLatestCandleFromCache(currency_t::USDT);
        if (candle_stick != CMCandleStick()) {
          conversion_data.cached_price = candle_stick.getMean();
          cache_updated = true;
        }
      }
      break;
    }
    case currency_t::EUR:
    case currency_t::KRW:
    case currency_t::JPY: {
      if ((cur_time - last_update) > 12_hour) {
        // get price from fixer.io in every 12 hours

        json fiat_info;
        try {
          fiat_info = m_fixer_io_api_handle->getJSON_GET("/latest?base=USD");
        } catch (...) {
          CT_CRIT_WARN << "invalid json response from fixer.io\n";
          this_thread::sleep_for(chrono::milliseconds(10));
          break;
        }

        double usd_in_cur_currency = fiat_info["rates"][Currency::sCurrencyToString(a_currency)];
        conversion_data.cached_price = (1 / usd_in_cur_currency);
        cache_updated = true;
      }
      break;
    }
    default:
      assert(0);
  }

  if (cache_updated) {
    conversion_data.last_updated = cur_time;

    COUT << CMAGENTA << Currency::sCurrencyToString(a_currency);
    COUT << " price updated to ";
    COUT << CMAGENTA << "$" << conversion_data.cached_price << endl;
  }

  return conversion_data.cached_price;
}
