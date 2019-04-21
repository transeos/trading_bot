// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Subhagato Dutta on 9/21/17.
//
//*****************************************************************

#ifndef CRYPTOTRADER_TRADERUTILS_H
#define CRYPTOTRADER_TRADERUTILS_H

#include <cmath>
#include <iostream>
#include <vector>

class Time;
class Duration;

namespace TradeUtils {

template <typename T>
class GreaterPT {
 public:
  inline bool operator()(const T* lhs, const T* rhs) {
    return (*lhs > *rhs);
  }
};

template <typename T>
class LessPT {
 public:
  inline bool operator()(const T* lhs, const T* rhs) {
    return (*lhs < *rhs);
  }
};

template <typename T>
class CompT {
  inline bool operator()(const T& lhs, const T& rhs) {
    return (lhs < rhs);
  }
};

template <typename T>
class EqualT {
  inline bool operator()(const T& lhs, const T& rhs) {
    return (lhs == rhs);
  }
};

template <typename T>
static void displayT(const T& a_data) {
  std::cout << a_data << std::endl;
}

template <typename T>
static void displayT(T& a_data) {
  std::cout << a_data << std::endl;
}

static bool isTickerApproxEqual(double ticker1, double ticker2, double tolerance = 0.0001) {
  return ((2 * std::abs(ticker1 - ticker2) / (ticker1 + ticker2)) < tolerance);
}

time_t my_timegm(struct tm* t);

/**
 * http://www.itl.nist.gov/div898/software/dataplot/refman2/ch2/weighvar.pdf
 */
double mean(const std::vector<double>& x);
double weighted_mean(const std::vector<double>& x, const std::vector<double>& w);
double variance(const std::vector<double>& x);
double weighted_variance(const std::vector<double>& x, const std::vector<double>& w);
double weighted_incremental_variance(const std::vector<double>& x, const std::vector<double>& w);
double weightedStdDev(const std::vector<double>& price_arr, const std::vector<double>& volume_arr);

uint64_t getUTCFromISOTime(std::string iso_time);
std::string getISOTimeFromUTC(int64_t utc_time, bool millisecond = true);
time_t getTimeInSecs(const int a_yr, const int a_month, const int a_mday, const int a_hr, const int a_min,
                     const int a_sec);

bool isValidPath(const char* path);
bool isValidPath(const std::string& path);

std::string removeSpaces(const std::string& word);

// parses CSV and returns number of elements
int parseCSVLine(std::vector<std::string>& a_data, const std::string& line);

bool extractDateTimeFromCSV(const std::string& line, time_t& a_time);
bool extractDateTimeFromCSV(const std::string& line, Time& a_time);

inline uint32_t getDateFromTimeStamp(time_t timestamp);
inline int64_t getTimeFromTimeStamp(time_t timestamp);

int getUUIDHash(const std::string& a_uuid, const int a_numbits);

bool createDir(const std::string& a_dir);

double getSigmf(const double x, const double a, const double c);
};  // namespace TradeUtils

#endif  // CRYPTOTRADER_TRADERUTILS_H
