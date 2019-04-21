// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 12/9/17
//

#include "Globals.h"
#include "TraderBot.h"
#include "utils/TraderUtils.h"
#include <fstream>

using namespace std;

// ============================================
// functions executed w.r.t different arguments
// ============================================

void dummyCallBack(string a_val) {
  // dummy call back functions for arguments
}

void setDumpRestJson(string a_val) {
  g_dump_rest_api_responses = true;
  COUT << "Dumping rest API response info\n";
}

void setDumpOrderJson(string a_val) {
  g_dump_order_responses = true;
  COUT << "Dumping order response info\n";
}

void setDumpTradesFromWebsocket(string a_val) {
  g_dump_trades_websocket = true;
  COUT << "Dumping trades captured through websocket\n";
}

void setFixDatabase(string a_val) {
  TraderBot::getInstance()->fixDatabase();
  COUT << "Repairing database by filling missing data\n";
}

void setCoinMarketCapCapture(string a_val) {
  TraderBot::getInstance()->captureCoinmarketcapUpdate();
}

void setCoinAPICapture(string a_val) {
  TraderBot::getInstance()->updateCoinapi();
}

void setGDAXCapture(string a_val) {
  TraderBot::getInstance()->captureGDAXUpdate();
}

void setGeminiCapture(string a_val) {
  TraderBot::getInstance()->captureGeminiUpdate();
}

void setCSVDatabaseDirectory(string dir) {
  // check if database directory exists
  if (!TradeUtils::isValidPath(dir)) {
    INVALID_DIR_ERROR(dir);
  }

  TraderBot::getInstance()->setCSVDatabaseDir(dir);
  COUT << "Database directory = " << dir << endl;
}

// file format:
// line 1: currency (e.g., "BTC")
// line 2: starting date in CSV format (e.g., 2017,8,1,0,0,0)
// line 3: ending date in CSV format (e.g., 2017,8,30,23,59,59)
// line 4: dump area
void getData(string a_filename) {
  if (!TradeUtils::isValidPath(a_filename)) {
    INVALID_FILE_ERROR(a_filename);
  }

  int lineIdx = 0;

  Time start_time, end_time;

  ifstream file(a_filename.c_str());
  string line;
  while (getline(file, line)) {
    istringstream iss(line);

    switch (lineIdx) {
      case 0:
        TraderBot::getInstance()->setExportedData(line);
        COUT << "Exporting " << line << " data";
        break;

      case 1:
        start_time = Time(line);
        if (!start_time.valid()) {
          DATETIME_FORMAT_ERROR(line);
        }
        COUT << " from " << start_time;
        break;

      case 2:
        end_time = Time(line);
        if (!end_time.valid()) {
          DATETIME_FORMAT_ERROR(line);
        }
        COUT << " to " << end_time << endl;
        break;

      case 3:
        TraderBot::getInstance()->setExportedDatabaseDir(line);
        COUT << " in " << line;
        break;

      default:
        EXPORT_FILEFORMAT_ERROR(a_filename);
    }

    lineIdx++;
  }

  if (start_time > end_time) {
    EXPORT_TIME_RANGE_ERROR(start_time, end_time);
  }

  TraderBot::getInstance()->setDataStartTime(start_time);
  TraderBot::getInstance()->setDataEndTime(end_time);
}

void enableUpdateCass(string a_val) {
  g_update_cass = true;
}

void disableRandom(string a_val) {
  g_random = false;
}

void enableGemini(string a_val) {
  TraderBot::getInstance()->enableGeminiEx();
}

void suppressWarnings(string a_val) {
#if ENABLE_LOGGING
  Logger::s_suppress_warnings = true;
#endif
}

void updateUserId(string a_val) {
  int user_id = 0;
  try {
    user_id = stoi(a_val);
  } catch (...) {
    USER_ID_ERROR(a_val);
  }

  if (user_id < 1) USER_ID_ERROR(a_val);

  TraderBot::getInstance()->setUserId(user_id);
}

void setControllerConfig(string a_filename) {
  if (!TradeUtils::isValidPath(a_filename)) {
    INVALID_FILE_ERROR(a_filename);
  }

  ifstream json_config(a_filename.c_str());
  try {
    TraderBot::getInstance()->setCtrlConfig(json::parse(json_config));
  } catch (...) {
    INVALID_JSON_ERROR(a_filename);
  }
}

// ==============================================
// TraderBot class arguments processing functions
// ==============================================

void TraderBot::populateArgumentsList() {
  // m_arg_parser.addArguments("--fullArg", "-shortArg", "argument description", <switch>, <function pointer>);
  // m_arg_parser.addArguments("--fullArg", "-shortArg", "argument description", true, <function pointer>,
  //                           secondaryArg);

  m_arg_parser.addArguments("--dump", "-d", "dumps json response from rest APIs", true, setDumpRestJson, "rj");

  m_arg_parser.addArguments("--dump", "-d", "dumps order json responses", true, setDumpOrderJson, "o");

  m_arg_parser.addArguments("--dump", "-d", "dumps trades captured through websocket", true, setDumpTradesFromWebsocket,
                            "t");

  m_arg_parser.addArguments("--fixDatabase", "-f", "fills missing data", true, setFixDatabase);

  m_arg_parser.addArguments("--captureCoinMarketCap", "-cm", "captures CoinMarketCap data to update Cassandra database",
                            true, setCoinMarketCapCapture);

  m_arg_parser.addArguments("--captureGDAX", "-cg", "capture GDAX data to update Cassandra database", true,
                            setGDAXCapture);

  m_arg_parser.addArguments("--captureGemini", "-ci", "capture Gemini data to update Cassandra database", true,
                            setGeminiCapture);

  m_arg_parser.addArguments("--captureCoinAPI", "-coin", "captures CoinAPI data to update Cassandra database", true,
                            setCoinAPICapture);

  m_arg_parser.addArguments("--legacyDatabaseDir", "-db", "takes legacy CoinMarketCap database directory as input",
                            false, setCSVDatabaseDirectory);

  m_arg_parser.addArguments("--getDataInCSV", "-g", "takes a file containing timestamps, dump directory path as input",
                            false, getData);

  m_arg_parser.addArguments("--updateCassandraDB", "-u", "update Cassandra database if new data is available", true,
                            enableUpdateCass);

  m_arg_parser.addArguments("--deterministicFlow", "-noRand", "disable all randomness", true, disableRandom);

  m_arg_parser.addArguments("--userId", "-uid", "Changes user id from dafault value of 1", false, updateUserId);

  m_arg_parser.addArguments("--run", "-r", "takes json config for trading controller", false, setControllerConfig);

  m_arg_parser.addArguments("--suppressWarnings", "-noWarn", "suppresses non-critical warnings", true,
                            suppressWarnings);

  m_arg_parser.addArguments("--retry", "-rt", "restarts crypto trader in case of crash/error", true, dummyCallBack);

  m_arg_parser.addArguments("--enableGemini", "-gem", "enable Gemini exchange", true, enableGemini);
}

// processes arguments provided to the main exe (cryptotrader)
// argc : number of arguments
// argv : argument string array containing all the arguments
void TraderBot::processArguments(const int argc, const char** argv) {
  COUT << CGREEN << "Processing arguments:\n";

  for (int arg_idx = 0; arg_idx < argc; ++arg_idx) COUT << CBLUE << argv[arg_idx] << " ";

  COUT << "\n====================\n";

  m_arg_parser.processArguments(argc, argv);

  COUT << "\n";

  checkArgumentsValidity();
}

void TraderBot::checkArgumentsValidity() const {
  // nothing here for now
}
