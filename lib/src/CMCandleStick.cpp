// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 25/11/17
//
//*****************************************************************

#include "CMCandleStick.h"

using namespace std;

const pair<string, string> CMCandleStick::m_unique_id = {"", ""};  // No unique id for CMCandleStick
const field_t CMCandleStick::m_fields = {
    {"market_cap_usd", "double"}, {"price_btc", "double"}, {"price_usd", "double"}, {"volume_usd", "double"}};
const string CMCandleStick::m_type = "CMCandleStick";
const Duration CMCandleStick::m_interval = 5_min;  // CMCandleStick data has interval value 5 minutes

void* CMCandleStick::getMemberPtr(size_t i) {
  switch (i) {
    case 0:
      return &m_market_cap_usd;
    case 1:
      return &m_price_btc;
    case 2:
      return &m_price_usd;
    case 3:
      return &m_volume_usd;
    default:
      return NULL;
  }
}
