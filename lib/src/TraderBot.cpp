// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 16/9/17
//

#include "TraderBot.h"
#include "CoinAPI.h"
#include "CoinMarketCap.h"
#include "Controller.h"
#include "Tick.h"
#include "dbServer.h"
#include "exchanges/GDAX.h"
#include "exchanges/Gemini.h"

#define CONFIG_FILE "configs/static_config.json"

using namespace std;

void signal_handler(int signal) {
  sigset_t signalmask;
  sigemptyset(&signalmask);
  sigaddset(&signalmask, signal);
  sigprocmask(SIG_UNBLOCK, &signalmask, NULL);

  if (signal == SIGCONT) {
    std::signal(SIGCONT, SIG_DFL);
    std::signal(SIGTSTP, signal_handler);
    kill(getpid(), SIGCONT);
  } else {
    std::signal(signal, SIG_DFL);
    std::thread t([=] {
      g_critcal_task.lock();
      kill(getpid(), signal);
      g_critcal_task.unlock();
    });
    t.detach();
  }
}

void TraderBot::captureGDAX() {
  COUT << CGREEN << endl << "Capturing data from Coinbase..." << endl;

  while (m_capture_gdax_update) {
    for (auto iter : mp_gdax->getMarkets()) {
      if (iter.second->getTradeHistory()) mp_gdax->storeRecentTrades(iter.first);
    }

    // sleep for 60 second
    this_thread::sleep_for(chrono::seconds(60));
  }
}

void TraderBot::captureGemini() {
  COUT << CGREEN << endl << "Capturing data from Gemini..." << endl;

  while (m_capture_gemini_update) {
    for (auto iter : mp_gemini->getMarkets()) {
      if (iter.second->getTradeHistory()) mp_gemini->storeRecentTrades(iter.first);
    }

    // sleep for 60 second
    this_thread::sleep_for(chrono::seconds(60));
  }
}

void TraderBot::captureCoinMarketCap() {
  COUT << CGREEN << endl << "Capturing data from CoinMarketCap..." << endl;

  while (m_capture_coinmarketcap_update) {
    mp_coinmarketcap->storeRecentCandles();

    // sleep for 60 seconds
    this_thread::sleep_for(chrono::seconds(60));
  }
}

void TraderBot::captureCoinAPI() {
  while (m_update_coinapi) {
    mp_coinapi->getHistory().syncToDatabase();

    // sleep for few seconds
    this_thread::sleep_for(chrono::seconds(COINAPI_SYNC_INTERVAL.getDuration() / 1000000L));
    COUT << endl;
  }
}

TraderBot::TraderBot()
    : m_thread_pool(16)  // 16 concurrent threads in thread_pool
{
  COUT << CGREEN << "\n\n== TraderBot created ==\n\n";

  const char* trader_home = getenv("TRADER_HOME");
  g_trader_home = (trader_home ? trader_home : "");

  const char* cass_ip = getenv("CASSANDRA_IP");
  g_cassandra_ip = (cass_ip ? cass_ip : "127.0.0.1");

  if (g_trader_home.empty()) {
    TRADER_HOME_ERROR;
  }

  populateArgumentsList();

  mp_coinmarketcap = NULL;

  mp_coinapi = NULL;

  mp_gdax = NULL;

  mp_gemini = NULL;

  mp_trading_ctrl = NULL;

  m_initialized = false;
  m_early_exit = false;

  // flags
  m_capture_coinmarketcap_update = false;
  m_capture_gdax_update = false;
  m_capture_gemini_update = false;
  m_update_coinapi = false;
  m_fix_database = false;
  m_enable_gemini = false;
  m_user_id = 1;
  m_csv_database_dir = "";
  m_exported_database = "";
  m_exported_data = "";
  m_get_data_start_time = Time();
  m_get_data_end_time = Time();
  m_ctrl_config = json();
}

TraderBot::~TraderBot() {
  if (m_early_exit) exit(0);

  DELETE(mp_coinmarketcap);

  DELETE(mp_coinapi);

  DELETE(mp_gdax);

  DELETE(mp_gemini);

  DELETE(mp_trading_ctrl);

  g_exiting = true;

  m_thread_pool.shutdown();

  // wait for 1 sec to all threads to finish
  this_thread::sleep_for(chrono::seconds(1));
}

int TraderBot::traderMain(const int argc, const char** argv) {
  // reset globals
  resetGlobals();

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTSTP, signal_handler);
  std::signal(SIGCONT, signal_handler);

  // process arguments
  // argc = number of arguments + 1
  // argv[0] : exe path, argv[1] to argv(argc - 2)] : arguments.
  // Hence, address of the pointer passed to this function is increased by 1.
  if (argv)
    processArguments((argc - 1), (argv + 1));
  else
    assert(!argc);

  // create new controller directory
  string command = "rm -rf ";
  command += CTRL_LOG;
  const int exit_code = system(command.c_str());
  if (exit_code) DEL_DIR_ERROR(CTRL_LOG);
  TradeUtils::createDir(CTRL_LOG);

  // initialize
  initialize();

  Controller::cancelPendingOrdersFromPreviousRun();

#ifndef DISABLE_THREAD
  vector<thread> run_threads;
#endif

  // restore legacy data
  if (m_csv_database_dir != "") {
    mp_gdax->restoreFromCSV(m_csv_database_dir);
    if (mp_gemini) mp_gemini->restoreFromCSV(m_csv_database_dir);
    mp_coinapi->getHistory().restoreFromCSV(m_csv_database_dir);
  }

  if (m_capture_gdax_update) {
#ifdef DISABLE_THREAD
    captureGDAX();
#else
    run_threads.push_back(thread(&TraderBot::captureGDAX, this));
#endif
  }

  if (m_capture_coinmarketcap_update) {
#ifdef DISABLE_THREAD
    captureCoinMarketCap();
#else
    run_threads.push_back(thread(&TraderBot::captureCoinMarketCap, this));
#endif
  }

  if (m_capture_gemini_update) {
#ifdef DISABLE_THREAD
    captureGemini();
#else
    run_threads.push_back(thread(&TraderBot::captureGemini, this));
#endif
  }

  if (m_update_coinapi) {
    COUT << CGREEN << "capturing data from CoinAPI" << endl;

    mp_coinapi->connectWebsocket();
    mp_coinapi->subscribeToTopics();

#ifdef DISABLE_THREAD
    captureCoinAPI();
#else
    run_threads.push_back(thread(&TraderBot::captureCoinAPI, this));
#endif
  }

  // export data of certain time range
  if (m_get_data_start_time != Time()) {
    COUT << CGREEN << "exporting data\n";

    const currency_t currency = Currency::sStringToCurrency(m_exported_data);
    if (currency != currency_t::UNDEF) {
    } else if (mp_gdax->saveToCSV(m_exported_data, m_get_data_start_time, m_get_data_end_time, m_exported_database)) {
      COUT << CBLUE << "COINBASE data exported.\n";
    } else if (mp_gemini->saveToCSV(m_exported_data, m_get_data_start_time, m_get_data_end_time, m_exported_database)) {
      COUT << CBLUE << "GEMINI data exported.\n";
    } else if (mp_coinapi->getHistory().saveToCSV(m_exported_data, m_get_data_start_time, m_get_data_end_time,
                                                  m_exported_database)) {
      COUT << CBLUE << "COINAPI data exported.\n";
    }

    return 0;
  }

  // repair database
  if (m_fix_database) {
    for (auto iter : mp_gdax->getMarkets()) {
      if (iter.second->getTradeHistory()) mp_gdax->repairDatabase(iter.first);
    }

    if (mp_gemini) {
      for (auto iter : mp_gemini->getMarkets()) {
        // TODO: Partial repair in gemimi is not supported right now.
        if (iter.second->getTradeHistory()) mp_gemini->repairDatabase(iter.first, true);
      }
    }

    mp_coinmarketcap->repairDatabase();

    return 0;
  }

  // run trading controller
  if (isTradingMode()) {
    mp_trading_ctrl = new Controller(m_ctrl_config);
    mp_trading_ctrl->init(m_ctrl_config);
    mp_trading_ctrl->run();
  }

#ifndef DISABLE_THREAD
  for (auto& th : run_threads) th.join();
#endif

  return 0;
}

void TraderBot::initialize() {
  if (m_initialized) return;

  COUT << CGREEN << "\n\n== Initializing TraderBot ==\n\n";

  ifstream json_config(g_trader_home + "/" + CONFIG_FILE);

  m_config = json::parse(json_config);
  // checks
  checkForAssumptionLimits();
  checkForSize();

  if (!g_cass_server) g_cass_server = new CassServer();

  mp_coinmarketcap = new CoinMarketCap();

  mp_coinapi = new CoinAPI(m_config["coinapi"]["api-credential"][m_user_id - 1]["key"].get<string>());

  mp_gdax = new GDAX(exchange_t::COINBASE, m_config["exchanges"]["coinbase"]);

  if (m_enable_gemini) mp_gemini = new Gemini(exchange_t::GEMINI, m_config["exchanges"]["gemini"]);

  m_thread_pool.init();

  m_initialized = true;
}

void TraderBot::resetGlobals() const {
  // All globals are not listed here.

  g_dump_rest_api_responses = false;
  g_dump_order_responses = false;
  g_dump_trades_websocket = false;
  g_update_cass = false;
  g_random = true;
  g_exiting = false;
  g_order_idx = 0;
}

void TraderBot::checkForSize() const {
  // Tick
  assert(((sizeof(Tick) + 7) / 8) == (((sizeof(int64_t) * 2) + (sizeof(double) * 2) + 7) / 8));

  // CMCandleStick
  assert(((sizeof(CMCandleStick) + 7) / 8) == ((sizeof(int64_t) + sizeof(int32_t) + (sizeof(double) * 9) + 7) / 8));
}

void TraderBot::checkForAssumptionLimits() const {
  // number of currencies should be less than 2^12
  assert(g_currency_strs.size() < 4096);

  // number of exchanges should be less than 2^8
  assert(g_exchange_strs.size() < 256);
}

Exchange* TraderBot::getExchange(const exchange_t a_exchange_id) const {
  switch (a_exchange_id) {
    case exchange_t::COINBASE:
      return mp_gdax;
    case exchange_t::GEMINI:
      return mp_gemini;
    default:
      assert(0);
  }

  return NULL;
}

void TraderBot::setController(Controller* ap_tradingCtrl) {
  if (mp_trading_ctrl) {
    CT_CRIT_WARN << "Deleting trading controller\n";
    DELETE(mp_trading_ctrl);
  }
  mp_trading_ctrl = ap_tradingCtrl;
}
