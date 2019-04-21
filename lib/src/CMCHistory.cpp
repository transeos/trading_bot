//
// Created by subhagato on 2/26/18.
//

#include "CMCHistory.h"
#include "CMCandleStick.h"
#include "Database.h"
#include "Tick.h"
#include "TickPeriod.h"
#include "exchanges/Exchange.h"

#include <vector>

using namespace std;

CMCHistory::CMCHistory(const currency_t currency, const vector<Duration>& candlestick_intervals) {
  m_currency = currency;

  m_db = new Database<CMCandleStick>("crypto", getFullSymbolStr(true));

#if 0
    // delete table

	if (g_update_cass && g_cass_session)
	{
		m_db->deleteTable();
		WARNING<<"All data of " << getFullSymbolStr() << " CoinMarketCap is deleted\n";
	}
#endif
  m_db->createTable();  // create table if not exists

  for (auto& interval : candlestick_intervals)
    m_candle_periods.insert(make_pair(interval, new CMCCandlePeriod(interval)));
}

string CMCHistory::getFullSymbolStr(bool lower) const {
  string symbol = Currency::sCurrencyToString(m_currency, lower) + "_" +
                  Exchange::sExchangeToString(exchange_t::COINMARKETCAP, lower);
  return symbol;
}

bool CMCHistory::appendCandle(const CMCandleStick& c) {
  int result = m_candle_periods[5_min]->append(c);

  if (result == 1) {
    for (auto& candle_period : m_candle_periods) {
      int c_result;
      size_t num_candles = candle_period.second->size();

      if (candle_period.first != 5_min)
        c_result = candle_period.second->appendSmallerCandle(c, 5_min);
      else
        c_result = 2;

      if (c_result == 2 && num_candles > 1) {
        m_indicators[candle_period.first]->append(
            candle_period.second->at(num_candles - 2));  // add the second latest candle
      }
    }

    return true;
  } else {
    return (result == 0);
  }
}

void CMCHistory::restoreFromCSV(const string& a_csv_file_dir) {
  const string filename = a_csv_file_dir + "/" + Exchange::sExchangeToString(exchange_t::COINMARKETCAP) + "/" +
                          Currency::sCurrencyToString(m_currency) + ".csv";

  m_db->restoreDataFromCSVFile(filename);
}

void CMCHistory::saveToCSV(const string& a_database_dir, const Time a_start_time, const Time a_end_time) {
  // create directories
  const string dir = a_database_dir + "/" + Exchange::sExchangeToString(exchange_t::COINMARKETCAP);
  TradeUtils::createDir(dir);

  const string file = dir + "/" + Currency::sCurrencyToString(m_currency) + ".csv";

  m_db->saveDataToCSVFile(file, a_start_time, a_end_time);
}

bool CMCHistory::loadFromDatabase(Time start_time, Time end_time) {
  CMCCandlePeriod& candle_period = *m_candle_periods[5_min];

  if (!candle_period.empty()) clearData();

  candle_period.loadFromDatabase(m_db, start_time, end_time);

  for (auto& candle_period_itr : m_candle_periods) {
    if (candle_period_itr.first != 5_min)
      candle_period_itr.second->convertFrom(candle_period);  // get bigger candles from unit candles
  }

  return true;
}

int64_t CMCHistory::syncToDatabase() {
  return m_candle_periods[5_min]->storeToDatabase(m_db);
}

void CMCHistory::clearData() {
  for (auto& candle_period : m_candle_periods) {
    candle_period.second->clear();
  }

  for (auto indicator_iter : m_indicators) {
    DELETE(indicator_iter.second);
  }

  m_indicators.clear();
}

bool CMCHistory::appendCandles(CMCCandlePeriod& candle_period) {
  bool result;

  for (auto& c : candle_period) {
    COUT << "CMCHistory::appendCandles " << c << endl;

    result = this->appendCandle(c);

    if (!result)
      ;
    {
      CT_WARN << "Unable to add trades to TradeHistory\n";
      return false;
    }
  }

  return true;
}

void CMCHistory::addCandlePeriod(Duration interval) {
  if (interval.getDuration() % Duration(0, 0, 5, 0).getDuration() != 0) {
    CT_CRIT_WARN << "Invalid interval specified to add.\n";
    return;
  }

  if (m_candle_periods.find(interval) == m_candle_periods.end()) {
    CMCCandlePeriod* candle_period = new CMCCandlePeriod(interval);
    m_candle_periods.insert(make_pair(interval, candle_period));
    candle_period->convertFrom(*m_candle_periods[5_min]);
  }
}

bool CMCHistory::addIndicator(Duration interval, DiscreteIndicatorA<CMCandleStick>* indicator) {
  if (m_candle_periods.find(interval) == m_candle_periods.end()) {
    CT_CRIT_WARN << "CandlePeriod for this duration doesn't exist. Can't create indicator.\n";
    return false;
  }
  indicator->constructFrom(*m_candle_periods[interval]);
  m_indicators.insert(make_pair(interval, indicator));

  return true;
}

CMCandleStick CMCHistory::getLatestCandle(Duration interval) {
  return m_candle_periods[interval]->back();
}

CMCandleStick CMCHistory::getOldestCandle(Duration interval) {
  return m_candle_periods[interval]->front();
}

CMCHistory::~CMCHistory() {
  clearData();
  DELETE(m_db);
}
