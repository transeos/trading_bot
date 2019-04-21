// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 01/03/18.
//

#ifndef COINAPI_HISTORY_H
#define COINAPI_HISTORY_H

#include "CandlePeriod.h"
#include "Globals.h"
#include "TickPeriod.h"
#include "TradeHistory.h"

class RestAPI2JSON;
class CoinAPITick;

class CoinAPIHistory {
 private:
  RestAPI2JSON* m_fixer_io_api_handle;

  std::unordered_map<CurrencyPair, CapiTradeHistory*> m_trade_histories;

  std::unordered_map<currency_t, conversion_t> m_conversions;

  double getUSDConversionVal(const currency_t a_currency);

 public:
  CoinAPIHistory();
  ~CoinAPIHistory();

  void addEntry(const CurrencyPair a_currency_pair);

  void restoreFromCSV(const std::string& a_csv_file_dir);
  bool saveToCSV(const std::string& a_exported_data_name, const Time a_start_time, const Time a_end_time,
                 const std::string& a_database_dir);

  void saveTickDataToCSV(const std::string& a_csv_file_dir);
  void saveCandlestickDataToCSV(const std::string& a_csv_file_dir);

  bool appendTrade(const CoinAPITick& a_tick, const CurrencyPair a_currency_pair);

  // by default it will load all data from database
  bool loadFromDatabase(Time start_time = Time(0), Time end_time = Time::sNow());

  void addCandlePeriod(Duration a_interval);

  const CapiTickPeriod* getTickPeriod(const CurrencyPair& a_currency_pair) const {
    const CapiTradeHistory* p_trade_history = m_trade_histories.at(a_currency_pair);
    ASSERT(p_trade_history);
    return p_trade_history->getTickPeriod();
  }

  void printCandlePeriod(const CurrencyPair& currency_pair, Duration interval) const {
    const CapiTradeHistory* p_trade_history = m_trade_histories.at(currency_pair);
    ASSERT(p_trade_history);
    p_trade_history->printCandlePeriod(interval);
  }

  int getNumCandles(const CurrencyPair& currency_pair, Duration interval) const {
    const CapiTradeHistory* p_trade_history = m_trade_histories.at(currency_pair);
    ASSERT(p_trade_history);
    return p_trade_history->getNumCandles(interval);
  }

  void clearData();

  void syncToDatabase();
};

#endif  // COINAPI_HISTORY_H
