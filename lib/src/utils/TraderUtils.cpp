// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 29/04/18.
//

#include "utils/TraderUtils.h"
#include "Globals.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include <sys/stat.h>

using namespace std;

namespace TradeUtils {

time_t my_timegm(struct tm* t) {
  /* struct tm to seconds since Unix epoch */

  long year;
  time_t result;
#define MONTHSPERYEAR 12 /* months per calendar year */
  static const int cumdays[MONTHSPERYEAR] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

  /*@ +matchanyintegral @*/
  year = 1900 + t->tm_year + t->tm_mon / MONTHSPERYEAR;
  result = (year - 1970) * 365 + cumdays[t->tm_mon % MONTHSPERYEAR];
  result += (year - 1968) / 4;
  result -= (year - 1900) / 100;
  result += (year - 1600) / 400;
  if ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0) && (t->tm_mon % MONTHSPERYEAR) < 2) result--;
  result += t->tm_mday - 1;
  result *= 24;
  result += t->tm_hour;
  result *= 60;
  result += t->tm_min;
  result *= 60;
  result += t->tm_sec;
  if (t->tm_isdst == 1) result -= 3600;
  /*@ -matchanyintegral @*/
  return (result);
}

double mean(const vector<double>& x) {
  assert(x.size());

  double sum_xi = 0;
  size_t n = x.size();
  size_t i;
  for (i = 0; i < n; i++) {
    sum_xi += x[i];
  }
  return sum_xi / n;
}

double weighted_mean(const vector<double>& x, const vector<double>& w) {
  assert(x.size() && (x.size() == w.size()));

  double sum_wixi = 0;
  double sum_wi = 0;

  size_t n = x.size();
  size_t i;
  for (i = 0; i < n; i++) {
    sum_wixi += w[i] * x[i];
    sum_wi += w[i];
  }

  assert(sum_wi);
  return sum_wixi / sum_wi;
}

double variance(const vector<double>& x) {
  assert(x.size() > 1);

  double mean_x = mean(x);
  double dist, dist2;
  double sum_dist2 = 0;

  size_t n = x.size();

  size_t i;
  for (i = 0; i < n; i++) {
    dist = x[i] - mean_x;
    dist2 = dist * dist;
    sum_dist2 += dist2;
  }

  return sum_dist2 / (n - 1);
}

double weighted_variance(const vector<double>& x, const vector<double>& w) {
  assert(x.size() && (x.size() == w.size()));

  double xw = weighted_mean(x, w);
  double dist, dist2;
  double sum_wi_times_dist2 = 0;
  double sum_wi = 0;
  size_t n = x.size();
  size_t n_prime = 0;
  size_t i;

  for (i = 0; i < n; i++) {
    dist = x[i] - xw;
    dist2 = dist * dist;
    sum_wi_times_dist2 += w[i] * dist2;
    sum_wi += w[i];

    if (w[i] > 0) n_prime++;
  }

  if (n_prime > 1) {
    double weight = ((float)((n_prime - 1) * sum_wi) / n_prime);
    assert(weight > 0);
    return (sum_wi_times_dist2 / weight);
  } else {
    return 0.0f;
  }
}

double weighted_incremental_variance(const vector<double>& x, const vector<double>& w) {
  assert(x.size() && (x.size() == w.size()));

  double volume = w[0];
  double mean = x[0];
  double variance = 0;

  double price;
  double size;

  double volume_new;
  double mean_new;
  double variance_new;

  double Sn;
  double Sn_new;

  // math taken from http://people.ds.cam.ac.uk/fanf2/hermes/doc/antiforgery/stats.pdf

  for (size_t i = 1; i < x.size(); i++) {
    price = x[i];
    size = w[i];

    volume_new = volume + size;
    mean_new = (mean * volume + price * size) / volume_new;

    Sn = volume * variance;
    Sn_new = Sn + size * (price - mean) * (price - mean_new);
    variance_new = Sn_new / volume_new;

    volume = volume_new;
    mean = mean_new;
    variance = variance_new;
  }

  return variance_new;
}

double weightedStdDev(const vector<double>& price_arr, const vector<double>& volume_arr) {
  assert(price_arr.size() && (price_arr.size() == volume_arr.size()));

  double variance_weighted = 0.0;
  size_t count = price_arr.size();

  if (count > 1) {
    double sum_weights = 0.0, meanWeighted = 0.0, m2 = 0.0;

    for (size_t i = 0; i < count; i++) {
      double temp = volume_arr[i] + sum_weights;
      double delta = price_arr[i] - meanWeighted;
      double r = delta * volume_arr[i] / temp;

      meanWeighted += r;
      m2 += sum_weights * delta * r;  // Alternatively, m2 += weights[i] * delta * (samples[i]−meanWeighted)
      sum_weights = temp;
    }

    variance_weighted = (m2 / sum_weights) * ((double)count / (count - 1));  // sample variance
  }

  return sqrt(variance_weighted);
}

uint64_t getUTCFromISOTime(string iso_time) {
  int y;
  int M, d, h, m;
  float s;
  double dummy;
  tm t;

  sscanf(iso_time.c_str(), "%d-%d-%dT%d:%d:%fZ", &y, &M, &d, &h, &m, &s);

  t.tm_year = y - 1900;      // Year since 1900
  t.tm_mon = M - 1;          // 0-11
  t.tm_mday = d;             // 1-31
  t.tm_hour = h;             // 0-23
  t.tm_min = m;              // 0-59
  t.tm_sec = (int)floor(s);  // 0-61 (0-60 in C++11)

  time_t t2 = my_timegm(&t);

  return 1000 * (uint64_t)t2 + (uint64_t)(1000.f * modf(s, &dummy));
}

string getISOTimeFromUTC(int64_t utc_time, bool millisecond) {
  time_t utc_time_s;

  if (millisecond)
    utc_time_s = (time_t)(utc_time / 1000LL);
  else
    utc_time_s = (time_t)utc_time;

  char buf[sizeof "1970-01-01T00:00:00.000Z"];
  strftime(buf, sizeof buf, "%FT%T", gmtime(&utc_time_s));
  if (millisecond)
    sprintf(buf, "%s.%03dZ", buf, (int)(utc_time % 1000LL));
  else
    strcat(buf, "Z");

  return string(buf);
}

time_t getTimeInSecs(const int a_yr, const int a_month, const int a_mday, const int a_hr, const int a_min,
                     const int a_sec) {
  assert((a_yr > 2000) && (a_month > 0) && (a_month <= 12) && (a_mday > 0) && (a_mday < 32) && (a_hr >= 0) &&
         (a_hr < 24) && (a_min >= 0) && (a_min < 60) && (a_sec >= 0) && (a_sec < 60));

  time_t start_time = time(0);
  tm* time_info;
  time_info = gmtime(&start_time);

  time_info->tm_year = (a_yr - 1900);
  time_info->tm_mon = (a_month - 1);
  time_info->tm_mday = a_mday;
  time_info->tm_hour = a_hr;
  time_info->tm_min = a_min;
  time_info->tm_sec = a_sec;

  return (mktime(time_info) + time_info->tm_gmtoff);
}

// It can be used for both files and folders.
bool isValidPath(const char* path) {
  struct stat info;
  return (stat(path, &info) == 0);
}
bool isValidPath(const string& path) {
  return isValidPath(path.c_str());
}

// remove unnecessary spaces from start and end
string removeSpaces(const string& word) {
  string new_word = word;

  if (new_word.size() == 0) return new_word;

  while (new_word.at(0) == ' ') new_word = new_word.substr(1, (new_word.size() - 1));

  if (new_word.size() < 2) return new_word;

  while (new_word.at(new_word.size() - 1) == ' ') new_word = new_word.substr(0, (new_word.size() - 1));

  return new_word;
}

// parses CSV and returns number of elements
int parseCSVLine(vector<string>& a_data, const string& line) {
  int new_word_start = 0;

  string sub_str = "";
  for (size_t char_idx = 0; char_idx < line.size(); ++char_idx) {
    if (line.at(char_idx) == ',') {
      sub_str = line.substr(new_word_start, (char_idx - new_word_start));
      sub_str = removeSpaces(sub_str);
      a_data.push_back(sub_str);

      new_word_start = (char_idx + 1);
    }
  }
  sub_str = line.substr(new_word_start, (line.size() - new_word_start));
  sub_str = removeSpaces(sub_str);
  a_data.push_back(sub_str);

  return a_data.size();
}

bool extractDateTimeFromCSV(const string& line, time_t& a_time) {
  // format: yr,month,mday,hr,min,sec

  vector<string> data;

  int num_elements = parseCSVLine(data, line);
  if (num_elements == 6) {
    int yr = stoi(data[0]);
    int month = stoi(data[1]);
    int mday = stoi(data[2]);
    int hr = stoi(data[3]);
    int min = stoi(data[4]);
    int sec = stoi(data[5]);

    a_time = getTimeInSecs(yr, month, mday, hr, min, sec);
    return true;
  }
  return false;
}

bool extractDateTimeFromCSV(const string& line, Time& a_time) {
  time_t time = 0;
  bool success = extractDateTimeFromCSV(line, time);

  a_time = Duration(0, 0, 0, time, 0, 0);  // assuming a_time is in seconds

  return success;
}

// returns number of days from 1970-1-1
uint32_t getDateFromTimeStamp(time_t timestamp) {
  return (uint32_t)(timestamp / (1000LL * 60 * 60 * 24));
}

// returns time from start of day in nanoseconds
int64_t getTimeFromTimeStamp(time_t timestamp) {
  return 1000000LL * (static_cast<int64_t>(timestamp % (1000LL * 60 * 60 * 24)));
}

int getUUIDHash(const string& a_uuid, const int a_numbits) {
  int hash_val = 0;

  int split_str_len = 1;
  int remaining_num = a_numbits;
  while (remaining_num > 2) {
    remaining_num /= 2;
    split_str_len++;
  }

  vector<string> uuid_parts = StringUtils::split(a_uuid, '-');

  for (size_t partIdx = 0; partIdx < uuid_parts.size(); ++partIdx) {
    string uuid_part = uuid_parts[partIdx];
    int str_len = uuid_part.length();

    for (int idx = 0; idx < str_len; idx = idx + split_str_len) {
      string hex_str = uuid_part.substr(idx, split_str_len);

      int cur_val = 0;
      stringstream ss;
      ss << hex << hex_str;
      ss >> cur_val;

      hash_val ^= cur_val;
      ASSERT(hash_val < pow(2, a_numbits));
    }
  }

  return hash_val;
}

bool createDir(const string& a_dir) {
  const string command = "mkdir -p " + a_dir;
  const int exit_code = system(command.c_str());

  if (exit_code) {
    ASSERT(0);
    return false;
  }

  return true;
}

// y = sigmf(x,[a c])
//                   1
// f(x,a,c) = ---------------
//            1 + e^(−a(x−c))
double getSigmf(const double x, const double a, const double c) {
  const double exp_power = (((-1) * a) * (x - c));
  const double denominator = (1 + exp(exp_power));
  const double sigmf = (1 / denominator);

  return sigmf;
}

};  // namespace TradeUtils
