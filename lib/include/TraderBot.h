// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 13/9/17
//
//*****************************************************************

#ifndef TRADERBOT_H
#define TRADERBOT_H

#include "Enums.h"
#include "utils/Argument.h"
#include "utils/TimeUtils.h"
#include <ThreadPool.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

class Exchange;
class GDAX;
class Gemini;
class CoinMarketCap;
class CoinAPI;
class Controller;

#define CTRL_LOG "controller_logs"

using json = nlohmann::json;

// This the topmost singleton class.
// Hence, it can be accessed from everywhere.
//
// Method 1:
//	TraderBot* traderBot;
//	traderBot = traderBot->getInstance();
// Method 2:
// 	TraderBot* traderBot = TraderBot::getInstance();
//
class TraderBot {
 private:
  // private constructor and destructor
  TraderBot();
  ~TraderBot();

  // private copy constructor and assignment operator
  TraderBot(TraderBot&);
  TraderBot& operator=(TraderBot&);

  bool m_initialized;
  bool m_early_exit;

  static TraderBot* mp_handler;

  ThreadPool m_thread_pool;

  ArgumentParser m_arg_parser;

  // CoinMarketCap
  CoinMarketCap* mp_coinmarketcap;
  // Coin API
  CoinAPI* mp_coinapi;

  // GDAX exchange
  GDAX* mp_gdax;

  Gemini* mp_gemini;

  // trading controller
  Controller* mp_trading_ctrl;

  json m_config;

  void captureGDAX();

  void captureGemini();

  void captureCoinMarketCap();

  void captureCoinAPI();

  // add arguments in a list
  void populateArgumentsList();

  void processArguments(const int argc, const char** argv);

  void checkArgumentsValidity() const;

  void initialize();

  void resetGlobals() const;

  // checks for valid sizes of certain classes
  void checkForSize() const;

  // checks for max limit of certain parameters like number currencies, exchanges etc.
  void checkForAssumptionLimits() const;

  // flags from agrument processing
  bool m_capture_coinmarketcap_update;
  bool m_capture_gdax_update;
  bool m_capture_gemini_update;
  bool m_update_coinapi;
  bool m_fix_database;
  bool m_enable_gemini;
  int m_user_id;
  std::string m_csv_database_dir;
  std::string m_exported_database;
  std::string m_exported_data;
  Time m_get_data_start_time;
  Time m_get_data_end_time;
  json m_ctrl_config;

 public:
  static TraderBot* getInstance() {
    if (!mp_handler) mp_handler = new TraderBot();
    return mp_handler;
  }

  static void deleteInstance() {
    assert(mp_handler);
    DELETE(mp_handler);
  }

  json& getConfig() {
    return m_config;
  }

  int traderMain(const int argc = 0, const char** argv = NULL);

  // set flags from agrument processing
  void captureCoinmarketcapUpdate() {
    m_capture_coinmarketcap_update = true;
  }
  void captureGDAXUpdate() {
    m_capture_gdax_update = true;
  }
  void captureGeminiUpdate() {
    m_capture_gemini_update = true;
    m_enable_gemini = true;
  }
  void updateCoinapi() {
    m_update_coinapi = true;
  }
  void fixDatabase() {
    m_fix_database = true;
  }
  void enableGeminiEx() {
    m_enable_gemini = true;
  }
  void setUserId(const int a_user_id) {
    m_user_id = a_user_id;
  }
  void setCSVDatabaseDir(const std::string& a_csv_database_dir) {
    m_csv_database_dir = a_csv_database_dir;
  }
  void setExportedDatabaseDir(const std::string& a_csv_database_dir) {
    m_exported_database = a_csv_database_dir;
  }
  void setExportedData(const std::string& a_data) {
    m_exported_data = a_data;
  }
  void setDataStartTime(const Time a_time) {
    m_get_data_start_time = a_time;
  }
  void setDataEndTime(const Time a_time) {
    m_get_data_end_time = a_time;
  }
  void setCtrlConfig(const json& a_json) {
    m_ctrl_config = a_json;
  }

  bool isTradingMode() const {
    return (m_ctrl_config != json());
  }

  void ExitEarly() {
    m_early_exit = true;
  }

  CoinMarketCap* getCoinMarketCap() {
    return mp_coinmarketcap;
  }

  CoinAPI* getCoinAPI() {
    return mp_coinapi;
  }

  GDAX* getGDAX() {
    return mp_gdax;
  }

  Gemini* getGemini() {
    return mp_gemini;
  }

  ThreadPool& getThreadPool() {
    return m_thread_pool;
  }

  void setController(Controller* ap_tradingCtrl);
  Controller* getController() {
    return mp_trading_ctrl;
  }

  Exchange* getExchange(const exchange_t a_exchange_id) const;
};

#endif  // TRADERBOT_H
