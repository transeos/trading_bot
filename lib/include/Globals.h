// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 12/9/17.
//

#include <cassandra.h>
#include <iostream>
#include <mutex>

class CassServer;

// ASSERT: debug assert
#ifdef DEBUG
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr) \
  {}
#endif

// delete if not NULL and set NULL if deleted
#define DELETE(x) \
  {               \
    if (x) {      \
      delete x;   \
      x = NULL;   \
    }             \
  }

// execute 'x' statement indefinitely till the execution is successfull
#define CONTINUOUS_TRY(expr, sleep_ms)                                                    \
  for (;;) {                                                                              \
    try {                                                                                 \
      expr;                                                                               \
      break;                                                                              \
    } catch (exception & e) {                                                             \
      CT_WARN << __func__ << " Exception: " << e.what() << " [continuous try]\n";         \
      if (sleep_ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms)); \
    }                                                                                     \
  }

#define CHECK_WITH_PRECISION(value1, value2, precision) CHECK(std::abs((value1) - (value2)) < (precision))

// submit jobs in thread pool
#define PARALLEL_PROCESS(...) TraderBot::getInstance()->getThreadPool().submit(std::bind(__VA_ARGS__))

//#define DISABLE_THREAD

// global variables
#ifdef DEFINE_GLOBALS
#undef GLOBAL
#undef GLOBAL_NOINIT
#define GLOBAL(type_var, initial_value) type_var = initial_value
#define GLOBAL_NOINIT(type_var) type_var
#else
#undef GLOBAL
#undef GLOBAL_NOINIT
#define GLOBAL(type_var, initial_value) extern type_var
#define GLOBAL_NOINIT(type_var) extern type_var
#endif

GLOBAL(bool g_dump_rest_api_responses, false);
GLOBAL(bool g_dump_order_responses, false);
GLOBAL(bool g_dump_trades_websocket, false);

GLOBAL(bool g_update_cass, false);
GLOBAL(CassSession* g_cass_session, NULL);
GLOBAL(CassServer* g_cass_server, NULL);
GLOBAL(std::string g_cassandra_ip, "127.0.0.1");

GLOBAL(bool g_random, true);
GLOBAL(bool g_exiting, false);
GLOBAL(std::string g_trader_home, "..");
GLOBAL(int g_order_idx, 0);

GLOBAL_NOINIT(std::mutex g_critcal_task);

// After adding a global variable here, it should be include in 'TraderBot::resetGlobals()' if required.
