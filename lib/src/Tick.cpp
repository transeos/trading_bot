//
// Created by Subhagato Dutta on 11/12/17.
//

#include "Tick.h"

using namespace std;

const pair<string, string> Tick::m_unique_id = {"trade_id", "bigint"};
const field_t Tick::m_fields = {{"price", "double"}, {"size", "double"}};
const string Tick::m_type = "Tick";

void* Tick::getMemberPtr(size_t i) {
  switch (i) {
    case 0:
      return &(m_price);
    case 1:
      return &(m_size);
    default:
      return nullptr;
  }
}

void Tick::setMemberPtr(void* value, size_t i) {
  switch (i) {
    case 0:
      m_price = *(double*)value;
    case 1:
      m_size = *(double*)value;
    default:
      assert(0);
  }
}
