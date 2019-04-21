//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 13/01/18.
//
// This consists simulation test code.

#include <catch2/catch.hpp>
#include <regex>

#include "CandlePeriod.h"
#include "CoinAPI.h"
#include "Controller.h"
#include "TraderBot.h"

using namespace std;

TEST_CASE("currency_pair", "[basic][precommit]") {
  COUT << CBLUE << "TEST: currency_pair [basic]\n";

  CurrencyPair currency_pair(currency_t::BTC, currency_t::USD);

  REQUIRE(currency_pair.toString() == "BTC-USD");
  REQUIRE(currency_pair.getBaseCurrency() == currency_t::BTC);
  REQUIRE(currency_pair.getQuoteCurrency() == currency_t::USD);
}

TEST_CASE("account", "[basic][precommit]") {
  COUT << CBLUE << "TEST: account [basic]\n";

  Account account("BTC", 1.2);
  stringstream ss;
  regex e("\\[ BTC : VIRTUAL-BTC-[0-9a-f\\- ]+\\] : balance: 1.2000, available: 1.2000, hold: 0.0000");

  unordered_map<currency_t, Account> accounts;

  ss << account;

  accounts.insert(make_pair(currency_t::BTC, account));

  COUT << account << endl;

  REQUIRE(regex_match(ss.str(), e));
}

TEST_CASE("market_order_sim", "[advanced][precommit]") {
  COUT << CBLUE << "TEST: market_order_sim [advanced]\n";

  TraderBot* trader_bot = TraderBot::getInstance();

  const string config_file = g_trader_home + "/tests/files/controller/market/market_algo_simulation.json";
  const char* args[] = {"exe", "-noRand", "-r", config_file.c_str()};

  REQUIRE(!trader_bot->traderMain(4, args));

  double portfolio_value = trader_bot->getController()->getPortfolioValue();
  int portfolio_value_int = (portfolio_value * 1000);
  REQUIRE(portfolio_value_int == 10049);

  TraderBot::deleteInstance();
}

TEST_CASE("limit_order_sim", "[advanced][precommit]") {
  COUT << CBLUE << "TEST: limit_order_sim [advanced]\n";

  TraderBot* trader_bot = TraderBot::getInstance();

  const string config_file = g_trader_home + "/tests/files/controller/limit/limit_algo_simulation.json";
  const char* args[] = {"exe", "-noRand", "-r", config_file.c_str()};

  REQUIRE(!trader_bot->traderMain(4, args));

  double portfolio_value = trader_bot->getController()->getPortfolioValue();
  int portfolio_value_int = (portfolio_value * 1000);
  REQUIRE(portfolio_value_int == 15528605);

  TraderBot::deleteInstance();
}

// ============================================================================================
// For following testcases to work, trades needs to be updated constantly by another program.
// ============================================================================================
//
TEST_CASE("market_order_real_sim", "[long][precommit]") {
  COUT << CBLUE << "TEST: market_order_real_sim [long]\n";

  TraderBot* trader_bot = TraderBot::getInstance();

  const string config_file = g_trader_home + "/tests/files/controller/market/market_algo_real_sim.json";
  const char* args[] = {"exe", "-r", config_file.c_str(), "-noRand", "-d", "t"};

  REQUIRE(!trader_bot->traderMain(6, args));

  double portfolio_value = trader_bot->getController()->getPortfolioValue();
  int portfolio_value_int = (portfolio_value * 1000);

  COUT << CBLUE << "First round simulation complete at " << Time::sNow() << endl;

  Controller* p_controller = trader_bot->getController();

  // create json for simulation
  const string new_config = "sim.json";

  string command = "rm -f " + new_config + "; ";
  command += "head -3 " + config_file + " > " + new_config + "; ";

  int exit_code = system(command.c_str());
  REQUIRE(exit_code == 0);

  FILE* p_json_file = fopen(new_config.c_str(), "a");

  char quote = '"';
  fprintf(p_json_file, "\t\t%cstart_time%c : %c%s%c,\n", quote, quote, quote,
          p_controller->getStartTime().toISOTimeString().c_str(), quote);
  fprintf(p_json_file, "\t\t%cend_time%c : %c%s%c\n", quote, quote, quote,
          p_controller->getEndTime().toISOTimeString().c_str(), quote);

  fclose(p_json_file);

  command = "tail -19 " + config_file + " >> " + new_config + "; ";
  exit_code = system(command.c_str());
  REQUIRE(exit_code == 0);

  TraderBot::deleteInstance();

  // run normal simulation

  // wait for 1 min
  this_thread::sleep_for(chrono::minutes(1));

  // reset static variables
  StringUtils::s_str_offset = 0;

  COUT << CBLUE << "Restarting simulation at " << Time::sNow() << endl;

  trader_bot = TraderBot::getInstance();

  const char* new_args[] = {"exe", "-r", new_config.c_str()};

  REQUIRE(!trader_bot->traderMain(3, new_args));

  double new_portfolio_value = trader_bot->getController()->getPortfolioValue();
  int new_portfolio_value_int = (new_portfolio_value * 1000);

  REQUIRE(portfolio_value_int == new_portfolio_value_int);

  TraderBot::deleteInstance();
}

TEST_CASE("market_order_real", "[advanced]") {
  COUT << CBLUE << "TEST: market_order_real [advanced]\n";

  TraderBot* trader_bot = TraderBot::getInstance();

  const string config_file = g_trader_home + "/tests/files/controller/market/market_algo_real.json";
  const char* args[] = {"exe", "-r", config_file.c_str(), "-d", "o"};

  REQUIRE(!trader_bot->traderMain(5, args));

  double portfolio_value = trader_bot->getController()->getPortfolioValue();
  COUT << CYELLOW << "\n portfolio value: " << portfolio_value << endl << endl;

  TraderBot::deleteInstance();
}

TEST_CASE("market_order_both", "[long]") {
  COUT << CBLUE << "TEST: market_order_both [long]\n";

  TraderBot* trader_bot = TraderBot::getInstance();

  const string config_file = g_trader_home + "/tests/files/controller/market/market_algo_both.json";
  const char* args[] = {"exe", "-r", config_file.c_str(), "-d", "o"};

  REQUIRE(!trader_bot->traderMain(5, args));

  double portfolio_value = trader_bot->getController()->getPortfolioValue();
  double virtual_value = trader_bot->getController()->getVirtualPortfolioValue();

  COUT << CYELLOW << "\nReal portfolio value: " << portfolio_value << ", virtual portfolio value: " << virtual_value
       << endl
       << endl;

  TraderBot::deleteInstance();
}

TEST_CASE("limit_order_real", "[advanced]") {
  COUT << CBLUE << "TEST: limit_order_real [advanced]\n";

  TraderBot* trader_bot = TraderBot::getInstance();

  const string config_file = g_trader_home + "/tests/files/controller/limit/limit_algo_real.json";
  const char* args[] = {"exe", "-r", config_file.c_str(), "-d", "o"};

  REQUIRE(!trader_bot->traderMain(5, args));

  double portfolio_value = trader_bot->getController()->getPortfolioValue();
  COUT << CYELLOW << "\n portfolio value: " << portfolio_value << endl << endl;

  TraderBot::deleteInstance();
}

TEST_CASE("limit_order_both", "[long]") {
  COUT << CBLUE << "TEST: limit_order_both [long]\n";

  TraderBot* trader_bot = TraderBot::getInstance();

  const string config_file = g_trader_home + "/tests/files/controller/limit/limit_algo_both.json";
  const char* args[] = {"exe", "-r", config_file.c_str(), "-d", "o"};

  REQUIRE(!trader_bot->traderMain(5, args));

  double portfolio_value = trader_bot->getController()->getPortfolioValue();
  double virtual_value = trader_bot->getController()->getVirtualPortfolioValue();

  COUT << CYELLOW << "\nReal portfolio value: " << portfolio_value << ", virtual portfolio value: " << virtual_value
       << endl
       << endl;

  TraderBot::deleteInstance();
}
