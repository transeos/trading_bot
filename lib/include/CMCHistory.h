//
// Created by subhagato on 2/26/18.
//

#ifndef CRYPTOTRADER_CMCHISTORY_H
#define CRYPTOTRADER_CMCHISTORY_H

#include "CandlePeriod.h"
#include <indicators/DiscreteIndicator.h>

#include <vector>

class CMCandleStick;
template <typename T>
class Database;

class CMCHistory {
 private:
  currency_t m_currency;
  std::map<Duration, CMCCandlePeriod*> m_candle_periods;
  std::map<Duration, DiscreteIndicatorA<CMCandleStick>*> m_indicators;
  Database<CMCandleStick>* m_db;

 public:
  explicit CMCHistory(const currency_t currency, const std::vector<Duration>& candlestick_intervals = {5_min});
  // by default 5 minute candlestick will be generated

  ~CMCHistory();

  currency_t getCurrency() const {
    return m_currency;
  }

  void restoreFromCSV(const std::string& a_csv_file_dir);
  void saveToCSV(const std::string& a_database_dir, const Time a_start_time, const Time a_end_time);

  bool appendCandle(const CMCandleStick& c);

  bool appendCandles(CMCCandlePeriod& candle_period);

  bool loadFromDatabase(Time start_time = Time(0),
                        Time end_time = Time::sNow());  // by default it will load all data from database

  void addCandlePeriod(Duration interval);

  bool addIndicator(Duration interval, DiscreteIndicatorA<CMCandleStick>* indicator);

  CMCCandlePeriod* getCandlePeriod(Duration interval = 5_min) const {
    if (m_candle_periods.find(interval) != m_candle_periods.end())
      return m_candle_periods.at(interval);
    else
      return nullptr;
  }

  Database<CMCandleStick>* getDb() const {
    return m_db;
  }

  void clearData();

  int64_t syncToDatabase();

  bool isEmpty() {
    return (m_candle_periods.at(5_min)->size() == 0);
  }

  CMCandleStick getLatestCandle(Duration interval = 5_min);
  CMCandleStick getOldestCandle(Duration interval = 5_min);

  std::string getFullSymbolStr(bool lower = false) const;
};

#endif  // CRYPTOTRADER_CMCHISTORY_H
