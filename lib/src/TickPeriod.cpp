//
// Created by subhagato on 11/18/17.
//

#include "TickPeriod.h"
#include "CMCandleStick.h"
#include "Candlestick.h"
#include "CoinAPITick.h"
#include "Database.h"
#include "Tick.h"
#include "TraderBot.h"
#include "utils/Logger.h"

using namespace std;

template <typename T>
TickPeriodT<T>::TickPeriodT(bool consecutive) : m_consecutive(consecutive) {
  reset();
}

template <typename T>
void TickPeriodT<T>::reset() {
  m_first_tid = 0;
  m_last_tid = 0;
  m_first_ts = 0;
  m_last_ts = 0;
  m_first_stored_itr = deque<T>::end();
  m_last_stored_itr = deque<T>::end();
}

template <typename T>
int TickPeriodT<T>::append(T& t, FILE* ap_csv_file) {
  int64_t unique_id = t.getUniqueID();

  if (this->size() == 0) {
    m_first_ts = m_last_ts = t.getTimeStamp();
    m_first_tid = m_last_tid = unique_id;
    this->push_back(t);

    if (ap_csv_file) DbUtils::writeToCSVLine<T>(ap_csv_file, t);

    return 1;  // considered as added in the end
  }

  if ((m_consecutive && getFirstUniqueId() == (unique_id + 1)) || (!m_consecutive && getFirstUniqueId() > unique_id)) {
    m_first_ts = t.getTimeStamp();
    m_first_tid = unique_id;
    this->push_front(t);

    if (ap_csv_file) DbUtils::writeToCSVLine<T>(ap_csv_file, t);

    return 2;  // added in the beginning
  } else if ((m_consecutive && getLastUniqueId() == (unique_id - 1)) ||
             (!m_consecutive && getLastUniqueId() < unique_id)) {
    m_last_ts = t.getTimeStamp();
    m_last_tid = unique_id;
    this->push_back(t);

    if (ap_csv_file) DbUtils::writeToCSVLine<T>(ap_csv_file, t);

    return 1;  // added in the end
  } else if (unique_id <= getLastUniqueId() && unique_id >= getFirstUniqueId()) {
    bool exists = m_consecutive;

    auto tick_itr = this->begin();

    if (!m_consecutive) {
      for (; tick_itr != this->end(); tick_itr++) {
        if (tick_itr->getUniqueID() == unique_id) {
          exists = true;
          break;
        } else if (tick_itr->getUniqueID() > unique_id)  // break when it just crosses the current id
        {
          break;
        }
      }
    }

    if (!exists) {
      if (ap_csv_file) DbUtils::writeToCSVLine<T>(ap_csv_file, t);

      this->insert(tick_itr, t);
      return 3;  // added in the middle
    } else {
      CT_WARN << t << " tick already exists\n";
      return 0;  // didn't add
    }

  } else {
    assert(m_consecutive);
    CT_WARN << "Can't add trade which is outside the range.\n";
    return -1;  // Error
  }
}

template <typename T>
void TickPeriodT<T>::sFixTimestamps(deque<T>& trades) {
  vector<T> ticks;

  const size_t num_ticks = trades.size();
  for (size_t idx = 0; idx < num_ticks; ++idx) ticks.push_back(trades.at(idx));

  sort(ticks.begin(), ticks.end(), less<T>());

  for (size_t idx = 0; idx < num_ticks; ++idx) {
    T& cur_tick = trades.at(idx);
    cur_tick.setTimeStamp(ticks[idx].getTimeStamp());
  }
}

template <typename T>
int TickPeriodT<T>::storeToDatabase(Database<T>* db) {
  size_t num_stored = 0;

  if (this->empty()) return 0;

  sFixTimestamps(*this);

  // sanity check
  if (m_consecutive) {
    for (size_t idx = (this->size() - 1); idx > 0; --idx) {
      auto& next_trade = this->at(idx);
      auto& cur_trade = this->at(idx - 1);

      if ((next_trade.getUniqueID() != (cur_trade.getUniqueID() + 1)) ||
          (next_trade.getTimeStamp() < cur_trade.getTimeStamp())) {
        CT_WARN << "Current trade : " << cur_trade << endl;
        CT_WARN << "Next trade : " << next_trade << endl;
        assert(0);
      }
    }
  }

  if (m_num_saved != 0) {
    if (getFirstUniqueId() == m_first_stored_itr->getUniqueID() &&
        getLastUniqueId() == m_last_stored_itr->getUniqueID()) {
      CT_WARN << "Data already stored in the database.\n";
      return 0;
    } else if (getFirstUniqueId() == m_first_stored_itr->getUniqueID()) {
      num_stored = db->storeData(m_last_stored_itr, this->end());
      if (num_stored > 0) m_last_stored_itr = this->end() - 1;  // iterator to the last element
    } else if (getLastUniqueId() == m_last_stored_itr->getUniqueID()) {
      num_stored = db->storeData(this->begin(), m_first_stored_itr);
      if (num_stored > 0) m_first_stored_itr = this->begin();
    }
  } else {
    num_stored = db->storeData(this->begin(), this->end());
    m_first_stored_itr = this->begin();
    m_last_stored_itr = this->end() - 1;

    // if(num_stored != this->size())
    //	num_stored = -1;
  }

  if (num_stored > 0) m_num_saved = num_stored;

  return num_stored;
}

template <typename T>
bool TickPeriodT<T>::loadFromDatabase(Database<T>* db, Time start_time, Time end_time) {
  deque<T>& data = *this;
  size_t num_stored;
  data.clear();
  num_stored = db->loadData(data, start_time, end_time);

  if (this->empty()) {
    CT_WARN << "No data in database\n";
    reset();
    return true;
  }

  m_first_ts = this->front().getTimeStamp();
  m_last_ts = this->back().getTimeStamp();
  m_first_tid = this->front().getUniqueID();
  m_last_tid = this->back().getUniqueID();

  m_first_stored_itr = this->begin();
  m_last_stored_itr = this->end();

  if (num_stored == this->size()) {
    m_num_saved = num_stored;
    return true;
  } else {
    return false;
  }
}

template <typename T>
void TickPeriodT<T>::saveToCSV(FILE* a_csv_file) {
  DbUtils::writeHeaderToCSVLine<T>(a_csv_file);

  for (auto& row : *this) {
    DbUtils::writeToCSVLine(a_csv_file, row);
    // fflush(p_file);	// optional
  }
}

template <typename T>
void TickPeriodT<T>::clear() {
  deque<T>::clear();

  m_first_tid = 0;
  m_last_tid = 0;
  m_first_ts = 0;
  m_last_ts = 0;
  m_first_stored_itr = deque<T>::end();
  m_last_stored_itr = deque<T>::end();
  m_num_saved = 0;
}

template class TickPeriodT<Tick>;
template class TickPeriodT<CMCandleStick>;
template class TickPeriodT<CoinAPITick>;
template class TickPeriodT<Candlestick>;
