//
// Created by subhagato on 11/24/17.
//

#ifndef CRYPTOTRADER_TIME_H
#define CRYPTOTRADER_TIME_H

#include "utils/Logger.h"
#include "utils/TraderUtils.h"
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sys/time.h>
#include <thread>

const unsigned long MAX_USECS = 1000000L;

// Time class to provide way to manipulate times with normal operators.
// When working with the innards of this class, it's useful to know
// that on Linux, things are defined like this:
//
// struct timespec {
//     tv_sec:       time_t
//     tv_nsec:      long int
// }
//
// struct timeval {
//     tv_sec:       time_t
//     suseconds_t:  long int
// }
//
// time_t:           long int
//

class Duration {
 private:
  int64_t m_duration;
  friend class Time;

 public:
  inline Duration() {
    m_duration = 0;
  }
  explicit inline Duration(int days, int hours, int minutes, int64_t seconds, int64_t milliseconds = 0,
                           int64_t microseconds = 0) {
    m_duration = (days * 24LL * 60LL * 60LL * 1000000LL + hours * 60LL * 60LL * 1000000LL + minutes * 60LL * 1000000LL +
                  seconds * 1000000LL + milliseconds * 1000L + microseconds);
  }
  explicit inline Duration(int64_t offset_micros) {
    m_duration = offset_micros;
  }

  inline Duration(const Duration& d) {
    this->m_duration = d.m_duration;
  }

  inline ~Duration() {}

  inline int64_t getDuration() const {
    return m_duration;
  }

  inline int32_t getDurationInSec() const {
    return static_cast<int32_t>(m_duration / 1000000L);
  }

  inline void wait() const {
    std::this_thread::sleep_for(std::chrono::microseconds(m_duration));
  }

  Duration& operator=(const int64_t rhs) {
    this->m_duration = rhs;
    return *this;
  }

  Duration& operator/=(int64_t denominator) {
    m_duration /= denominator;
    return *this;
  }

  Duration& operator*=(int64_t multiplier) {
    m_duration *= multiplier;
    return *this;
  }

  explicit operator int64_t() const {
    return this->m_duration;
  }

  bool operator==(const Duration& rhs) const {
    return m_duration == rhs.m_duration;
  }
  bool operator==(const int64_t& rhs) const {
    return m_duration == rhs;
  }
  Duration operator%(const Duration& rhs) const {
    return Duration(m_duration % rhs.m_duration);
  }
  bool operator!=(const Duration& rhs) const {
    return !(*this == rhs);
  }
  bool operator!=(const int64_t& rhs) const {
    return !(*this == rhs);
  }
  Duration operator%(const int64_t& rhs) const {
    return Duration(m_duration % rhs);
  }
  bool operator>=(const Duration& rhs) const {
    return m_duration >= rhs.m_duration;
  }
  bool operator<=(const Duration& rhs) const {
    return m_duration <= rhs.m_duration;
  }
  bool operator>(const Duration& rhs) const {
    return m_duration > rhs.m_duration;
  }
  bool operator<(const Duration& rhs) const {
    return m_duration < rhs.m_duration;
  }

  friend Duration operator+(const Duration& d1, int64_t offset_micros) {
    Duration d(d1.getDuration() + offset_micros);
    return d;
  }

  friend Duration operator-(const Duration& d1, int64_t offset_micros) {
    Duration d(d1.getDuration() - offset_micros);
    return d;
  }

  friend Duration operator+(const Duration& d1, const Duration& d2) {
    Duration d(d1.getDuration() + d2.getDuration());
    return d;
  }

  friend Duration operator-(const Duration& d1, const Duration& d2) {
    Duration d(d1.getDuration() - d2.getDuration());
    return d;
  }

  friend std::ostream& operator<<(std::ostream& os, const Duration& d) {
    int64_t total_microseconds = d.m_duration;
    int64_t days = total_microseconds / (24 * 60 * 60 * 1000000LL);
    total_microseconds %= (24 * 60 * 60 * 1000000LL);
    int64_t hours = total_microseconds / (60 * 60 * 1000000LL);
    total_microseconds %= (60 * 60 * 1000000LL);
    int64_t minutes = total_microseconds / (60 * 1000000LL);
    total_microseconds %= (60 * 1000000LL);
    int64_t seconds = total_microseconds / 1000000LL;
    int64_t microseconds = total_microseconds % 1000000LL;
    if (days > 0) os << days << " days ";
    if (hours > 0) os << hours << " hours ";
    if (minutes > 0) os << minutes << " minutes ";
    if (seconds > 0) os << seconds << " seconds ";
    if (microseconds > 0) os << microseconds << " microseconds";

    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }

  friend Duration operator*(const Duration& d, unsigned int multiplier) {
    Duration r = d;
    r *= multiplier;
    return r;
  }

  friend Duration operator/(const Duration& d, unsigned int divider) {
    Duration r = d;
    r /= divider;
    return r;
  }

  friend int64_t operator/(const Duration& d1, const Duration& d2) {
    return d1.getDuration() / d2.getDuration();
  }
};

namespace std {
template <>
struct hash<Duration> {
  std::size_t operator()(const Duration& k) const {
    using std::hash;

    // Compute  hash values for currency_t as integer:

    return (hash<int64_t>()((int64_t)k.getDuration()));
  }
};
}

class Time {
 public:
  inline Time() {
    m_timestamp = 0;
  }

  inline Time(const Time& t) {
    m_timestamp = t.m_timestamp;
  }

  explicit inline Time(const int32_t date, const int64_t time) {
    m_timestamp = Duration(date, 0, 0, 0, 0, time).getDuration();
  }

  inline Time(const int year, const int month, const int day, const int hour, const int minute, const int second,
              const int microseconds = 0) {
    tm t;

    t.tm_year = year - 1900;  // Year since 1900
    t.tm_mon = month - 1;     // 0-11
    t.tm_mday = day;          // 1-31
    t.tm_hour = hour;         // 0-23
    t.tm_min = minute;        // 0-59
    t.tm_sec = second;        // 0-61 (0-60 in C++11)

    time_t t2 = timegm(&t);

    m_timestamp = (int64_t)t2 * 1000000 + static_cast<int64_t>(microseconds);
  }

  inline Time(const timespec& t) {
    m_timestamp = t.tv_sec * 1000000LL + t.tv_nsec / 1000LL;
  }

  Time(std::string iso_time, bool check_regex = false) {
    int y;
    int M, d, h, m;
    double s;
    tm t;

    if (check_regex) {
      std::regex e("\\d{4}-\\d{1,2}-\\d{1,2}T\\d{1,2}:\\d{1,2}:\\d{1,2}(Z|\\.\\d{0,7}Z)");

      if (!std::regex_match(iso_time, e)) {
        CT_CRIT_WARN << "Invalid time format:" << iso_time << std::endl;
        m_timestamp = INT64_MAX;
        return;
      }
    }

    sscanf(iso_time.c_str(), "%d-%d-%dT%d:%d:%lfZ", &y, &M, &d, &h, &m, &s);

    t.tm_year = y - 1900;      // Year since 1900
    t.tm_mon = M - 1;          // 0-11
    t.tm_mday = d;             // 1-31
    t.tm_hour = h;             // 0-23
    t.tm_min = m;              // 0-59
    t.tm_sec = (int)floor(s);  // 0-61 (0-60 in C++11)

    time_t t2 = timegm(&t);

    if (errno == EOVERFLOW || M > 12 || d > 31 || h > 23 || m > 59 || s > 61) {
      m_timestamp = INT64_MAX;
      return;
    }

    m_timestamp = (int64_t)t2 * 1000000 + static_cast<int64_t>(round((s - floor(s)) * 1000000.0));
  }

  Time(std::string date_str, std::string time_str, double fraction) {
    int y;
    int M, d, h, m;
    int s;
    tm t;

    std::regex date_e("\\d{4}-\\d{1,2}-\\d{1,2}");
    std::regex time_e("\\d{1,2}:\\d{1,2}:\\d{1,2}");

    if (!(std::regex_match(date_str, date_e) && (time_str.empty() || std::regex_match(time_str, time_e)))) {
      CT_CRIT_WARN << "Invalid date/time format:" << date_str << " " << time_str << std::endl;
      m_timestamp = INT64_MAX;
      return;
    }

    sscanf(date_str.c_str(), "%d-%d-%d", &y, &M, &d);
    if (!time_str.empty())
      sscanf(time_str.c_str(), "%d:%d:%d", &h, &m, &s);
    else {
      h = 0;
      m = 0;
      s = 0;
    }

    t.tm_year = y - 1900;  // Year since 1900
    t.tm_mon = M - 1;      // 0-11
    t.tm_mday = d;         // 1-31
    t.tm_hour = h;         // 0-23
    t.tm_min = m;          // 0-59
    t.tm_sec = s;          // 0-61 (0-60 in C++11)

    time_t t2 = timegm(&t);

    if (errno == EOVERFLOW || M > 12 || d > 31 || h > 23 || m > 59 || s > 61) {
      m_timestamp = INT64_MAX;
      return;
    }

    m_timestamp = (int64_t)t2 * 1000000 + static_cast<int64_t>(fraction * 1000000.0);
  }

  explicit inline Time(const int64_t micros_since_epoch) {
    m_timestamp = micros_since_epoch;
  }

  inline ~Time() {}

  inline struct tm* utctime() const {
    time_t utc_timestamp = static_cast<time_t>(m_timestamp / 1000000LL);
    return ::gmtime(&utc_timestamp);
  }

  inline unsigned int hour() const {
    return utctime()->tm_hour;
  }

  inline unsigned int minute() const {
    return utctime()->tm_min;
  }

  inline unsigned int second() const {
    return utctime()->tm_sec;
  }

  inline unsigned int millisecond() const {
    return (m_timestamp % 1000000LL) / 1000LL;
  }

  inline unsigned int microsecond() const {
    return (m_timestamp % 1000000LL);
  }

  inline unsigned int day() const {
    return utctime()->tm_mday;
  }

  inline unsigned int month() const {
    return (utctime()->tm_mon + 1);
  }

  inline unsigned int year() const {
    return (utctime()->tm_year + 1900);
  }

  inline unsigned int yday() const {
    return utctime()->tm_yday;
  }

  inline bool valid() {
    return (m_timestamp != INT64_MAX);
  }

  inline Time quantize(Duration interval) {
    m_timestamp /= interval.getDuration();
    m_timestamp *= interval.getDuration();
    return *this;
  }

  Time& operator=(const Time& rhs) {
    this->m_timestamp = rhs.m_timestamp;
    return *this;
  }

  explicit operator int64_t() const {
    return this->m_timestamp;
  }

  explicit operator int32_t() const {
    return static_cast<int32_t>(this->m_timestamp / 1000000LL);
  }

  Time& operator=(const Duration& rhs) {
    this->m_timestamp = rhs.getDuration();
    return *this;
  }

  Time& operator=(const int64_t rhs) {
    this->m_timestamp = rhs;
    return *this;
  }

  bool operator==(const Time& rhs) const {
    return m_timestamp == rhs.m_timestamp;
  }
  bool operator!=(const Time& rhs) const {
    return !(*this == rhs);
  }
  bool operator>=(const Time& rhs) const {
    return m_timestamp >= rhs.m_timestamp;
  }
  bool operator<=(const Time& rhs) const {
    return m_timestamp <= rhs.m_timestamp;
  }
  bool operator>(const Time& rhs) const {
    return m_timestamp > rhs.m_timestamp;
  }
  bool operator<(const Time& rhs) const {
    return m_timestamp < rhs.m_timestamp;
  }

  static inline Time sNow() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int64_t timestamp = ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;
    return Time(timestamp);
  }

  inline Time& future(int days = 0, int hours = 0, int minutes = 0, int64_t seconds = 0, int64_t milliseconds = 0,
                      int64_t microseconds = 0) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    m_timestamp = ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;
    m_timestamp += (days * 24 * 60 * 60 * 1000000LL + hours * 60 * 60 * 1000000LL + minutes * 60 * 1000000LL +
                    seconds * 1000000LL + milliseconds * 1000L + microseconds);
    return *this;
  }

  static inline Time sMax() {
    return Time(INT64_MAX);
  }

  inline Time& future(Duration d) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    m_timestamp = ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;
    m_timestamp += d.getDuration();

    return *this;
  }

  inline Time& past(int days = 0, int hours = 0, int minutes = 0, int64_t seconds = 0, int64_t milliseconds = 0,
                    int64_t microseconds = 0) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    m_timestamp = ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;
    m_timestamp -= (days * 24 * 60 * 60 * 1000000LL + hours * 60 * 60 * 1000000LL + minutes * 60 * 1000000LL +
                    seconds * 1000000LL + milliseconds * 1000L + microseconds);
    return *this;
  }

  inline Time& past(Duration d) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    m_timestamp = ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;
    m_timestamp -= d.getDuration();

    return *this;
  }

  std::string toISOTimeString(bool millisecond = true) const {
    time_t utc_time_s;
    int64_t utc_time_ms;

    utc_time_s = (time_t)(m_timestamp / 1000000LL);

    if (millisecond) utc_time_ms = ((m_timestamp + 500) / 1000LL);  // round off from microsecond

    char buf[sizeof "1970-01-01T00:00:00.000Z"];
    strftime(buf, sizeof buf, "%FT%T", gmtime(&utc_time_s));
    if (millisecond)
      sprintf(buf, "%s.%03dZ", buf, (int)(utc_time_ms % 1000LL));
    else
      strcat(buf, "Z");

    return std::string(buf);
  }

  std::string toDateString(std::string seperator = "-") {
    assert(seperator.size() <= 1);
    char buf[sizeof "1970-01-01"];
    sprintf(buf, "%04d%s%02d%s%02d", year(), seperator.c_str(), month(), seperator.c_str(), day());
    return std::string(buf);
  }

  std::string toTimeString(std::string seperator = ":") {
    assert(seperator.size() <= 1);
    char buf[sizeof "00-00-00"];
    sprintf(buf, "%02d%s%02d%s%02d", hour(), seperator.c_str(), minute(), seperator.c_str(), second());
    return std::string(buf);
  }

  Time& operator+=(const Duration& rhs) {
    m_timestamp += rhs.getDuration();
    return *this;
  }

  Time& operator-=(const Duration& rhs) {
    m_timestamp -= rhs.getDuration();
    return *this;
  }

  Duration operator%(const Duration& rhs) const {
    return Duration(m_timestamp % rhs.getDuration());
  }

  Time& operator/=(int64_t denominator) {
    m_timestamp /= denominator;
    return *this;
  }

  Time& operator*=(int64_t multiplier) {
    m_timestamp *= multiplier;
    return *this;
  }

  // seconds since the epoch
  inline time_t seconds_since_epoch() const {
    return static_cast<time_t>(m_timestamp / 1000000LL);
  }

  inline int64_t millis_since_epoch() const {
    return (m_timestamp / 1000LL);
  }

  inline int64_t micros_since_epoch() const {
    return m_timestamp;
  }

  inline int64_t micros_since_midnight() const {
    return m_timestamp % (86400LL * 1000000LL);
  }

  inline int32_t days_since_epoch() const {
    return static_cast<int32_t>(m_timestamp / (86400LL * 1000000LL));
  }

  friend Time operator+(const Time& t1, int64_t offset_micros) {
    Time t = t1;
    t += Duration(offset_micros);
    return t;
  }

  friend Time operator-(const Time& t1, int64_t offset_micros) {
    Time t = t1;
    t -= Duration(offset_micros);
    return t;
  }

  friend Time operator+(const Time& t1, Duration offset) {
    Time t = t1;
    t += offset;
    return t;
  }

  friend Time operator-(const Time& t1, Duration offset) {
    Time t = t1;
    t -= offset;
    return t;
  }

  friend Time operator/(const Time& t, unsigned int denominator) {
    Time r = t;
    r /= denominator;
    return r;
  }

  friend int64_t operator/(const Time& t, const Duration& d) {
    return t.m_timestamp / d.getDuration();
  }

  friend Time operator*(const Time& t, unsigned int multiplier) {
    Time r = t;
    r *= multiplier;
    return r;
  }

  friend Duration operator-(const Time& t1, const Time& t2) {
    return Duration(t1.micros_since_epoch() - t2.micros_since_epoch());
  }

  friend std::ostream& operator<<(std::ostream& os, const Time& t) {
    struct tm* tmp = t.utctime();
    os << std::put_time(tmp, "%d-%m-%Y %H-%M-%S");
    os << "." << std::setfill('0') << std::setw(3) << t.millisecond();
    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }

 private:
  int64_t m_timestamp;  // time from epoc in microseconds
  friend class Duration;
};

Duration operator"" _sec(unsigned long long s);
Duration operator"" _min(unsigned long long m);
Duration operator"" _hour(unsigned long long h);
Duration operator"" _day(unsigned long long d);

#endif  // CRYPTOTRADER_TIME_H
