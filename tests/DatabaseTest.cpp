//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 13/01/18.
//
// This consists database test code.

#include <catch2/catch.hpp>
#include <regex>

#include "CoinAPI.h"
#include "CoinAPITick.h"
#include "CoinMarketCap.h"
#include "Database.h"
#include "Tick.h"
#include "TraderBot.h"
#include "exchanges/GDAX.h"

using namespace std;

TEST_CASE("database_io", "[basic][precommit]") {
  COUT << CBLUE << "TEST: database_io [basic]\n";

  size_t num_inserted = 0;

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  Database<Tick>* db = trader_bot->getGDAX()->getTradeHistory(CurrencyPair("BTC-USD"))->getDb();

  dbMetadata* db_metadata = db->getMetadata();

  primary_key_t max_entry = db_metadata->getMaxEntryKey();
  size_t num_entries = db_metadata->getNumEntries();

  g_update_cass = true;

  deque<Tick> ticks = {
      Tick(Time(2019, 12, 16, 10, 10, 0), 1000000000, 1, 1), Tick(Time(2019, 12, 16, 10, 11, 0), 1000000001, 20, -1),
      Tick(Time(2019, 12, 16, 10, 12, 0), 1000000002, 3, 10), Tick(Time(2019, 12, 16, 10, 13, 0), 1000000003, 41, -5),
      Tick(Time(2019, 12, 16, 10, 11, 0), 1000000001, 85, -10)};

  num_inserted = db->storeData(ticks);

  COUT << "num_inserted = " << num_inserted << endl;

  CHECK(num_inserted == 4);  // not 5
  CHECK(db_metadata->getMaxEntryKey() == primary_key_t{18246, 36780000000, 1000000003});
  CHECK(db_metadata->getNumEntries() == (num_entries + 4));

  string delete_inserted_data_cql = "DELETE FROM crypto.btc_usd_coinbase WHERE date = 18246;";

  CassStatement* cass_statement = cass_statement_new(delete_inserted_data_cql.c_str(), 0);

  CassFuture* cass_future = cass_session_execute(g_cass_session, cass_statement);
  cass_statement_free(cass_statement);

  CHECK(cass_future_error_code(cass_future) == CASS_OK);

  cass_future_free(cass_future);

  db_metadata->setMaxEntryKey(max_entry);
  db_metadata->setNumEntries(num_entries);

  TraderBot::deleteInstance();
}

TEST_CASE("database_iterator", "[basic][precommit]") {
  COUT << CBLUE << "TEST: database_iterator [basic]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  Database<Tick>& db = *trader_bot->getGDAX()->getTradeHistory(CurrencyPair("BTC-USD"))->getDb();

  dbMetadata& db_metadata = *db.getMetadata();

  //  primary_key_t max_entry = db_metadata.getMaxEntryKey();
  primary_key_t min_entry = db_metadata.getMinEntryKey();

  size_t num_entries = db_metadata.getNumEntries();

  COUT << "num_entries = " << num_entries << endl;

  db.setScope(min_entry.getTimeStamp() + Duration(500, 2, 30, 0), min_entry.getTimeStamp() + Duration(500, 18, 30, 0));

  COUT << "Some old tick data...\n";

  for (auto it = db.begin(); it != db.end(); it++) {
    COUT << *it << endl;
  }

  db.setScope(Time::sNow() - Duration(10, 0, 0, 0), Time::sNow() - Duration(9, 23, 0, 0));

  COUT << "Relatively recent tick data...\n";

  for (auto it : db) {
    COUT << it << endl;
  }

  TraderBot::deleteInstance();
}

TEST_CASE("database_iterator_cmc", "[basic]") {
  COUT << CBLUE << "TEST: database_iterator_cmc [basic]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  Database<CMCandleStick>& db = *trader_bot->getCoinMarketCap()->getCMCHistory(currency_t::BTC)->getDb();

  // dbMetadata &db_metadata = *db.getMetadata();

  db.setScope(Time::sNow() - Duration(10, 0, 0, 0), Time::sNow() - Duration(9, 23, 0, 0));

  COUT << "Some Candlestick data...\n";

  for (auto it = db.begin(); it != db.end(); it++) {
    COUT << *it << endl;
  }

  db.setScope(Time::sNow() - Duration(10, 0, 0, 0), Time::sNow() - Duration(9, 23, 0, 0));

  COUT << "Relatively recent tick data...\n";

  for (auto it : db) {
    COUT << it << endl;
  }

  TraderBot::deleteInstance();
}

TEST_CASE("database_metadata", "[basic][precommit]") {
  COUT << CBLUE << "TEST: database_metadata [basic]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  GDAX* p_gdax = trader_bot->getGDAX();

  for (auto iter : p_gdax->getMarkets()) {
    TradeHistory* tradeHistory = iter.second->getTradeHistory();
    if (tradeHistory) COUT << *tradeHistory->getDb()->getMetadata() << endl;
  }

  TraderBot::deleteInstance();
}

TEST_CASE("database_tickperiod", "[basic][precommit]") {
  COUT << CBLUE << "TEST: database_tickperiod [basic]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  g_update_cass = true;

  Database<Tick>* db = trader_bot->getGDAX()->getTradeHistory(CurrencyPair("BTC-USD"))->getDb();

  TickPeriod tick_period;

  trader_bot->getGDAX()->fillTrades(tick_period, CurrencyPair("BTC-USD"), 34119371, 34119359);

  int num_trades_saved = tick_period.storeToDatabase(db);
  COUT << "Stored " << num_trades_saved << " trades into database " << endl;

  tick_period.clear();

  TraderBot::deleteInstance();
}

TEST_CASE("database_repair", "[long]") {
  COUT << CBLUE << "TEST: database_repair [long]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  g_update_cass = true;

  // trader_bot->getCoinMarketCap()->repairDatabase(currency_t::BTC);

  // trader_bot->getGDAX()->repairDatabase(CurrencyPair("BTC-USD"));
  // trader_bot->getGDAX()->repairDatabase(CurrencyPair("LTC-USD"));
  trader_bot->getGDAX()->repairDatabase(CurrencyPair("ETH-USD"));

  TraderBot::deleteInstance();
}

TEST_CASE("database_restore", "[long]") {
  COUT << CBLUE << "TEST: database_restore [long]\n";

  const char* args[] = {"exe", "-u"};

  TraderBot* trader_bot = TraderBot::getInstance();

  REQUIRE(!trader_bot->traderMain(2, args));

  Database<Tick>& db1 = *trader_bot->getGDAX()->getTradeHistory(CurrencyPair("BTC-USD"))->getDb();
  Database<Tick>& db2 = *trader_bot->getGDAX()->getTradeHistory(CurrencyPair("ETH-USD"))->getDb();
  Database<Tick>& db3 = *trader_bot->getGDAX()->getTradeHistory(CurrencyPair("LTC-USD"))->getDb();

  const string exchange_name = Exchange::sExchangeToString(trader_bot->getGDAX()->getExchangeID());

  db1.restoreDataFromCSVFile(exchange_name + "/BTC-USD.csv", {4, 0, 1, 3, 2});
  db2.restoreDataFromCSVFile(exchange_name + "/ETH-USD.csv", {4, 0, 1, 3, 2});
  db3.restoreDataFromCSVFile(exchange_name + "/LTC-USD.csv", {4, 0, 1, 3, 2});

  TraderBot::deleteInstance();
}

TEST_CASE("coinmarketcap", "[long]") {
  COUT << CBLUE << "TEST: coinmarketcap [long]\n";

  const char* args[] = {"exe", "-u"};

  TraderBot* trader_bot = TraderBot::getInstance();

  REQUIRE(!trader_bot->traderMain(2, args));

  trader_bot->getCoinMarketCap()->repairDatabase();

  trader_bot->getCoinMarketCap()->storeInitialCandles();

  trader_bot->getCoinMarketCap()->storeRecentCandles();

  TraderBot::deleteInstance();
}
