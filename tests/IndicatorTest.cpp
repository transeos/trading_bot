//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 13/01/18.
// Modified by Subhagato Dutta on 13/01/18.
//
// Template for testing codes.

#include <catch2/catch.hpp>

#include "Database.h"
#include "Tick.h"
#include "TraderBot.h"
#include "exchanges/GDAX.h"

#include "indicators/EMA.h"
#include "indicators/MA.h"
#include "indicators/MACD.h"
#include "indicators/RSI.h"
#include "indicators/SMA.h"
#include "indicators/TEMA.h"
#include <indicators/DiscreteIndicator.h>
#include <thread>

using namespace std;

TEST_CASE("indicator", "[basic][precommit]") {
  COUT << CBLUE << "TEST: indicator [basic]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  REQUIRE(trader_bot->getGDAX() != nullptr);

  TradeHistory* th = trader_bot->getGDAX()->getTradeHistory(CurrencyPair("BTC-USD"));
  DiscreteIndicator<MA, MA> indicators(th->getExchangeId(), th->getCurrencyPair(), 1_min, MA(5, false), MA(5, true));

  th->loadFromDatabase(Time(2017, 12, 1, 0, 0, 0), Time(2017, 12, 1, 0, 10, 0));
  th->addCandlePeriod(1_min);
  th->addIndicator(1_min, &indicators);

  Tick t;
  Database<Tick>& db = *th->getDb();

  db.setScope(Time(2017, 12, 1, 0, 15, 0), Time(2017, 12, 1, 0, 25, 0));  // add a gap of 5 minutes

  int64_t last_trade_id = th->getLatestTick().getUniqueID();

  for (auto tick : db) {
    tick.setUniqueId(++last_trade_id);  // hack to force the trades to be continuous
    th->appendTrade(tick);
  }
  // COUT<<tick_period<<endl;
  th->printCandlePeriod(1_min);

  COUT << "Price based :\n";
  COUT << indicators.getIndicator<0>() << endl;
  COUT << "Volume weighted :\n";
  COUT << indicators.getIndicator<1>() << endl;

  TraderBot::deleteInstance();
}

TEST_CASE("indicator_discrete", "[basic][precommit]") {
  COUT << CBLUE << "TEST: indicator_discrete [basic]\n";

  TraderBot* trader_bot = TraderBot::getInstance();

  REQUIRE(!trader_bot->traderMain());

  REQUIRE(trader_bot->getGDAX() != nullptr);

  TradeHistory* th = trader_bot->getGDAX()->getTradeHistory(CurrencyPair("BTC-USD"));

  th->loadFromDatabase(Time(2017, 12, 1, 0, 0, 0), Time(2017, 12, 1, 1, 0, 0));

  th->addCandlePeriod(1_min);

  // COUT<<tick_period<<endl;
  th->printCandlePeriod(1_min);

  DiscreteIndicator<SMA, SMA, TEMA, EMA, RSI, MACD> indicators(th->getExchangeId(), th->getCurrencyPair(), 1_min,
                                                               SMA(3), SMA(5), TEMA(3), EMA(3), RSI(5), MACD(5, 10, 3));

  // deque<sma_t>& sma3 = indicators.getIndicatorValues<0>();  // returns deque like CandlePeriod
  // deque<sma_t>& sma5 = indicators.getIndicatorValues<1>();
  deque<tema_t>& tema3 = indicators.getIndicatorValues<2>();
  // deque<ema_t>& ema3 = indicators.getIndicatorValues<3>();
  // deque<rsi_t>& rsi3 = indicators.getIndicatorValues<4>();
  // deque<macd_t>& macd5 = indicators.getIndicatorValues<5>();

  DiscreteIndicatorA<>* indicatorA = &indicators;
  th->addIndicator(1_min, indicatorA);

  sma_t val = indicators.getIndicatorAtIdx<0>(30);  // get indicator at 30th place

  REQUIRE(val.sma == 9989);

  std::cout << "TEMA3 ->\n";
  for (auto tema_val : tema3) {
    std::cout << tema_val.tema << std::endl;
  }

  REQUIRE(SMA::getFields()[0].first == "SMA-value");
  REQUIRE(SMA::getFields()[0].second == "double");

  TraderBot::deleteInstance();
}
