//
// Created by subhagato on 1/13/18.
//

//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 13/01/18.
// Modified by Subhagato Dutta on 13/01/18.
//
// GDAX testing codes.

#include "Tick.h"
#include "TraderBot.h"
#include "exchanges/GDAX.h"
#include "exchanges/Gemini.h"
#include <catch2/catch.hpp>
#include <cmath>
#include <thread>

using namespace std;

TEST_CASE("candlestick", "[advanced][precommit]") {
  COUT << CBLUE << "TEST: candlestick [advanced]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  TickPeriod trades;
  CandlePeriod candles_static(Duration(0, 0, 1, 0, 0, 0));
  CandlePeriod candles_incremental(Duration(0, 0, 1, 0, 0, 0));

  Tick new_tick0(Time(2017, 12, 1, 6, 0, 1), 1, 100, 2);
  Tick new_tick1(Time(2017, 12, 1, 6, 0, 10), 2, 200, -1);
  Tick new_tick2(Time(2017, 12, 1, 6, 0, 12), 3, 10, -1);
  Tick new_tick3(Time(2017, 12, 1, 6, 0, 14), 4, 2000, 1);
  Tick new_tick4(Time(2017, 12, 1, 6, 0, 23), 5, 300, -5);
  Tick new_tick5(Time(2017, 12, 1, 6, 0, 48), 6, 100, 1);
  Tick new_tick6(Time(2017, 12, 1, 6, 0, 55), 7, 1000, -0.5);

  trades.append(new_tick0);
  trades.append(new_tick1);
  trades.append(new_tick2);
  trades.append(new_tick3);
  trades.append(new_tick4);
  trades.append(new_tick5);
  trades.append(new_tick6);

  Tick new_tick7(Time(2017, 12, 1, 6, 1, 3), 8, 20, -2);
  Tick new_tick8(Time(2017, 12, 1, 6, 1, 18), 9, 400, 1);
  Tick new_tick9(Time(2017, 12, 1, 6, 1, 19), 10, 80, -3);
  Tick new_tick10(Time(2017, 12, 1, 6, 1, 19), 11, 50, 1);
  Tick new_tick11(Time(2017, 12, 1, 6, 1, 20), 12, 1000, -0.1);
  Tick new_tick12(Time(2017, 12, 1, 6, 1, 21), 13, 30, 2);
  Tick new_tick13(Time(2017, 12, 1, 6, 1, 34), 14, 10, -10);
  Tick new_tick14(Time(2017, 12, 1, 6, 1, 48), 15, 50, 1);
  Tick new_tick15(Time(2017, 12, 1, 6, 1, 51), 16, 100, -1);
  Tick new_tick16(Time(2017, 12, 1, 6, 1, 59), 17, 200, 2);

  trades.append(new_tick7);
  trades.append(new_tick8);
  trades.append(new_tick9);
  trades.append(new_tick10);
  trades.append(new_tick11);
  trades.append(new_tick12);
  trades.append(new_tick13);
  trades.append(new_tick14);
  trades.append(new_tick15);
  trades.append(new_tick16);

  Tick new_tick17(Time(2017, 12, 1, 6, 4, 02), 18, 150, 1);
  Tick new_tick18(Time(2017, 12, 1, 6, 4, 18), 19, 80, -1);
  Tick new_tick19(Time(2017, 12, 1, 6, 4, 50), 20, 121, 3);

  trades.append(new_tick17);
  trades.append(new_tick18);
  trades.append(new_tick19);

  candles_static = trades;  // converts trades to candles

  for (auto& trade : trades) {
    COUT << trade << endl;
    candles_incremental.appendTick(trade);
  }

  COUT << "Static candles:" << endl;

  for (auto& candle : candles_static) COUT << candle << endl;

  COUT << "Incremental candles:" << endl;

  for (auto& candle : candles_incremental) COUT << candle << endl;

  CHECK(candles_static.size() == candles_incremental.size());

  for (size_t i = 0; i < candles_static.size(); i++) {
    CHECK(candles_static[i].getTimeStamp() == candles_incremental[i].getTimeStamp());
    CHECK(candles_static[i].getLow() == candles_incremental[i].getLow());
    CHECK(candles_static[i].getHigh() == candles_incremental[i].getHigh());
    CHECK(candles_static[i].getMean() == candles_incremental[i].getMean());
    CHECK(candles_static[i].getVolume() == candles_incremental[i].getVolume());
    CHECK(abs(candles_static[i].getStdDev() - candles_incremental[i].getStdDev()) < 1e-9);
    CHECK(candles_static[i].getOpen() == candles_incremental[i].getOpen());
    CHECK(candles_static[i].getClose() == candles_incremental[i].getClose());
  }

  trades.clear();

  TradeHistory* p_trade_history =
      trader_bot->getGDAX()->getTradeHistory(CurrencyPair(currency_t::BTC, currency_t::USD));
  trades.loadFromDatabase(p_trade_history->getDb(), Time(2017, 11, 1, 1, 19, 0), Time(2017, 11, 1, 1, 20, 0));

  candles_static = trades;

  candles_incremental.clear();

  for (auto& trade : trades) {
    COUT << trade << endl;
    candles_incremental.appendTick(trade);
  }

  COUT << "Static candles" << endl;

  for (auto& candlestick : candles_static) {
    COUT << candlestick << endl;
  }

  COUT << "Incremental candles" << endl;

  for (auto& candlestick : candles_incremental) {
    COUT << candlestick << endl;
  }

  for (size_t i = 0; i < candles_static.size(); i++) {
    CHECK(candles_static[i].getTimeStamp() == candles_incremental[i].getTimeStamp());
    CHECK_WITH_PRECISION(candles_static[i].getLow(), candles_incremental[i].getLow(), 1e-9);
    CHECK_WITH_PRECISION(candles_static[i].getHigh(), candles_incremental[i].getHigh(), 1e-9);
    CHECK_WITH_PRECISION(candles_static[i].getMean(), candles_incremental[i].getMean(), 1e-9);
    //    COUT<<"BuyVolume = s"<<candles_static[i].getBuyVolume()<<" i"<<candles_incremental[i].getBuyVolume()<<endl;
    CHECK_WITH_PRECISION(candles_static[i].getBuyVolume(), candles_incremental[i].getBuyVolume(), 1e-9);
    //    COUT<<"SellVolume = s"<<candles_static[i].getSellVolume()<<" i"<<candles_incremental[i].getSellVolume()<<endl;
    CHECK_WITH_PRECISION(candles_static[i].getSellVolume(), candles_incremental[i].getSellVolume(), 1e-9);
    CHECK_WITH_PRECISION(candles_static[i].getStdDev(), candles_incremental[i].getStdDev(), 1e-9);
    CHECK_WITH_PRECISION(candles_static[i].getOpen(), candles_incremental[i].getOpen(), 1e-9);
    CHECK_WITH_PRECISION(candles_static[i].getClose(), candles_incremental[i].getClose(), 1e-9);
  }

  TraderBot::deleteInstance();
}

TEST_CASE("gdax_quote", "[basic][precommit]") {
  COUT << CBLUE << "TEST: gdax_quote [basic]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  GDAX& gdax = *trader_bot->getGDAX();
  gdax.setMode({CurrencyPair("BTC-USD")});

  if (gdax.connectWebsocket()) {
    gdax.subscribeToTopic();
    for (int i = 0; i < 10; i++) {
      this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << gdax.getQuote(CurrencyPair("BTC-USD")) << std::endl;
    }
  }
  gdax.disconnectWebsocket();
  TraderBot::deleteInstance();

#if 0
  vector<CurrencyPair> supported_currency_pairs;
    vector<Currency> supported_currencies;

    vector<candlestick_t> candlesticks;

    traderBot->getGDAX()->getQuote({currency_t::BTC, currency_t::USD});

    COUT<<"\nSupported currencies in GDAX are \n";

    traderBot->getGDAX()->getCurrencies(supported_currencies);

    for (auto currency : supported_currencies)
        COUT<<currency<<"\n";

    COUT<<"\nAccount details \n";
    unordered_map<currency_t, Account> accounts;

    traderBot->getGDAX()->getAccounts(accounts);

    for (auto account : accounts)
        COUT<<account.second<<"\n";

    COUT<<"\nSupported currency pairs in GDAX are \n";


    traderBot->getGDAX()->getCurrencyPairs(supported_currency_pairs);

    for (auto currency_pair : supported_currency_pairs) {
        COUT<<currency_pair << "\n";
        COUT<<"Ticker Price: " << traderBot->getGDAX()->getTicker(currency_pair.getCurrencyPair()) << "\n";
        COUT<<"Quote: \n" << traderBot->getGDAX()->getQuote(currency_pair.getCurrencyPair()) << "\n";
    }

    const OrderBook btc_usd_ob = traderBot->getGDAX()->getOrderBook({currency_t::BTC, currency_t::USD});

    COUT<<"\nBTC/USD Order-Book with top 50 agreegated bids and asks\n";
    COUT<<btc_usd_ob<<"\n";

    traderBot->getGDAX()->fillCandleSticks(candlesticks, {currency_t::BTC, currency_t::USD}, time(nullptr)-3660, time(nullptr)-60, 60 );

    COUT<<"BTC-USD Candlesticks for last 1 hour with 1 minute interval ->"<<endl;

    COUT<<"[timestamp , open       , close       , low        , high      , mean,       ,stddev,    volume  ]\n";
    for(auto& candlestick : candlesticks)
    {
        COUT<<candlestick<<endl;
//PRINTF("[%ld, %f, %f, %f, %f, %f]\n", candlestick.time,
//	   candlestick.open, candlestick.high, candlestick.low, candlestick.close, candlestick.volume);
    }

    COUT<<"BTC-USD TradeHistory ->"<<endl;
#endif
}

TEST_CASE("gdax_fill_trades", "[basic][precommit]") {
  COUT << CBLUE << "TEST: gdax_fill_trades [basic]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  TickPeriod trades;

  trader_bot->getGDAX()->fillTrades(trades, CurrencyPair("BTC-USD"), 732689, 732679);

  REQUIRE(trades.size() == 10);
  REQUIRE(trades.front().getUniqueID() == 732679);
  REQUIRE(trades.back().getUniqueID() == 732688);

  TraderBot::deleteInstance();

#if 0
    vector<CurrencyPair> supported_currency_pairs;
    vector<Currency> supported_currencies;

    vector<candlestick_t> candlesticks;

    traderBot->getGDAX()->getQuote({currency_t::BTC, currency_t::USD});

    COUT<<"\nSupported currencies in GDAX are \n";

    traderBot->getGDAX()->getCurrencies(supported_currencies);

    for (auto currency : supported_currencies)
        COUT<<currency<<"\n";

    COUT<<"\nAccount details \n";
    unordered_map<currency_t, Account> accounts;

    traderBot->getGDAX()->getAccounts(accounts);

    for (auto account : accounts)
        COUT<<account.second<<"\n";

    COUT<<"\nSupported currency pairs in GDAX are \n";


    traderBot->getGDAX()->getCurrencyPairs(supported_currency_pairs);

    for (auto currency_pair : supported_currency_pairs) {
        COUT<<currency_pair << "\n";
        COUT<<"Ticker Price: " << traderBot->getGDAX()->getTicker(currency_pair.getCurrencyPair()) << "\n";
        COUT<<"Quote: \n" << traderBot->getGDAX()->getQuote(currency_pair.getCurrencyPair()) << "\n";
    }

    const OrderBook btc_usd_ob = traderBot->getGDAX()->getOrderBook({currency_t::BTC, currency_t::USD});

    COUT<<"\nBTC/USD Order-Book with top 50 agreegated bids and asks\n";
    COUT<<btc_usd_ob<<"\n";

    traderBot->getGDAX()->fillCandleSticks(candlesticks, {currency_t::BTC, currency_t::USD}, time(nullptr)-3660, time(nullptr)-60, 60 );

    COUT<<"BTC-USD Candlesticks for last 1 hour with 1 minute interval ->"<<endl;

    COUT<<"[timestamp , open       , close       , low        , high      , mean,       ,stddev,    volume  ]\n";
    for(auto& candlestick : candlesticks)
    {
        COUT<<candlestick<<endl;
//PRINTF("[%ld, %f, %f, %f, %f, %f]\n", candlestick.time,
//	   candlestick.open, candlestick.high, candlestick.low, candlestick.close, candlestick.volume);
    }

    COUT<<"BTC-USD TradeHistory ->"<<endl;
#endif
}

#define GDAX_BUY_TEST 1
#define GDAX_SELL_TEST 0
#define GDAX_CANCEL_TEST 0

TEST_CASE("gdax_limit_order", "[advanced]") {
  COUT << CBLUE << "TEST: gdax_limit_order [advanced]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  const char* args[] = {"exe", "-d", "o"};

  REQUIRE(!trader_bot->traderMain(3, args));

  CurrencyPair trading_pair = CurrencyPair("ETH-USD");

  GDAX* p_gdax = trader_bot->getGDAX();

  // set accounts
  p_gdax->updateAccounts();
  p_gdax->setMode({trading_pair});

  p_gdax->initForTrading(1_hour, Time::sNow(), Time::sNow() + 1_hour, false);

  p_gdax->updateAccounts();

  double current_price = p_gdax->getTicker(trading_pair);
  COUT << endl << trading_pair << " ticker price : " << CMAGENTA << current_price << CRESET << endl;

  while (p_gdax->getTrades(trading_pair)->size() == 0) this_thread::sleep_for(chrono::seconds(1));

  order_id_t order_id = "";

  trader_bot->ExitEarly();

#if GDAX_BUY_TEST
  const Account& usd_acc = p_gdax->getExchangeAccounts()->getAccount(trading_pair.getQuoteCurrency());
  double usd_avail = usd_acc.getAvailable();

  double buy_price = 136.64; //p_gdax->getBidPrice(trading_pair);

  if (usd_avail > p_gdax->getMinOrderAmount(currency_t::USD))
    order_id = p_gdax->placeOrder(trading_pair, (floor(usd_avail * 1e2) / 1e2), order_type_t::LIMIT,
                                  order_direction_t::BUY, (buy_price - 0.01), Time::sNow(), current_price);
#if GDAX_CANCEL_TEST
  if (!order_id.empty()) {
    COUT << "order_id = " << order_id << endl;
    this_thread::sleep_for(chrono::seconds(1));

    bool order_cancel_success = trader_bot->getGDAX()->cancelOrder(order_id);
    REQUIRE(order_cancel_success == true);
  }
#endif

#endif

#if GDAX_SELL_TEST
  const Account& crypto_acc = p_gdax->getExchangeAccounts()->getAccount(trading_pair.getBaseCurrency());
  double crypto_avail = crypto_acc.getAvailable();

  double sell_price = p_gdax->getAskPrice(trading_pair);

  if (crypto_avail > p_gdax->getMinOrderAmount(trading_pair.getBaseCurrency()))
    order_id = p_gdax->placeOrder(trading_pair, (floor(crypto_avail * 1e5) / 1e5), order_type_t::LIMIT,
                                  order_direction_t::SELL, (sell_price + 0.01), Time::sNow(), current_price);
#if GDAX_CANCEL_TEST
  if (!order_id.empty()) {
    COUT << "order_id = " << order_id << endl;
    this_thread::sleep_for(chrono::seconds(1));

    // cancel all pending orders
    bool order_cancel_success = trader_bot->getGDAX()->cancelOrder(order_id);
    REQUIRE(order_cancel_success == true);
  }
#endif

#endif

  TraderBot::deleteInstance();
}

TEST_CASE("gdax_market_order", "[advanced]") {
  COUT << CBLUE << "TEST: gdax_order [advanced]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  const char* args[] = {"exe", "-d", "o"};

  REQUIRE(!trader_bot->traderMain(3, args));

  GDAX* p_gdax = trader_bot->getGDAX();

  CurrencyPair currency_pair = CurrencyPair("ETH-USD");
  p_gdax->setMode({currency_pair});

  p_gdax->initForTrading(1_min, Time::sNow(), Time::sNow() + 1_hour, false);

  p_gdax->updateAccounts();

  p_gdax->getExchangeAccounts()->printDetails();

  const Account& crypto_acc = p_gdax->getExchangeAccounts()->getAccount(currency_pair.getBaseCurrency());
  const Account& usd_acc = p_gdax->getExchangeAccounts()->getAccount(currency_t::USD);
  double crypto_avail = crypto_acc.getAvailable();
  double usd_avail = usd_acc.getAvailable();

  while (p_gdax->getTrades(currency_pair)->size() == 0) this_thread::sleep_for(chrono::seconds(1));

  double current_price = p_gdax->getTicker(currency_pair);
  COUT << "current price: " << current_price << endl;

  order_id_t order_id = "";

#if GDAX_BUY_TEST
  if (usd_avail > p_gdax->getMinOrderAmount(currency_t::USD))
    order_id = p_gdax->placeOrder(currency_pair, (floor(usd_avail * 1e2) / 1e2), order_type_t::MARKET,
                                  order_direction_t::BUY, -1, Time::sNow(), current_price);
#endif
  COUT << "crypto available: " << crypto_avail << endl;

#if GDAX_SELL_TEST
  if (crypto_avail > p_gdax->getMinOrderAmount(currency_pair.getBaseCurrency()))
    order_id = p_gdax->placeOrder(currency_pair, (floor(crypto_avail * 1e5) / 1e5), order_type_t::MARKET,
                                  order_direction_t::SELL, -1, Time::sNow(), current_price);
#endif
  COUT << "usd available: " << usd_avail << endl;

  COUT << CCYAN << "\nOrder : " << order_id << endl << endl;

  p_gdax->getExchangeAccounts()->printDetails();

  TraderBot::deleteInstance();
}

TEST_CASE("gdax_mode", "[advanced][precommit]") {
  COUT << CBLUE << "TEST: gdax_mode [advanced]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  trader_bot->getGDAX()->addVirtualAccount(currency_t::BTC, 0);
  trader_bot->getGDAX()->addVirtualAccount(currency_t::USD, 1);
  trader_bot->getGDAX()->setMode({CurrencyPair("BTC-USD")}, exchange_mode_t::SIMULATION);
  trader_bot->getGDAX()->initForTrading(5_min, Time::sNow(), Time::sMax(), false);

  this_thread::sleep_for(chrono::seconds(60));

  trader_bot->getGDAX()->disconnectWebsocket();

  COUT << *trader_bot->getGDAX()->getTradeHistory(CurrencyPair("BTC-USD"))->getTickPeriod() << endl;

  TraderBot::deleteInstance();
}

TEST_CASE("gemini_fill_trades", "[basic]") {
  COUT << CBLUE << "TEST: gemini_fill_trades [basic]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  g_update_cass = true;  // required to create metadata table for the first time

  Gemini gemini(exchange_t::GEMINI, trader_bot->getConfig()["exchanges"]["gemini"]);

  TickPeriod trades(false);  // tick not consecutive

  int64_t num_trades_saved;

  num_trades_saved = gemini.storeRecentTrades(CurrencyPair("BTC-USD"));

  // gemini.fillTrades(trades, CurrencyPair("BTC-USD"), Time::sNow() - Duration(0,1,0,0), Time::sNow(), last_trade_id,
  // 100);

  COUT << "Saved " << num_trades_saved << "to database" << endl;

  TraderBot::deleteInstance();
}

TEST_CASE("gdax_accounts", "[basic][precommit]") {
  COUT << CBLUE << "TEST: gdax_accounts [basic]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  vector<CurrencyPair> trading_pairs = {CurrencyPair(currency_t::BTC, currency_t::USD),
                                        CurrencyPair(currency_t::ETH, currency_t::USD),
                                        CurrencyPair(currency_t::LTC, currency_t::USD)};
  trader_bot->getGDAX()->setMode(trading_pairs, exchange_mode_t::SIMULATION);

  trader_bot->getGDAX()->updateAccounts();
  trader_bot->getGDAX()->getExchangeAccounts()->printDetails();
  // COUT << *trader_bot->getGDAX()->getExchangeAccounts() << endl;

  TraderBot::deleteInstance();
}

TEST_CASE("gdax_cancel_orders", "[basic]") {
  COUT << CBLUE << "TEST: gdax_cancel_orders [basic]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  const char* args[] = {"exe", "-d", "o"};

  REQUIRE(!trader_bot->traderMain(3, args));

  trader_bot->getGDAX()->cancelAllOrders();

  TraderBot::deleteInstance();
}
