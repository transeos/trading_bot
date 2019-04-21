//
// Created by subhagato on 5/23/18.
//
#include <TimeUtils.h>

Duration operator"" _sec(unsigned long long s) {
  return Duration(0, 0, 0, s);
}

Duration operator"" _min(unsigned long long m) {
  return Duration(0, 0, static_cast<int>(m), 0);
}

Duration operator"" _hour(unsigned long long h) {
  return Duration(0, static_cast<int>(h), 0, 0);
}

Duration operator"" _day(unsigned long long d) {
  return Duration(static_cast<int>(d), 0, 0, 0);
}