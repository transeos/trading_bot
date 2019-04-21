// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Subhagato Dutta on 9/10/17
//
//*****************************************************************

#ifndef CRYPTOTRADER_COINMARKETCAP_H
#define CRYPTOTRADER_COINMARKETCAP_H

#ifndef COIN_MARKET_CAP_DISABLED

#include "CMCHistory.h"
#include "CMCandleStick.h"
#include "Currency.h"

class RestAPI2JSON;

class CoinMarketCap {
 private:
  static const std::unordered_map<currency_t, std::string> m_cmc_ids;
  std::unordered_map<currency_t, CMCHistory*> m_cryptos;
  RestAPI2JSON* m_api_handle;
  RestAPI2JSON* m_database_handle;

  std::unordered_map<currency_t, CMCandleStick> m_latest_Candles;

  int64_t fillCandles(CMCCandlePeriod& cp, currency_t currency, Time start_time, Time end_time);

  CMCandleStick getLatestCandle(currency_t currency);

 public:
  CoinMarketCap();
  ~CoinMarketCap();

  CMCandleStick getLatestCandleFromCache(const currency_t a_currency);

  bool repairDatabase(currency_t currency = currency_t::UNDEF);
  bool storeInitialCandles(currency_t currency = currency_t::UNDEF);  // UNDEF means update all currency
  bool storeRecentCandles(currency_t currency = currency_t::UNDEF);   // UNDEF means update all currency

  CMCHistory* getCMCHistory(currency_t currency);

  void saveToCSV(const std::string& a_exported_data_name, const Time a_start_time, const Time a_end_time,
                 const std::string& a_database_dir);

  void restoreFromCSV();
};

#endif

#endif  // CRYPTOTRADER_COINMARKETCAP_H
