//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 21/01/18.
//
// This consists test code for handling CSV.

#include "TraderBot.h"
#include <catch2/catch.hpp>

using namespace std;

TEST_CASE("csv_load", "[advanced]") {
  COUT << CBLUE << "TEST: csv_load [advanced]\n";

  TraderBot* trader_bot = TraderBot::getInstance();

  string database_dir = g_trader_home + "/tests/files/csvData";

  const char* args[] = {"exe", "-u", "-db", database_dir.c_str()};

  REQUIRE(!trader_bot->traderMain(4, args));

  TraderBot::deleteInstance();
}

TEST_CASE("gdax_csv_dump", "[advanced][precommit]") {
  COUT << CBLUE << "TEST: gdax_csv_dump [advanced]\n";

  // clean "data_out/COINBASE" directory
  system("rm -rf data_out/COINBASE");

  TraderBot* trader_bot = TraderBot::getInstance();

  string input_file = g_trader_home + "/tests/files/extractCSVData/getGDAXData.txt";

  const char* args[] = {"exe", "-g", input_file.c_str()};

  REQUIRE(!trader_bot->traderMain(3, args));

  // compare with the gold file
  const string command = "diff data_out/COINBASE " + g_trader_home + "/tests/files/csvData/COINBASE";
  const int exit_code = system(command.c_str());
  REQUIRE(exit_code == 0);

  TraderBot::deleteInstance();
}
