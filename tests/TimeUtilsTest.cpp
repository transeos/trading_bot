//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 13/01/18.
//
// time utils test code.

#include <catch2/catch.hpp>
#include <regex>
#include <thread>

#include "CandlePeriod.h"
#include "CoinAPI.h"
#include "TraderBot.h"

using namespace std;

TEST_CASE("time", "[basic][precommit]") {
  COUT << CBLUE << "TEST: time [basic]\n";

  Time tp1("2017-11-29T14:00:00.000Z", true);
  Time tp2("2017-05-29T14:00:00Z", true);
  Time tp3("2017-1-29T14:00:00.000Z", true);
  Time tp4("2017-1-3T14:00:00.000Z", true);
  Time tp5("2017-1-3T14:00:00Z", true);
  Time tp6("3017-11-29T14:00:00.000Z", true);

  Time tf1("2017-11-32T14:00:00.000Z", true);
  Time tf2("2017-11-29T24:00:00.000Z", true);
  Time tf3("2017-11-29T14:00:00000Z", true);
  Time tf4("2017-02-29T14:00:00.000N", true);
  Time tf5("2017-15-3T14:00:00Z", true);
  Time tf6("hjkdsgadkjgaKJ", true);

  CHECK(tp1.valid());
  CHECK(tp2.valid());
  CHECK(tp3.valid());
  CHECK(tp4.valid());
  CHECK(tp5.valid());
  CHECK(tp6.valid());

  CHECK(!tf1.valid());
  CHECK(!tf2.valid());
  CHECK(!tf3.valid());
  CHECK(!tf4.valid());
  CHECK(!tf5.valid());
  CHECK(!tf6.valid());
}

TEST_CASE("duration", "[basic][precommit]") {
  COUT << CBLUE << "TEST: duration [basic]\n";

  Duration d = 2_day + 1_hour + 10_min + 5_sec;

  REQUIRE(d == Duration(2, 1, 10, 5));
}
