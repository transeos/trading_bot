//
// Created by Subhagato Dutta on 9/10/17.
//

#include "CoinMarketCap.h"
#include "Database.h"
#include "utils/RestAPI2JSON.h"

#ifndef COIN_MARKET_CAP_DISABLED

using namespace std;
using namespace nlohmann;

const unordered_map<currency_t, string> CoinMarketCap::m_cmc_ids = {
    {currency_t::BTC, "bitcoin"},  {currency_t::ETH, "ethereum"},
    {currency_t::LTC, "litecoin"}, {currency_t::BCH, "bitcoin-cash"},
    {currency_t::XRP, "ripple"},   {currency_t::ADA, "cardano"},
    {currency_t::XMR, "monero"},   {currency_t::XLM, "stellar"},
    {currency_t::NEO, "neo"},      {currency_t::EOS, "eos"},
    {currency_t::DASH, "dash"},    {currency_t::IOTA, "iota"},
    {currency_t::XEM, "nem"},      {currency_t::ETC, "ethereum-classic"},
    {currency_t::TRX, "tron"},     {currency_t::ZEC, "zcash"},
    {currency_t::XVG, "verge"},    {currency_t::USDT, "tether"}};

CoinMarketCap::CoinMarketCap() {
  m_database_handle = new RestAPI2JSON("https://graphs2.coinmarketcap.com", 1);
  m_api_handle = new RestAPI2JSON("https://api.coinmarketcap.com/v1", 1);

  for (auto iter : m_cmc_ids) {
    const currency_t c = iter.first;
    m_cryptos.insert(make_pair(c, new CMCHistory(c)));
  }
}

CoinMarketCap::~CoinMarketCap() {
  DELETE(m_database_handle);
  DELETE(m_api_handle);
}

CMCandleStick CoinMarketCap::getLatestCandle(currency_t currency) {
  string prefix = m_cmc_ids.find(currency)->second;
  CMCandleStick candle;
  json ticker_info;

  CONTINUOUS_TRY(ticker_info = m_api_handle->getJSON_GET("/ticker/" + prefix + "/"), 100);

  int64_t timestamp = 1000 * getJsonValueT<string, int64_t>(ticker_info[0], "last_updated");
  double market_cap_usd = getJsonValueT<string, double>(ticker_info[0], "market_cap_usd");
  double price_btc = getJsonValueT<string, double>(ticker_info[0], "price_btc");
  double price_usd = getJsonValueT<string, double>(ticker_info[0], "price_usd");
  double volume_usd = getJsonValueT<string, double>(ticker_info[0], "24h_volume_usd");

  candle = CMCandleStick(timestamp, market_cap_usd, price_btc, price_usd, volume_usd);

  // COUT<<CBLUE<<Currency::sCurrencyToString(currency)<<" -> "<<candle<<endl;

  m_latest_Candles[currency] = candle;
  return candle;
}

CMCandleStick CoinMarketCap::getLatestCandleFromCache(const currency_t a_currency) {
  if (m_latest_Candles.find(a_currency) == m_latest_Candles.end()) {
    // check if the cached candle is too old

    const CMCandleStick& candle = m_latest_Candles[a_currency];

    if ((Time::sNow() - candle.getTimeStamp()) < 6_min) return candle;
  }

  // update latest candle
  return getLatestCandle(a_currency);
}

int64_t CoinMarketCap::fillCandles(CMCCandlePeriod& cp, currency_t currency, Time start_time,
                                   Time end_time)  // must be <= 24 hours
{
  json daily_data;
  string prefix = m_cmc_ids.find(currency)->second;

  int64_t last_timestamp = 0;
  int64_t count = 0;

  assert(start_time > Time(0));
  assert(end_time >= start_time);
  assert((end_time - start_time) <= Duration(1, 0, 0, 0));

  do {
    try {
      daily_data =
          m_database_handle->getJSON_GET("/currencies/" + prefix + "/" + to_string(start_time.millis_since_epoch()) +
                                         "/" + to_string(end_time.millis_since_epoch()) + "/");
    } catch (detail::type_error& err) {
      CT_WARN << "Exception: " << err.what() << endl << "\t\tinvalid json response from CoinMarketCap\n";
      this_thread::sleep_for(chrono::milliseconds(10));
      continue;
    } catch (exception& err) {
      CT_CRIT_WARN << "Exception: " << err.what() << endl << "\t\tunable to connect to CoinMarketCap.\n";
      COUT << "/" + prefix + "/" + to_string(start_time.millis_since_epoch()) + "/" +
                  to_string(end_time.millis_since_epoch()) + "/"
           << endl;
      this_thread::sleep_for(chrono::milliseconds(10));
      continue;
    }

    if (daily_data["market_cap_by_available_supply"].empty()) return 0;  // reached oldest trade

    auto market_cap_itr = daily_data["market_cap_by_available_supply"].rbegin();
    auto price_btc_itr = daily_data["price_btc"].rbegin();
    auto price_usd_itr = daily_data["price_usd"].rbegin();
    auto volume_usd_itr = daily_data["volume_usd"].rbegin();

    for (; market_cap_itr != daily_data["market_cap_by_available_supply"].rend();
         market_cap_itr++, price_btc_itr++, price_usd_itr++, volume_usd_itr++) {
      int64_t timestamp = (*market_cap_itr)[0].get<int64_t>();
      double market_cap_usd = (*market_cap_itr)[1].get<double>();
      double price_btc = (*price_btc_itr)[1].get<double>();
      double price_usd = (*price_usd_itr)[1].get<double>();
      double volume_usd = (*volume_usd_itr)[1].get<double>();

      CMCandleStick cmc_candle(timestamp, market_cap_usd, price_btc, price_usd, volume_usd);

      Time last_candle_timestamp = cp.front().getTimeStamp();
      Time current_candle_timestamp = cmc_candle.getTimeStamp();

      if (timestamp < last_timestamp) {
        if (last_candle_timestamp == current_candle_timestamp) {  // candle with same timestamp
          cmc_candle.setTimeStamp(last_candle_timestamp - 5_min);
          COUT << CMAGENTA << "Candle with same timestamp (" << prefix << ")\n";
        } else {
          while (current_candle_timestamp < (last_candle_timestamp - 5_min)) {  // one or more sample missing
            last_candle_timestamp -= 5_min;
            cmc_candle.setTimeStamp(last_candle_timestamp);
            cp.append(cmc_candle);  // repeat same candle
            count++;
            // COUT<<CYELLOW<<Currency::sCurrencyToString(currency)<<" -> "<<cmc_candle<<endl;
            COUT << CMAGENTA << "One candle missing (" << prefix << ")\n";
          }
          cmc_candle.setTimeStamp(current_candle_timestamp);
        }
      } else {
        if (last_timestamp != 0) {
          CT_WARN << "Inconsistent data (" << prefix << ")\n";
          continue;
        }
      }

      cp.append(cmc_candle);
      count++;
      // COUT<<CBLUE<<Currency::sCurrencyToString(currency)<<" -> "<<cmc_candle<<endl;

      last_timestamp = timestamp;
    }

    break;

  } while (true);

  return count;
}

bool CoinMarketCap::storeInitialCandles(currency_t a_currency) {
  CMCHistory* ch;
  string prefix;

  for (auto iter : m_cmc_ids) {
    const currency_t currency = iter.first;

    if (a_currency == currency || a_currency == currency_t::UNDEF) {
      CMCCandlePeriod candle_period(5_min);

      ch = getCMCHistory(currency);

      if (ch->getDb()->checkIfFilledFromBeginning()) continue;

      CMCandleStick min_cmc_candle;

      bool not_empty = ch->getDb()->getOldestRow(min_cmc_candle);

      Time start_time, end_time;

      if (not_empty)
        end_time = min_cmc_candle.getTimeStamp();
      else
        end_time = Time::sNow();

      int64_t candles_filled = 0;

      do {
        start_time = end_time - Duration(1, 0, 0, 0);

        candles_filled = fillCandles(candle_period, currency, start_time, end_time);

        end_time = start_time;

        start_time -= Duration(1, 0, 0, 0);

      } while (candles_filled > 0);

      COUT << CGREEN << "Finished loading data..(" << prefix << ")\n";

      int64_t num_entries_stored = candle_period.storeToDatabase(ch->getDb());

      ch->getDb()->updateOldestEntryMetadata();

      COUT << CGREEN << "Stored " << num_entries_stored << " to Database." << endl;
    }
  }

  return true;
}

bool CoinMarketCap::storeRecentCandles(currency_t a_currency) {
  CMCHistory* ch;

  static unordered_map<currency_t, CMCCandlePeriod> new_candles;

  for (auto iter : m_cmc_ids) {
    const currency_t currency = iter.first;

    if (a_currency == currency || a_currency == currency_t::UNDEF) {
      ch = getCMCHistory(currency);

      CMCandleStick newest_historic_candle;

      CMCCandlePeriod historic_candles(5_min);

      if (new_candles.find(currency) == new_candles.end())
        new_candles.insert(make_pair(currency, CMCCandlePeriod(5_min)));

      int64_t num_entries_stored = 0;

      bool empty = !ch->getDb()->getNewestRow(newest_historic_candle);

      if (empty) {
        CT_INFO << "Database for " << Currency::sCurrencyToString(currency) << " does not contain any data."
                                                                               "\n Filling from the beginning";
        return storeInitialCandles(currency);
      }

      CMCandleStick latest_candle = getLatestCandle(currency);

      if (newest_historic_candle.getTimeStamp() < latest_candle.getTimeStamp())
        new_candles[currency].append(latest_candle);
      else
        continue;

      if (newest_historic_candle.getTimeStamp() < (new_candles[currency].front().getTimeStamp() - 5_min)) {
        Time start_time, end_time;

        end_time = Time::sNow();

        int64_t candles_filled = 0;

        do {
          start_time = end_time - Duration(1, 0, 0, 0);
          if (start_time < newest_historic_candle.getTimeStamp())
            start_time = newest_historic_candle.getTimeStamp() - 1_min;

          candles_filled += fillCandles(historic_candles, currency, start_time, end_time);

          end_time = start_time;

          if (end_time < newest_historic_candle.getTimeStamp()) break;

        } while (true);

        num_entries_stored += historic_candles.storeToDatabase(ch->getDb());

        if (!historic_candles.empty()) {
          COUT << CRED << "COINMARKETCAP: " << CGREEN << " Saving historic " << CYELLOW
               << Currency::sCurrencyToString(currency) << CRESET << " (" << historic_candles.size() << ")" << CGREEN
               << " data from " << CRESET << historic_candles.front() << CGREEN << " to " << CRESET
               << historic_candles.back() << "." << endl;
          historic_candles.clear();
        }

      } else {
        if (!new_candles.empty()) {
          num_entries_stored += new_candles[currency].storeToDatabase(ch->getDb());
          COUT << CRED << "COINMARKETCAP: " << CGREEN << " Saving new " << CYELLOW
               << Currency::sCurrencyToString(currency) << CRESET << " (" << new_candles[currency].size() << ")"
               << CGREEN << " data from " << CRESET << new_candles[currency].front() << CGREEN << " to " << CRESET
               << new_candles[currency].back() << "." << endl;
          new_candles[currency].clear();
        }
      }

      //            COUT<<CGREEN<<"Stored "<<num_entries_stored<<" candles to Database."<<endl;
    }
  }
  return false;
}

CMCHistory* CoinMarketCap::getCMCHistory(currency_t currency) {
  return m_cryptos.at(currency);
}

void CoinMarketCap::saveToCSV(const string& a_exported_data_name, const Time a_start_time, const Time a_end_time,
                              const string& a_database_dir) {}

void CoinMarketCap::restoreFromCSV() {}

bool CoinMarketCap::repairDatabase(currency_t a_currency) {
  primary_key_t last_examined;
  vector<pair<primary_key_t, primary_key_t>> discontinuity_pairs;
  primary_key_t min_key;
  primary_key_t max_key;
  int64_t num_entries;
  bool repair_needed;

  for (auto iter : m_cmc_ids) {
    const currency_t currency = iter.first;

    if (a_currency == currency || a_currency == currency_t::UNDEF) {
      Database<CMCandleStick>* db = getCMCHistory(currency)->getDb();
      repair_needed = db->fixDatabase(true, true, min_key, max_key, last_examined, num_entries, discontinuity_pairs);

      if (repair_needed) {
        assert(discontinuity_pairs.empty());
      }

      db->getMetadata()->setConsistentEntryKey(last_examined);
    }
  }

  return false;
}

#endif
