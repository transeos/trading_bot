// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 03/02/18.
//
// This consists coin api test code.

#include <catch2/catch.hpp>
#include <regex>

#include "CoinAPI.h"
#include "CoinAPITick.h"
#include "TraderBot.h"

using namespace std;

TEST_CASE("coin_api_load", "[advanced]") {
  COUT << CBLUE << "TEST: coin_api_load [advanced]\n";

  TraderBot* trader_bot = TraderBot::getInstance();

  REQUIRE(!trader_bot->traderMain());

  CoinAPI* p_coinAPI = trader_bot->getCoinAPI();
  CoinAPIHistory& history = p_coinAPI->getHistory();

  const Time end_time = Time::sNow();
  const Time start_time = end_time - 5_min;

  history.loadFromDatabase(start_time, end_time);

  CurrencyPair currency_pair1(currency_t::BTC, currency_t::USD);
  CurrencyPair currency_pair2(currency_t::ETH, currency_t::USD);

  const CapiTickPeriod* p_btc_tick_period = history.getTickPeriod(currency_pair1);
  const CapiTickPeriod* p_eth_tick_period = history.getTickPeriod(currency_pair2);
  REQUIRE(((p_btc_tick_period->size() > 0) && (p_eth_tick_period->size() > 0)));

  history.addCandlePeriod(1_min);

  REQUIRE(((history.getNumCandles(currency_pair1, 1_min) >= 5) && (history.getNumCandles(currency_pair2, 1_min) >= 5)));

  COUT << endl << CGREEN << currency_pair1 << endl;
  history.printCandlePeriod(currency_pair1, 1_min);

  COUT << endl << CGREEN << currency_pair2 << endl;
  history.printCandlePeriod(currency_pair2, 1_min);

  TraderBot::deleteInstance();
}
