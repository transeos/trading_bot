//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 13/01/18.
// Modified by Subhagato Dutta on 13/01/18.
//
// Initialization for testing codes.

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include <thread>
#include <utils/Volume.h>

#include "TraderBot.h"
#include "exchanges/GDAX.h"

#define DEFINE_GLOBALS
#include "Globals.h"

#if ENABLE_LOGGING
bool Logger::s_suppress_warnings = false;
#endif

#undef DEFINE_GLOBALS

using namespace std;

TraderBot* TraderBot::mp_handler = nullptr;

TEST_CASE("init", "[basic][precommit]") {
  COUT << CBLUE << "TEST: init [basic]\n";
  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  REQUIRE(trader_bot->getGDAX() != nullptr);
  REQUIRE(trader_bot->getGDAX()->isAuthenticated() == true);

  TraderBot::deleteInstance();
}

TEST_CASE("destructor", "[basic][precommit]") {
  COUT << CBLUE << "TEST: destructor [basic]\n";
  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  TraderBot::deleteInstance();

  trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  TraderBot::deleteInstance();
}

TEST_CASE("logger", "[basic][precommit]") {
  COUT << CBLUE << "TEST: logger [basic]\n";

#if ENABLE_LOGGING
  Logger::s_suppress_warnings = true;
#endif

  // ERROR(2) << "Hello World error!"<<endl;

  CT_INFO << CBLUE << "Hello World in Blue!" << endl;
  COUT << CGREEN << "Hello World\n<next line> in Green!" << endl;
  COUT << CMAGENTA << "Hello World" << endl << "<next line> in Magenta!" << endl;
  COUT << "Hello World in no color!" << endl;
  COUT << endl;
  CT_CRIT_WARN << "Hello World warning!" << endl;
  CT_WARN << "Hello World warning suppressed!" << endl;

  COUT << CGREEN << SB_ << "Hello World" << _SB << " in Bold Green!" << endl;
  COUT << CBLUE << SI_ << "Hello World" << _SI << " in Italic Blue!" << endl;
  COUT << CYELLOW << SU_ << "Hello World" << _SU << " in Underlined Yellow!" << endl;

  COUT << CRED << Time::sNow() << endl;

  COUT << CWHITE << SB_ << SU_ << "Hello World" << _SU << _SB << " in Bold Underlined White!" << endl;
}

TEST_CASE("volume", "[basic][precommit]") {
  Volume v1 = 1.3;
  Volume v2 = -1.5;

  Volume v3;

  v3 = v1 + v2;

  REQUIRE(v1 == 1.3);
  REQUIRE(v2 == 1.5);  // when it has only negative volume, matches with negative volume
  REQUIRE(v3 == 2.8);

  REQUIRE(v3.getBuyVolume() == 1.3);
  REQUIRE(v3.getSellVolume() == 1.5);

  cout << "v1 = " << v1 << endl;
  cout << "v2 = " << v2 << endl;
  cout << "v3 = " << v3 << endl;
}

TEST_CASE("signal", "[basic][precommit]") {
  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  for (int k = 0; k < 10; k++) {
    g_critcal_task.lock();
    COUT << "Inside critical section in TEST_CASE\n";
    int64_t j = 0;
    for (int64_t i = 0; i < 1000000000; i++) {
      j += i;
    }
    COUT << j << endl;
    COUT << "Task " << k << " done." << endl;
    g_critcal_task.unlock();

    this_thread::sleep_for(chrono::seconds(1));
  }
}
