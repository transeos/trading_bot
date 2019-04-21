// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 27/06/18.
//

#pragma once

#include "utils/Logger.h"

// exit codes
enum class exit_code_t { cDefault = 1, cSetup, cFile_io, cInternal_issue };

// order of error_type_t enums should not be changed
enum class error_type_t {
  cNo_trader_home = 100,
  cInvaid_arg,
  cIncomplete_arg,
  cDel_dir,
  cInvalid_dir,
  cInvalid_file,
  cInvalid_datetime,
  cExport_file_format,
  cExport_time_range,
  cInvalid_user_id,
  cInvalid_json,
  cCass_bind_type,
  cCass_get_val_type,
  cCSV_type_error,
  cCtrl_time_range,
  cCandlestick_deque,
  cNo_Ctrl_TH_Init,
  cAccount_share,
  cSimulation_time_range,
  cLive_trading_no_conn,
  cNo_coinbase_api,
  cWebsocket_conn,
  cNo_market,
  cJson_exception,
  cCoinAPI_processing,
  cCoinAPI_config_parse,
  cCandlestickAlgo_init,
  cNoTickError
};

#define TRADER_HOME_ERROR CT_ERROR(error_type_t::cNo_trader_home, exit_code_t::cSetup) << "TRADER_HOME not defined\n"

#define INVALID_ARGUMENT_ERROR(arg1, arg2)                 \
  CT_ERROR(error_type_t::cInvaid_arg, exit_code_t::cSetup) \
      << "Invalid argument(s): " << arg1 << " " << arg2 << std::endl

#define INCOMPLETE_ARGUMENT_ERROR(arg1, arg2)                  \
  CT_ERROR(error_type_t::cIncomplete_arg, exit_code_t::cSetup) \
      << "Incomplete argument(s): " << arg1 << " " << arg2 << std::endl

#define DEL_DIR_ERROR(dir) \
  CT_ERROR(error_type_t::cDel_dir, exit_code_t::cFile_io) << "Not able to delete directory : " << dir << std::endl

#define INVALID_DIR_ERROR(dir) \
  CT_ERROR(error_type_t::cInvalid_dir, exit_code_t::cFile_io) << "cannot access directory : " << dir << std::endl

#define INVALID_FILE_ERROR(file) \
  CT_ERROR(error_type_t::cInvalid_file, exit_code_t::cFile_io) << "file not found : " << file << std::endl

#define DATETIME_FORMAT_ERROR(date) \
  CT_ERROR(error_type_t::cInvalid_datetime, exit_code_t::cSetup) << "invalid date time format : " << date << std::endl

#define EXPORT_FILEFORMAT_ERROR(date) \
  CT_ERROR(error_type_t::cExport_file_format, exit_code_t::cSetup) << "invalid file format : " << date << std::endl

#define EXPORT_TIME_RANGE_ERROR(start_time, end_time)             \
  CT_ERROR(error_type_t::cExport_time_range, exit_code_t::cSetup) \
      << "start time(" << start_time << ") is more than end time(" << end_time << ")\n"

#define USER_ID_ERROR(user_id)                                  \
  CT_ERROR(error_type_t::cInvalid_user_id, exit_code_t::cSetup) \
      << "User id must be a postive number : " << user_id << std::endl

#define INVALID_JSON_ERROR(file) \
  CT_ERROR(error_type_t::cInvalid_json, exit_code_t::cSetup) << "invalid json config : " << file << std::endl

#define CASS_BIND_TYPE_ERROR(type)                                      \
  CT_ERROR(error_type_t::cCass_bind_type, exit_code_t::cInternal_issue) \
      << "Incompatible Cassandra bind type : " << type << std::endl

#define CASS_GETVAL_TYPE_ERROR(type)                                       \
  CT_ERROR(error_type_t::cCass_get_val_type, exit_code_t::cInternal_issue) \
      << "Incompatible Cassandra get value type : " << type << std::endl

#define CSV_DATA_TYPE_ERROR(type)                                       \
  CT_ERROR(error_type_t::cCSV_type_error, exit_code_t::cInternal_issue) \
      << "Incompatible CSV data type : " << type << std::endl

#define CTRL_TIME_RANGE_ERROR(start_time, current_time)                                                         \
  CT_ERROR(error_type_t::cCtrl_time_range, exit_code_t::cSetup)                                                 \
      << "Real time trading cannot be started from past(" << start_time << "), current time = " << current_time \
      << std::endl

#define CANDLESTICK_DEQUE_ERROR(current_timestamp, last_timestamp, interval)        \
  CT_ERROR(error_type_t::cCandlestick_deque, exit_code_t::cInternal_issue)          \
      << "Candlestick deque is not valid : current timestamp=" << current_timestamp \
      << ", last timestamp=" << last_timestamp << ", interval=" << interval << std::endl

#define NO_CONTROLLER_TH_INIT_ERROR                                      \
  CT_ERROR(error_type_t::cNo_Ctrl_TH_Init, exit_code_t::cInternal_issue) \
      << "Controller needs to be initialized before initilizing logs in TradeHistory\n"

#define ACCOUNT_SHARE_ERROR(a_json) \
  CT_ERROR(error_type_t::cAccount_share, exit_code_t::cSetup) << "Invalid account share info : " << a_json << std::endl

#define SIMULATON_TIME_RANGE_ERROR(start_time, end_time)              \
  CT_ERROR(error_type_t::cSimulation_time_range, exit_code_t::cSetup) \
      << "Simulating from past(" << start_time << ") to future(" << end_time << ") is not supported\n"

#define LIVE_TRADING_CONN_ERROR(exchange)                              \
  CT_ERROR(error_type_t::cLive_trading_no_conn, exit_code_t::cDefault) \
      << "Unable to connect websocket. Can't do live trading in " << exchange << std::endl

#define NO_COINBASE_API_ERROR                                   \
  CT_ERROR(error_type_t::cNo_coinbase_api, exit_code_t::cSetup) \
      << "Can't place/cancel order(s) in Coinbase without correct API credentials\n"

#define WEBSOCKET_CONN_ERROR(attempt_description, exchange)             \
  CT_ERROR(error_type_t::cWebsocket_conn, exit_code_t::cInternal_issue) \
      << "Websocket is not connected. Can't " << attempt_description << "in " << exchange << std::endl

#define NO_MARKET_ERROR(currency, exchange)               \
  CT_ERROR(error_type_t::cNo_market, exit_code_t::cSetup) \
      << currency << " market doesn't exist in " << exchange << std::endl

#define JSON_EXCEPTION_ERROR(json_exception, file)             \
  CT_ERROR(error_type_t::cJson_exception, exit_code_t::cSetup) \
      << "Json exception : " << json_exception << " in " << file << std::endl

#define COINAPI_EXCEPTION_ERROR(topic)                               \
  CT_ERROR(error_type_t::cCoinAPI_processing, exit_code_t::cDefault) \
      << "processing JSON on CoinAPI " << topic << " processing\n"

#define COINAPI_PARSING_ERROR(issue, line, line_num, config_file)    \
  CT_ERROR(error_type_t::cCoinAPI_config_parse, exit_code_t::cSetup) \
      << issue << " " << line << " (line:" << line_num << ") in " << config_file << std::endl

#define CANDLESTICKALGO_INIT_ERROR(start_time, end_time)             \
  CT_ERROR(error_type_t::cCandlestickAlgo_init, exit_code_t::cSetup) \
      << "market state didn't change between " << start_time << " and " << end_time << std::endl

#define NO_TICK_ERROR(time_since_last_tick)                   \
  CT_ERROR(error_type_t::cNoTickError, exit_code_t::cDefault) \
      << "No new tick in last " << time_since_last_tick << " min" << std::endl
