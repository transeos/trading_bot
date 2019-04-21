//
// Created by subhagato on 11/18/17.
//

#include "CandlePeriod.h"
#include "CMCandleStick.h"
#include "Candlestick.h"
#include "CoinAPITick.h"
#include "Database.h"
#include "Tick.h"
#include "TickPeriod.h"
#include "utils/ErrorHandling.h"

using namespace std;

// only for adding to the end
template <typename C, typename T>
void addTickToExistingCandle(C& cs, const T& t) {
  double price = t.getPrice();
  double size = t.getSize();

  Volume volume = cs.getVolume();
  double mean = cs.getMean();
  double high = cs.getHigh();
  double low = cs.getLow();
  double stddev = cs.getStdDev();

  // printf("volume = %lf, mean = %lf, high = %lf, low = %lf, stddev = %lf\n", volume, mean, high, low, stddev);

  Volume volume_new;
  double mean_new;
  double stddev_new;

  // math taken from http://people.ds.cam.ac.uk/fanf2/hermes/doc/antiforgery/stats.pdf

  double Sn;
  double Sn_new;

  volume_new = volume + Volume(size);
  mean_new = (mean * static_cast<double>(volume) + price * abs(size)) / static_cast<double>(volume_new);

  Sn = static_cast<double>(volume) * pow(stddev, 2);
  Sn_new = Sn + abs(size) * (price - mean) * (price - mean_new);
  stddev_new = sqrt(std::abs(Sn_new) / static_cast<double>(volume_new));

  // printf("volumeNew = %lf, meanNew = %lf, highNew = %lf, lowNew = %lf, stddevNew = %lf\n", volume_new, mean_new,
  // max(high, price), min(low, price), stddev_new);

  cs.setMean(mean_new);
  cs.setStdDev(stddev_new);
  cs.setClose(price);
  cs.setHigh(max(high, price));
  cs.setLow(min(low, price));
  cs.setVolume(volume_new);
}

template <typename C>
void addCandleToExistingCandle(C& existing_cs, Duration existing_interval, C new_cs, Duration new_interval) {
  Volume volume = existing_cs.getVolume();
  double mean = existing_cs.getMean();
  double high = existing_cs.getHigh();
  double low = existing_cs.getLow();
  double stddev = existing_cs.getStdDev();

  // PRINTF("volume = %lf, mean = %lf, high = %lf, low = %lf, stddev = %lf\n", volume, mean, high, low, stddev);

  Volume volume_new;
  double mean_new;
  double stddev_new;

  volume_new = volume + new_cs.getVolume();
  mean_new = (mean * static_cast<double>(volume) + new_cs.getMean() * static_cast<double>(new_cs.getVolume())) /
             static_cast<double>(volume_new);

  // not accurate, just a placeholder
  stddev_new = (stddev * static_cast<double>(volume) + new_cs.getStdDev() * static_cast<double>(new_cs.getVolume())) /
               static_cast<double>(volume_new);

  // PRINTF("volumeNew = %lf, meanNew = %lf, highNew = %lf, lowNew = %lf, stddevNew = %lf\n", volume_new, mean_new,
  // max(high, price), min(low, price), stddev_new);

  existing_cs.setMean(mean_new);
  existing_cs.setStdDev(stddev_new);

  if (existing_cs.getTimeStamp() == new_cs.getTimeStamp())
    existing_cs.setOpen(new_cs.getMean());
  else if ((existing_cs.getTimeStamp() + existing_interval - new_interval) == new_cs.getTimeStamp())
    existing_cs.setClose(new_cs.getMean());

  existing_cs.setHigh(max(high, new_cs.getHigh()));
  existing_cs.setLow(min(low, new_cs.getLow()));
  existing_cs.setVolume(volume_new);
}

template <typename T>
bool convertFromToCandlePeriodT(TickPeriodT<T>& trade_period, CandlePeriodT<Candlestick, T>* ap_candle_period) {
  if (trade_period.size() == 0) return false;

  Time cs_start = trade_period.getFirstTimestamp().quantize(ap_candle_period->getInterval());

  vector<double> price_arr;
  vector<Volume> volume_arr;

  ap_candle_period->clear();  // clear all existing candles

  bool beginning = true;

  for (auto it = trade_period.begin();;) {
    if (beginning) {
      price_arr.clear();
      volume_arr.clear();
      beginning = false;
    }
    Duration current_interval = (it->getTimeStamp() - cs_start);

    if (current_interval >= ap_candle_period->getInterval()) {
      beginning = true;
      Candlestick new_cs = Candlestick(cs_start, price_arr, volume_arr);
      if (!ap_candle_period->empty() && new_cs.getTotalVolume() == 0) {
        new_cs.setPrice(ap_candle_period->getLastPrice());
      }
      ap_candle_period->append(new_cs);
      cs_start += ap_candle_period->getInterval();
      continue;
    }

    price_arr.push_back(abs(it->getPrice()));
    volume_arr.push_back(Volume(it->getSize()));
    ap_candle_period->setLastPrice(it->getPrice());

    if ((++it) == trade_period.end()) {
      ap_candle_period->append(Candlestick(cs_start, price_arr, volume_arr));
      break;
    }
  }

  return true;
}

template <typename T>
int appendTickInCandlePeriodT(const T& t, CandlePeriodT<Candlestick, T>* ap_candle_period) {
  Time last_timestamp;

  double price = t.getPrice();
  double size = t.getSize();

  if (ap_candle_period->getLastPrice() == 0) ap_candle_period->setLastPrice(t.getPrice());

  Duration candle_interval = ap_candle_period->getInterval();

  bool new_cs_created = false;

  if (ap_candle_period->empty())  // empty period
  {
    Candlestick cs(t.getTimeStamp().quantize(candle_interval), price, price, price, price, price, 0, size);

    ap_candle_period->append(cs);
    ap_candle_period->setNonZeroSampleCount(1);
    return 2;
  }

  while ((last_timestamp = ap_candle_period->getLastTimestamp()) <= (t.getTimeStamp() - candle_interval)) {
    Candlestick cs;
    Duration time_difference = (t.getTimeStamp() - last_timestamp);

    cs.setTimeStamp(last_timestamp + candle_interval);

    if (time_difference >= candle_interval * 2)
      cs.setPrice(ap_candle_period->getLastPrice());
    else {
      cs.setPrice(price);
      cs.setVolume(size);
    }

    ap_candle_period->append(cs);  // keep on filling candlesticks till the point
    new_cs_created = true;
  }

  if (!new_cs_created)  // add to existing candlestick
  {
    Candlestick& cs = ap_candle_period->back();  // get the latest candlestick
    addTickToExistingCandle(cs, t);
  } else {
    // COUT << CRED << "Candlestick added for Tick-" << t.getUniqueID() << endl;
  }

  ap_candle_period->setLastPrice(price);

  ap_candle_period->setNonZeroSampleCount(ap_candle_period->getNonZeroSampleCount() + 1);

  if (new_cs_created)
    return 2;  // new candlestick created
  else
    return 1;
}

template <>
bool CandlePeriod::convertFrom(TickPeriod& trade_period) {
  return convertFromToCandlePeriodT(trade_period, this);
}

template <>
bool CapiCandlePeriod::convertFrom(CapiTickPeriod& trade_period) {
  return convertFromToCandlePeriodT(trade_period, this);
}

template <typename C, typename T>
bool CandlePeriodT<C, T>::convertFrom(CandlePeriodT<C, T>& candle_period) {
  if (candle_period.size() == 0) {
    CT_CRIT_WARN << "CandlePeriodT to convert from is empty.\n";
    return false;
  }

  if (candle_period.getInterval() != 5_min) {
    CT_CRIT_WARN << "CandlePeriodT to convert from has a different duration than 5 minutes.\n";
    return false;
  }

  if (candle_period.getInterval() == m_interval) {
    CT_CRIT_WARN << "CandlePeriodT to convert from has same interval as the current one.\n";
    *this = candle_period;
    return true;
  }

  this->clear();

  for (auto it = candle_period.begin(); it != candle_period.end(); it++) {
    this->appendSmallerCandle(*it, candle_period.getInterval());
  }

  return true;
}

template <>
CandlePeriodT<Candlestick, Tick>::CandlePeriodT(TickPeriod& trade_period, Duration interval) {
  m_interval = interval;
  m_last_price = 0;
  convertFrom(trade_period);
}

template <>
CandlePeriodT<Candlestick, CoinAPITick>::CandlePeriodT(CapiTickPeriod& trade_period, Duration interval) {
  m_interval = interval;
  m_last_price = 0;
  convertFrom(trade_period);
}

template <>
CandlePeriod& CandlePeriod::operator=(TickPeriod& trade_period) {
  convertFrom(trade_period);
  return *this;
}

template <>
CapiCandlePeriod& CapiCandlePeriod::operator=(CapiTickPeriod& trade_period) {
  convertFrom(trade_period);
  return *this;
}

template <typename C, typename T>
CandlePeriodT<C, T>::CandlePeriodT(deque<C>& candlesticks, Duration interval) {
  m_last_price = 0;

  Time last_timestamp, current_timestamp;
  auto candlestick_iter = candlesticks.begin();

  last_timestamp = candlestick_iter->getTimeStamp();

  for (candlestick_iter += 1; candlestick_iter != candlesticks.end(); candlestick_iter++) {
    current_timestamp = candlestick_iter->getTimeStamp();

    if (current_timestamp != (last_timestamp + interval)) {
      CANDLESTICK_DEQUE_ERROR(current_timestamp, last_timestamp, interval);
    }
    last_timestamp = current_timestamp;
  }

  static_cast<deque<C>>(*this) = candlesticks;

  m_first_ts = candlesticks.front().getTimeStamp();
  m_last_ts = candlesticks.back().getTimeStamp();
  m_interval = interval;
  m_stored = false;
}

template <typename C, typename T>
int CandlePeriodT<C, T>::append(const C& cs) {
  Time timestamp = cs.getTimeStamp();

  if (this->size() == 0) {
    m_first_ts = m_last_ts = timestamp;
    this->push_back(cs);
    return 1;  // added in the beginning
  }

  if (getFirstTimestamp() == (timestamp + m_interval)) {
    m_first_ts = cs.getTimeStamp();
    this->push_front(cs);
    return 2;  // added in the beginning
  } else if (getLastTimestamp() == (timestamp - m_interval)) {
    m_last_ts = cs.getTimeStamp();
    this->push_back(cs);
    return 1;  // added in the end
  } else if (timestamp >= getFirstTimestamp() && timestamp <= getLastTimestamp()) {
    return 0;  // already exist
  } else {
    CT_WARN << "Can't add candlestick which is outside the range.\n";
    return -1;  // add failed
  }
}

template <typename C, typename T>
int CandlePeriodT<C, T>::appendSmallerCandle(const C& cs, Duration interval) {
  if (m_interval.getDuration() % interval.getDuration() != 0) {
    CT_WARN << "Can't add incompatible candlesticks\n";
    return 0;
  }

  Time timestamp = cs.getTimeStamp();

  if (this->size() == 0) {
    m_first_ts = m_last_ts = timestamp;
    this->push_back(cs);
    return 1;
  }

  if (getFirstTimestamp() <= timestamp &&
      (getLastTimestamp() + m_interval) > timestamp)  // insert within existing candlestick
  {
    C current_cs;
    for (size_t i = 0; i < this->size(); i++) {
      if (timestamp.quantize(m_interval) == this->at(i).getTimeStamp()) {
        C& current_cs = this->at(i);
        addCandleToExistingCandle(current_cs, m_interval, cs, interval);
        break;
      }
    }

    return 1;  // added to existing candlestick
  }

  if (timestamp >= (getLastTimestamp() + m_interval) && timestamp < getLastTimestamp() + m_interval * 2) {
    C new_cs = cs;
    new_cs.setTimeStamp(cs.getTimeStamp().quantize(m_interval));  // quantize timestamp to the bigger candlestick period
    m_last_ts = new_cs.getTimeStamp();
    this->push_back(new_cs);

    return 2;  // created candlestick at the end
  }

  if (timestamp < getFirstTimestamp() && (timestamp >= getFirstTimestamp() - m_interval)) {
    C new_cs = cs;
    new_cs.setTimeStamp(cs.getTimeStamp().quantize(m_interval));
    m_first_ts = new_cs.getTimeStamp();
    this->push_front(new_cs);

    return 3;  // created candlestick at the beginning
  }

  return 0;
}

template <typename C, typename T>
int CandlePeriodT<C, T>::storeToDatabase(Database<C>* db) {
  deque<C>& data = *this;
  int num_stored;
  num_stored = db->storeData(data, getFirstTimestamp(), getLastTimestamp() + Duration(0, 0, 0, 0, 0, 1));
  m_stored = true;

  return num_stored;
}

template <typename C, typename T>
bool CandlePeriodT<C, T>::loadFromDatabase(Database<C>* db, Time start_time, Time end_time) {
  this->clear();
  deque<C>& data = *this;
  size_t num_stored;
  num_stored = db->loadData(data, start_time, end_time);
  m_first_ts = this->front().getTimeStamp();
  m_last_ts = this->back().getTimeStamp();
  m_stored = true;

  return (num_stored == this->size());
}

template <typename C, typename T>
void CandlePeriodT<C, T>::saveToCSV(FILE* a_csv_file, const bool complete_line) {
  saveHeaderToCSV(a_csv_file, complete_line);

  for (auto& row : *this) {
    DbUtils::writeToCSVLine(a_csv_file, row, complete_line);
    // fflush(p_file);	// optional
  }
}

template <typename C, typename T>
void CandlePeriodT<C, T>::saveHeaderToCSV(FILE* a_csv_file, const bool complete_line) {
  DbUtils::writeHeaderToCSVLine<C>(a_csv_file, complete_line);
}

template <>
int CandlePeriod::appendTick(const Tick& t) {
  return appendTickInCandlePeriodT(t, this);
}

template <>
int CapiCandlePeriod::appendTick(const CoinAPITick& t) {
  return appendTickInCandlePeriodT(t, this);
}

template <typename C, typename T>
void CandlePeriodT<C, T>::clear() {
  deque<C>::clear();

  m_first_ts = 0;
  m_last_ts = 0;
  m_non_zero_sample_count = 0;
  m_stored = false;
}

template class CandlePeriodT<Candlestick, Tick>;
template class CandlePeriodT<CMCandleStick, Tick>;
template class CandlePeriodT<Candlestick, CoinAPITick>;
