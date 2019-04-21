//
// Created by Subhagato Dutta on 9/24/17.
//

#ifndef CRYPTOTRADER_TRADEHISTORY_H
#define CRYPTOTRADER_TRADEHISTORY_H

#include "CurrencyPair.h"
#include "Enums.h"
#include "indicators/DiscreteIndicator.h"
#include "utils/TimeUtils.h"

#include <iostream>
#include <map>
#include <set>
#include <vector>

class Tick;
class CoinAPITick;
class Candlestick;
template <typename T>
class Database;
template <typename T>
class TickPeriodT;
template <typename C, typename T>
class CandlePeriodT;
template <typename C, typename T>
class DiscreteIndicatorA;

template <typename T>
class TradeHistoryT {
 private:
  exchange_t m_exchange_id;
  CurrencyPair m_currency_pair;

  TickPeriodT<T>* m_tick_period;

  // candle sticks dependent data structures
  std::map<Duration, CandlePeriodT<Candlestick, T>*> m_candle_periods;
  std::map<Duration, DiscreteIndicatorA<Candlestick, T>*> m_indicators;
  std::map<Duration, FILE*> m_logs;

  Database<T>* m_db;

  bool m_ongoing_trading;

  // mutex
  mutable std::mutex m_new_tick;

  std::string createDir(const std::string& a_csv_file_dir) const;

  void closeLogs();

  void saveStatsInCSV(const Duration a_interval) const;

  void controllerCallBack() const;

 public:
  TradeHistoryT(
      const exchange_t exchange_id, const CurrencyPair currency_pair, const bool consecutive = true,
      const std::vector<Duration> candlestick_intervals = {});  // by default no candlesticks will be generated

  ~TradeHistoryT();

  exchange_t getExchangeId() const {
    return m_exchange_id;
  }
  const CurrencyPair& getCurrencyPair() const {
    return m_currency_pair;
  }

  void restoreFromCSV(const std::string& a_csv_file_dir);

  void saveToCSV(const std::string& a_database_dir, const Time a_start_time, const Time a_end_time);

  void saveTickDataToCSV(const std::string& a_csv_file_dir);

  void saveCandlestickDataToCSV(const std::string& a_csv_file_dir);

  void initLogs(const std::string& a_csv_file_dir);

  bool appendTrade(T& t);

  bool appendTrades(TickPeriodT<T>& tick_period);

  bool loadFromDatabase(Time start_time = Time(0),
                        Time end_time = Time::sNow());  // by default it will load all data from database

  void addCandlePeriod(Duration interval);

  void addCandlePeriod(const std::set<Duration>& a_intervals);

  bool addIndicator(Duration interval, DiscreteIndicatorA<Candlestick, T>* indicator);

  TickPeriodT<T>* getTickPeriod() const {
    return m_tick_period;
  }

  void populateNumCandles(std::map<Duration, int>& nunCandlesHash) const;
  int getNumCandles(Duration interval) const;

  Database<T>* getDb() const {
    return m_db;
  }

  void clearData();

  int64_t syncToDatabase();

  bool isEmpty() {
    return (m_tick_period->size() == 0);
  }

  T getLatestTick();

  std::string getFullSymbolStr(bool lower = false) const;

  void setOngoingTrading(const bool ongoing_trading) {
    m_ongoing_trading = ongoing_trading;
  }

  Candlestick getCandleFromBegin(Duration interval, const int candle_idx = 0) const;
  Candlestick getCandleFromEnd(Duration interval, const int candle_idx = 0) const;
  Candlestick getLastCandle(Duration interval, const Time a_cur_time, const int a_idx_from_back = 0) const;

  void printCandlePeriod(Duration interval) const;
};

typedef TradeHistoryT<Tick> TradeHistory;
typedef TradeHistoryT<CoinAPITick> CapiTradeHistory;

#endif  // CRYPTOTRADER_TRADEHISTORY_H
