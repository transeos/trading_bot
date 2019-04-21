// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 17/02/18.
//

#include "CoinAPITick.h"
#include "Globals.h"

using namespace std;

const pair<string, string> CoinAPITick::m_unique_id = {"unique_id", "bigint"};
const field_t CoinAPITick::m_fields = {{"price", "double"}, {"size", "double"}};
const string CoinAPITick::m_type = "CoinAPITick";

CoinAPITick::CoinAPITick(const Time a_coinapi_time, const double a_price, const double a_size, exchange_t a_exchange_id,
                         string a_uuid)
    : m_price(a_price), m_size(a_size) {
  computeUniqueID(a_coinapi_time, a_exchange_id, TradeUtils::getUUIDHash(a_uuid, 16));
}

void* CoinAPITick::getMemberPtr(const size_t i) {
  switch (i) {
    case 0:
      return &m_price;
    case 1:
      return &m_size;
    default:
      assert(0);
  }
  return NULL;
}

void CoinAPITick::setMemberPtr(const void* value, const size_t i) {
  switch (i) {
    case 0:
      m_price = *(double*)value;
    case 1:
      m_size = *(double*)value;
    default:
      assert(0);
  }
}

Time CoinAPITick::getTimeStamp() const {
  const Duration time_diff = Duration((m_flags >> 24) * 1000L);
  Time timestamp = (Time(2013, 0, 0, 0, 0, 0) + time_diff);
  return timestamp;
}

exchange_t CoinAPITick::getExchangeId() const {
  int exchange_id_int = ((m_flags >> 16) & 0xFF);
  return static_cast<exchange_t>(exchange_id_int);
}

// 63    : sign bit (1 bit)
// 62-24 : time from 2013 (39 bit), max yr = 2030
// 23-16 : exchange id (8 bits)
// 15-0  : hash of UUID (16 bits)
void CoinAPITick::computeUniqueID(const Time a_coinapi_time, exchange_t a_exchange_id, int64_t a_hash) {
  const Duration time_diff = (a_coinapi_time - Time(2013, 0, 0, 0, 0, 0));

  int64_t time_diff_int = (time_diff.getDuration() / 1000L);
  assert(time_diff_int < 549755813888L);

  int64_t exchange_id_int = static_cast<int64_t>(a_exchange_id);
  ASSERT(exchange_id_int < 256);

  assert(a_hash < 65536);

  // compute unique id
  int64_t unique_id = a_hash;
  unique_id += ((exchange_id_int & 0xFF) << 16);
  unique_id += ((time_diff_int & 0x7FFFFFFFFF) << 24);

  m_flags = unique_id;
  ASSERT(((getTimeStamp() / 1000L) == (a_coinapi_time / 1000L)) && (getExchangeId() == a_exchange_id));
}
