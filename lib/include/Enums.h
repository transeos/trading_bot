// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 2/10/17
//
//*****************************************************************

#ifndef ENUMS_H
#define ENUMS_H

#include "CurrencyEnums.h"
#include "ExchangeEnums.h"

enum class rest_request_t { GET, POST, DELETE };

enum class candle_price_t { OPEN, CLOSE, LOW, HIGH, MEAN };

enum class exchange_mode_t { SIMULATION = 0, REAL, BOTH };

enum class order_type_t { LIMIT, MARKET, STOP };

enum class order_direction_t { BUY, SELL };

enum class trigger_type_t { MIN, MAX, CURRENT };

enum class indicator_t { MA, EMA, RSI, STDDEV, MACD };

enum class tradeAlgo_t { BASICMARKET = 0, BASICLIMIT };

enum class trade_algo_trigger_t { NEW_CANDLESTICK = 0, NEW_TICK, NEW_INTERVAL };

#endif  // ENUMS_H
