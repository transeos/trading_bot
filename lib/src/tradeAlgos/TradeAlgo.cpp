// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 08/02/18
//
//*****************************************************************
//

#include "tradeAlgos/TradeAlgo.h"
#include "TraderBot.h"
#include "triggers/Trigger.h"
#include "utils/dbUtils.h"
#include <algorithm>

//#define CHECK_EVENT_IN_PARALLEL

using namespace std;

bool dummyFalseReturnFunction() {
  return false;
}

TradeAlgo::TradeAlgo(const tradeAlgo_t a_tradeAlgo_idx) : m_algo_idx(a_tradeAlgo_idx) {
  m_fields = {{"Algo-State(num)", "int"}, {"Algo-State", "ascii"}};

  m_num_event_checks = 0;
  m_num_events = 0;

  m_avg_event_check_time = 0;
  m_avg_event_time = 0;

  m_max_event_check_time = 0;
  m_max_event_time = 0;
}

TradeAlgo::~TradeAlgo() {
  for (auto p_trigger : m_triggers) {
    DELETE(p_trigger);
  }
  m_triggers.clear();

  for (auto exchange_iter : m_indicator_map) {
    for (auto interval_iter : exchange_iter.second) {
      for (auto currency_pair_iter : interval_iter.second) {
        auto* indicator = currency_pair_iter.second;
        DELETE(indicator);
      }
    }
  }
}

void TradeAlgo::checkForEvent(const Time a_cur_time, const bool a_interval_event) {
  // event check starts
  const chrono::high_resolution_clock::time_point start_time = chrono::high_resolution_clock::now();

  int num_triggers = m_triggers.size();

#ifdef CHECK_EVENT_IN_PARALLEL
  vector<future<bool>> event_status;
#else
  vector<bool> event_status;
#endif

  vector<trade_algo_trigger_t> matured_triggers;

  // launching in parallel slows down simulation in case it has small number of triggers
  for (int trigger_idx = 0; trigger_idx < num_triggers; ++trigger_idx) {
    if ((a_interval_event && (m_triggers[trigger_idx]->getTriggerType() != trade_algo_trigger_t::NEW_INTERVAL)) ||
        (!a_interval_event && (m_triggers[trigger_idx]->getTriggerType() == trade_algo_trigger_t::NEW_INTERVAL))) {
#ifdef CHECK_EVENT_IN_PARALLEL
      event_status.push_back(PARALLEL_PROCESS(&dummyFalseReturnFunction));
#else
      event_status.push_back(false);
#endif
      continue;
    }

#ifdef CHECK_EVENT_IN_PARALLEL
    event_status.push_back(
        PARALLEL_PROCESS(&Trigger::checkForEvent, m_triggers[trigger_idx], a_cur_time, m_trade_histories));
#else
    event_status.push_back(m_triggers[trigger_idx]->checkForEvent(a_cur_time, m_trade_histories));
#endif

    const trade_algo_trigger_t trigger_type = m_triggers[trigger_idx]->getTriggerType();
    if (find(matured_triggers.begin(), matured_triggers.end(), trigger_type) == matured_triggers.end())
      matured_triggers.push_back(trigger_type);
  }

  // trade decisions functions to be called
  vector<AlgoFnPtr> foos;

  for (int trigger_idx = 0; trigger_idx < num_triggers; ++trigger_idx) {
#ifdef CHECK_EVENT_IN_PARALLEL
    const bool result = event_status[trigger_idx].get();
#else
    const bool result = event_status[trigger_idx];
#endif

    if (result) {
      for (auto foo : m_triggers[trigger_idx]->getFoos()) {
        // check if foo is already added
        bool already_added = false;

        for (auto already_added_foo : foos) {
          if (already_added_foo == foo) {
            already_added = true;
            break;
          }
        }

        if (!already_added) foos.push_back(foo);
      }
    }
  }

  // end of event checks, start of function call back
  const chrono::high_resolution_clock::time_point start_func_callback_time = chrono::high_resolution_clock::now();

  for (auto foo : foos) (this->*foo)(a_cur_time, matured_triggers);

  // end of functio call back
  const chrono::high_resolution_clock::time_point end_time = chrono::high_resolution_clock::now();

  // populate run time info
  int cur_event_check_time = chrono::duration_cast<chrono::microseconds>(start_func_callback_time - start_time).count();

  if (m_max_event_check_time < Duration(cur_event_check_time)) m_max_event_check_time = cur_event_check_time;

  const Duration total_time_event_check = (m_avg_event_check_time * m_num_event_checks);
  m_avg_event_check_time = ((total_time_event_check + cur_event_check_time) / ++m_num_event_checks);

  if (!foos.size()) return;

  int cur_func_callback_time = chrono::duration_cast<chrono::microseconds>(end_time - start_func_callback_time).count();

  if (m_max_event_time < Duration(cur_func_callback_time)) m_max_event_time = cur_func_callback_time;

  const Duration avg_time_events = (m_avg_event_time * m_num_events);
  m_avg_event_time = ((avg_time_events + cur_func_callback_time) / ++m_num_events);
}

void TradeAlgo::init(const Time a_cur_time, const vector<const TradeHistory*>& ap_past_trade_histories) {
  m_current_time = a_cur_time;

  m_trade_histories = ap_past_trade_histories;

  for (auto p_trade_history : m_trade_histories) {
    const exchange_t exchange_id = p_trade_history->getExchangeId();

    if (m_trading_pairs.find(exchange_id) == m_trading_pairs.end()) m_trading_pairs[exchange_id] = {};

    vector<CurrencyPair>& currencies = m_trading_pairs[exchange_id];
    currencies.push_back(p_trade_history->getCurrencyPair());
  }
}

void TradeAlgo::initTrigger(const vector<AlgoFnPtr>& a_foos, Trigger* ap_trigger, const Time a_cur_time) {
  m_triggers.push_back(ap_trigger);

  ap_trigger->init(a_foos, a_cur_time, m_trade_histories);
}

void TradeAlgo::registerIndicator(DiscreteIndicatorA<>* indicator) {
  const exchange_t exchange_id = indicator->getExchangeId();
  const CurrencyPair& currency_pair = indicator->getCurrencyPair();
  const Duration interval = indicator->getInterval();

  assert(!m_indicator_map[exchange_id][interval][currency_pair]);
  m_indicator_map[exchange_id][interval][currency_pair] = indicator;
}

Duration TradeAlgo::getMinInterval() const {
  Duration min_interval;

  for (auto& interval : m_intervals) {
    if (min_interval == Duration()) {
      min_interval = interval;
      continue;
    }

    min_interval = __gcd(min_interval, interval);
  }

  for (auto trigger : m_triggers) {
    if (trigger->getTriggerType() != trade_algo_trigger_t::NEW_INTERVAL) continue;

    if (min_interval == Duration()) {
      min_interval = trigger->getInterval();
      continue;
    }

    min_interval = __gcd(min_interval, trigger->getInterval());
  }

  return min_interval;
}

void TradeAlgo::saveHeaderToCSV(FILE* ap_file) const {
  for (auto& field : m_fields) fprintf(ap_file, ",%s", field.first.c_str());
}

void TradeAlgo::saveStateToCSV(FILE* ap_file) {
  int i = 0;
  for (auto& field : m_fields) {
    fprintf(ap_file, ",");
    DbUtils::writeInCSV(ap_file, field.second, getMemberPtr(i++));
  }
}

void TradeAlgo::dumpRunTimeStats() const {
  COUT << CMAGENTA << "\n==== Algorithm run time info ====\n";

  COUT << "Number of event checks : " << m_num_event_checks << endl;
  COUT << "Average time taken to check for events : " << m_avg_event_check_time << endl;
  COUT << "Maximum time taken to check for events : " << m_max_event_check_time << endl;

  COUT << "Number of events : " << m_num_events << endl;
  COUT << "Average time taken to handle events : " << m_avg_event_time << endl;
  COUT << "Maximum time taken to handle events : " << m_max_event_time << endl << endl;
}

void TradeAlgo::printStats() const {
  int idx = 0;
  for (auto& field : m_fields) {
    COUT << field.first << " : ";
    DbUtils::printData(field.second, getMemberPtr(idx++));
    COUT << ", ";
  }
  COUT << endl;
}
