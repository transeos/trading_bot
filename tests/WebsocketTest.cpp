//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 13/01/18.
//
// This consists websocket test code.

#include <catch2/catch.hpp>

#include "TraderBot.h"
#include "exchanges/GDAX.h"

using namespace std;

TEST_CASE("websocket", "[long]") {
  COUT << CBLUE << "TEST: websocket [long]\n";

  TraderBot* trader_bot = TraderBot::getInstance();
  REQUIRE(!trader_bot->traderMain());

  GDAX* gdax = trader_bot->getGDAX();

  gdax->addVirtualAccount(currency_t::ETH, 0);
  gdax->addVirtualAccount(currency_t::USD, 1);

  const CurrencyPair currency_pair("ETH-USD");
  gdax->setMode({currency_pair}, exchange_mode_t::SIMULATION);

  gdax->connectWebsocket();

  gdax->subscribeToTopic();

  Quote restapi_quote;
  Quote websocket_quote;

  for (int i = 0; i < (1 << 30); i++) {
    this_thread::sleep_for(chrono::milliseconds(100));
    try {
      restapi_quote = gdax->getRestAPIQuote(currency_pair);
    } catch (...) {
      continue;
    }

    //    if( i%30 == 29){
    //      gdax->rePopulateOrderBook();
    //    }

    websocket_quote = gdax->getQuote(currency_pair);

    if (websocket_quote != restapi_quote) {
      CT_CRIT_WARN << "Quote mistmatch-[RestAPI]:" << restapi_quote << "\t";
    }

    COUT << "[Websocket]:" << websocket_quote;
    COUT << "  Bid:" << gdax->getBidPrice(currency_pair);
    COUT << ", Ask: " << gdax->getAskPrice(currency_pair) << endl << flush;

    // if (websocket_quote.getBidPrice() >= websocket_quote.getAskPrice())
    // std::cout << "\e[2J";
    // COUT << "[Websocket]:" << *gdax->getOrderBook(CurrencyPair("ETH-USD")) << endl << flush;
  }

  TraderBot::deleteInstance();

#if 0  // websocket code
	string uri = "wss://ws-feed.gdax.com";

	try {
		perftest endpoint;
		endpoint.start(uri);
	} catch (const exception & e) {
		COUT<< e.what() << endl;
	} catch (websocketpp::lib::error_code e) {
		COUT<< e.message() << endl;
	} catch (...) {
		COUT<< "other exception" << endl;
	}
#endif
}
