//
// Created by Subhagato Dutta on 9/24/17.
//

#include "TradeHistory.h"
#include "Candlestick.h"
#include "CoinAPITick.h"
#include "Controller.h"
#include "Database.h"
#include "Tick.h"
#include "TraderBot.h"
#include "exchanges/Exchange.h"
#include "indicators/MA.h"

#include <cmath>
#include <vector>

using namespace std;

template <typename T>
TradeHistoryT<T>::TradeHistoryT(const exchange_t exchange_id, const CurrencyPair currency_pair, const bool consecutive,
                                const vector<Duration> candlestick_intervals)
    : m_exchange_id(exchange_id), m_currency_pair(currency_pair) {
  m_db = new Database<T>("crypto", exchange_id, currency_pair, consecutive);

#if 0
  // delete table

  if (g_update_cass && g_cass_session)
  {
      m_db->deleteTable();
      CT_CRIT_WARN<<"All data of " << getFullSymbolStr << " TradeHistoryT is deleted\n";
  }
#endif
  m_db->createTable();  // create table if not exists

  for (auto& interval : candlestick_intervals) {
    m_candle_periods.insert(make_pair(interval, new CandlePeriodT<Candlestick, T>(interval)));
  }

  m_tick_period = new TickPeriodT<T>(consecutive);

  m_ongoing_trading = false;
}

template <typename T>
TradeHistoryT<T>::~TradeHistoryT() {
  clearData();
  DELETE(m_db);

  DELETE(m_tick_period);

  for (auto candle_period : m_candle_periods) {
    DELETE(candle_period.second)
  }

  // indicators are deleted inside trade algo

  closeLogs();
}

template <typename T>
string TradeHistoryT<T>::getFullSymbolStr(bool lower) const {
  string symbol = m_currency_pair.toString("-", lower) + "_" + Exchange::sExchangeToString(m_exchange_id, lower);

  return symbol;
}

template <typename T>
void TradeHistoryT<T>::clearData() {
  m_tick_period->clear();

  for (auto& candle_period : m_candle_periods) {
    candle_period.second->clear();
  }

  m_indicators.clear();
}

template <typename T>
T TradeHistoryT<T>::getLatestTick() {
  return m_tick_period->back();
}

template <typename T>
int64_t TradeHistoryT<T>::syncToDatabase() {
  return m_tick_period->storeToDatabase(m_db);
}

template <typename T>
void TradeHistoryT<T>::saveCandlestickDataToCSV(const string& a_csv_file_dir) {
  const string dir = createDir(a_csv_file_dir);

  for (auto& candle_period : m_candle_periods) {
    Duration duration = candle_period.first;
    CandlePeriodT<Candlestick, T>* p_candle = candle_period.second;

    const string filename =
        dir + "/" + m_currency_pair.toString() + "_candlestick_" + to_string(duration.getDurationInSec()) + "_sec.csv";

    FILE* p_file = fopen(filename.c_str(), "w");
    ASSERT(p_file);

    p_candle->saveToCSV(p_file);

    fclose(p_file);
  }
}

template <typename T>
void TradeHistoryT<T>::saveTickDataToCSV(const string& a_csv_file_dir) {
  const string dir = createDir(a_csv_file_dir);

  const string filename = dir + "/" + m_currency_pair.toString() + ".csv";

  FILE* p_file = fopen(filename.c_str(), "w");

  ASSERT(p_file);

  m_tick_period->saveToCSV(p_file);

  fclose(p_file);
}

template <typename T>
void TradeHistoryT<T>::initLogs(const string& a_csv_file_dir) {
  const string dir = createDir(a_csv_file_dir);

  const Controller* p_Controller = TraderBot::getInstance()->getController();
  if (!p_Controller) {
    // Controller not created yet
    NO_CONTROLLER_TH_INIT_ERROR;
  }

  for (auto iter : m_candle_periods) {
    const Duration interval = iter.first;

    const string filename =
        dir + "/" + m_currency_pair.toString() + "_logs_" + to_string(interval.getDurationInSec()) + "_sec.csv";

    FILE* p_file = fopen(filename.c_str(), "w");
    m_logs[interval] = p_file;

    // initial header of controller (example - account)
    p_Controller->saveHeaderInCSV(p_file, m_exchange_id);

    // candle stick header
    iter.second->saveHeaderToCSV(p_file, false);

    if (m_indicators.find(interval) != m_indicators.end()) {
      // indicator headers
      m_indicators.at(interval)->saveHeaderToCSV(p_file, false);
    }

    // completion of 1st line
    fprintf(p_file, "\n");

    fflush(p_file);
  }
}

template <typename T>
void TradeHistoryT<T>::closeLogs() {
  for (auto iter : m_logs) fclose(iter.second);

  m_logs.clear();
}

template <typename T>
void TradeHistoryT<T>::saveStatsInCSV(const Duration a_interval) const {
  const auto p_candle_period = m_candle_periods.at(a_interval);

  Controller* p_Controller = TraderBot::getInstance()->getController();
  if (!p_Controller || (p_Controller->getPortfolioValue(m_exchange_id) < 0)) return;

  if (p_candle_period->size() < 2) return;

  if (m_logs.find(a_interval) == m_logs.end()) return;

  FILE* p_file = m_logs.at(a_interval);
  assert(p_file);

  p_Controller->saveStatsInCSV(p_file, m_exchange_id);

  auto& matured_candle_stick = *(p_candle_period->begin() + p_candle_period->size() - 2);
  DbUtils::writeToCSVLine(p_file, matured_candle_stick, false);

  if (m_indicators.find(a_interval) != m_indicators.end()) {
    // save indicators
    m_indicators.at(a_interval)->saveToCSV(p_file, false);
  }

  // completion of line
  fprintf(p_file, "\n");

  fflush(p_file);
}

template <typename T>
void TradeHistoryT<T>::restoreFromCSV(const string& a_csv_file_dir) {
  const string filename =
      a_csv_file_dir + "/" + Exchange::sExchangeToString(m_exchange_id) + "/" + m_currency_pair.toString() + ".csv";

  m_db->restoreDataFromCSVFile(filename);
}

template <typename T>
void TradeHistoryT<T>::saveToCSV(const string& a_database_dir, const Time a_start_time, const Time a_end_time) {
  const string dir = createDir(a_database_dir);

  const string filename = dir + "/" + m_currency_pair.toString() + ".csv";

  m_db->saveDataToCSVFile(filename, a_start_time, a_end_time);
}

template <typename T>
bool TradeHistoryT<T>::appendTrade(T& t) {
  Controller* p_Controller = TraderBot::getInstance()->getController();
  if (m_ongoing_trading) {
    ASSERT(p_Controller);
    p_Controller->LockTrading();
  }

  // lock tick based data
  m_new_tick.lock();

  int t_result = m_tick_period->append(t);

  if (t_result == 1) {
    for (auto& candle_period : m_candle_periods) {
      int c_result = candle_period.second->appendTick(t);
      int num_candles = candle_period.second->size();
      Time last_candle_timestamp = candle_period.second->back().getTimeStamp();

      if (c_result == 2 && num_candles > 1)  // new candlestick created
      {
        Duration interval = candle_period.first;

        // save controller stats
        saveStatsInCSV(interval);

        if (m_indicators.find(interval) != m_indicators.end()) {
          // add all the the mature candlesticks to indicators
          int64_t num_candles_to_add =
              (last_candle_timestamp - m_indicators[interval]->getLastTimeStamp()) / interval - 1;
          for (int64_t i = (num_candles_to_add + 1); i > 1; i--) {
            m_indicators[interval]->append(candle_period.second->at(num_candles - i));  // add the second latest candle
          }
        }
      }
    }
  }

  // unlock tick based data
  m_new_tick.unlock();

  if (m_ongoing_trading) p_Controller->UnlockTrading();

  if (t_result == -1) return false;

  if (t_result == 1) controllerCallBack();

  return true;
}

template <>
void TradeHistoryT<Tick>::controllerCallBack() const {
  if (m_ongoing_trading) {
    Controller* p_Controller = TraderBot::getInstance()->getController();
    ASSERT(p_Controller);

    p_Controller->doRealTimeTrading(this);
  }
}

template <>
void TradeHistoryT<CoinAPITick>::controllerCallBack() const {
  assert(!m_ongoing_trading);
}

template <typename T>
bool TradeHistoryT<T>::appendTrades(TickPeriodT<T>& tick_period) {
  int result;

  for (auto& t : tick_period) {
    result = this->appendTrade(t);

    if (!result) {
      CT_WARN << "Unable to add trades to TradeHistoryT\n";
      return false;
    }
  }

  return true;
}

template <typename T>
void TradeHistoryT<T>::addCandlePeriod(Duration interval) {
  if (m_candle_periods.find(interval) == m_candle_periods.end()) {
    CandlePeriodT<Candlestick, T>* candle_period = new CandlePeriodT<Candlestick, T>(interval);
    m_candle_periods.insert(make_pair(interval, candle_period));
    candle_period->convertFrom(*m_tick_period);
  }
}

template <typename T>
void TradeHistoryT<T>::addCandlePeriod(const std::set<Duration>& a_intervals) {
  for (auto duration : a_intervals) addCandlePeriod(duration);
}

template <typename T>
bool TradeHistoryT<T>::loadFromDatabase(Time start_time, Time end_time) {
  if (m_tick_period->size() > 0) {
    clearData();
  }

  m_tick_period->loadFromDatabase(m_db, start_time, end_time);

  for (auto& candle_period : m_candle_periods) {
    candle_period.second->convertFrom(*m_tick_period);  // convert tick to candles
  }

  return true;
}

template <typename T>
bool TradeHistoryT<T>::addIndicator(Duration interval, DiscreteIndicatorA<Candlestick, T>* indicator) {
  if (m_candle_periods.find(interval) == m_candle_periods.end()) {
    CT_CRIT_WARN << "CandlePeriod for this duration doesn't exist. Can't create indicator.\n";
    return false;
  }

  assert(indicator->getInterval() == interval);

  indicator->constructFrom(*m_candle_periods[interval]);

  m_indicators.insert(make_pair(interval, indicator));

  return true;
}

template <typename T>
string TradeHistoryT<T>::createDir(const string& a_csv_file_dir) const {
  const string dir = a_csv_file_dir + "/" + Exchange::sExchangeToString(m_exchange_id);
  TradeUtils::createDir(dir);
  return dir;
}

template <typename T>
Candlestick TradeHistoryT<T>::getCandleFromBegin(Duration interval, const int candle_idx) const {
  // lock tick based data
  m_new_tick.lock();

  auto candle_period = m_candle_periods.at(interval);
  const Candlestick candle = candle_period->at(candle_idx);

  // unlock tick based data
  m_new_tick.unlock();

  return candle;
}

template <typename T>
Candlestick TradeHistoryT<T>::getCandleFromEnd(Duration interval, const int candle_idx) const {
  // lock tick based data
  m_new_tick.lock();

  auto candle_period = m_candle_periods.at(interval);
  const Candlestick candle = candle_period->at(candle_period->size() - candle_idx - 1);

  // unlock tick based data
  m_new_tick.unlock();

  return candle;
}

template <typename T>
Candlestick TradeHistoryT<T>::getLastCandle(Duration interval, const Time a_cur_time, const int a_idx_from_back) const {
  // lock tick based data
  m_new_tick.lock();

  assert(m_candle_periods.find(interval) != m_candle_periods.end());
  auto candle_period = m_candle_periods.at(interval);

  const Candlestick& last_candle = candle_period->back();

  int num_missing_candles = ((a_cur_time - last_candle.getTimeStamp() - 1L) / interval);
  if (num_missing_candles > a_idx_from_back) {
    Candlestick cs;
    cs.setTimeStamp(last_candle.getTimeStamp() + (interval * num_missing_candles));
    cs.setPrice(candle_period->back().getClose());

    // unlock tick based data
    m_new_tick.unlock();

    return cs;
  }

  const int candle_idx = (candle_period->size() + num_missing_candles - a_idx_from_back - 1);
  const Candlestick candle = candle_period->at(candle_idx);

  // unlock tick based data
  m_new_tick.unlock();

  return candle;
}

template <typename T>
void TradeHistoryT<T>::populateNumCandles(map<Duration, int>& nunCandlesHash) const {
  // lock tick based data
  m_new_tick.lock();

  for (auto iter : m_candle_periods) {
    Duration duration = iter.first;
    auto candle_period = iter.second;

    nunCandlesHash[duration] = candle_period->size();
  }

  // unlock tick based data
  m_new_tick.unlock();
}

template <typename T>
int TradeHistoryT<T>::getNumCandles(Duration interval) const {
  // lock tick based data
  m_new_tick.lock();

  ASSERT(m_candle_periods.find(interval) != m_candle_periods.end());
  int numCandle = m_candle_periods.at(interval)->size();

  // unlock tick based data
  m_new_tick.unlock();

  return numCandle;
}

template <typename T>
void TradeHistoryT<T>::printCandlePeriod(Duration interval) const {
  // lock tick based data
  m_new_tick.lock();

  ASSERT(m_candle_periods.find(interval) != m_candle_periods.end());
  m_candle_periods.at(interval)->print();

  // unlock tick based data
  m_new_tick.unlock();
}

template class TradeHistoryT<Tick>;
template class TradeHistoryT<CoinAPITick>;
