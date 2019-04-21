// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 04/03/18.
//

#include "Controller.h"
#include "CMCandleStick.h"
#include "Tick.h"
#include "TraderBot.h"
#include "tradeAlgos/TradeAlgo.h"
#include "exchanges/VirtualExchange.h"
#include <float.h>

#define ORDER_LOG_FILE "OrderLog.txt"

using namespace std;

Controller::Controller(const json& a_json) {
  mp_TradeAlgo = NULL;

  populateFromJson(a_json);

  if ((m_mode != exchange_mode_t::SIMULATION) && (m_start_time < (Time::sNow() - 1_min))) {
    CTRL_TIME_RANGE_ERROR(m_start_time, Time::sNow());
  }

  assert(m_exchanges.size() > 0);

  // initialize prices
  for (auto exchange_iter : m_exchanges)
    m_prices[exchange_iter.second->getExchangeID()] = unordered_map<currency_t, double>{};

  m_starting_portfolio_value = -1;

  m_time_passed_last_tick = 0;

  string filename = CTRL_LOG;
  filename += "/ControllerSummary.csv";
  m_summary_file = fopen(filename.c_str(), "w");

  m_order_log = fopen(ORDER_LOG_FILE, "w");

  adjustStartTime();

  m_controller_time = m_start_time;
}

Controller::~Controller() {
  DELETE(mp_TradeAlgo);

  fclose(m_summary_file);
  fclose(m_order_log);
}

void Controller::populateFromJson(const json& a_json) {
  TraderBot* p_trader_bot = TraderBot::getInstance();

  m_history = Duration(0, 0, a_json["time"]["history_min"].get<int>(), 0);

  auto iter_start_time = a_json["time"].find("start_time");
  if (iter_start_time == a_json["time"].end()) {
    // current time is taken as start time
    m_start_time = Time::sNow();

    Duration duration = Duration(0, 0, a_json["time"]["duration_min"].get<int>(), 0);
    m_end_time = (m_start_time + duration);
  } else {
    m_start_time = iter_start_time->get<string>();
    m_end_time = a_json["time"]["end_time"].get<string>();
  }

  m_mode = static_cast<exchange_mode_t>(a_json["mode"].get<int>());

  tradeAlgo_t trade_algo_id = static_cast<tradeAlgo_t>(a_json["algo"].get<int>());
  createTradeAlgo(trade_algo_id);

  for (auto& exchange_json : a_json["exchanges"].items()) {
    string exchange_str = exchange_json.key();
    Exchange* p_exchange = p_trader_bot->getExchange(Exchange::sStringToExchange(exchange_str));
    m_exchanges[p_exchange->getExchangeID()] = p_exchange;

    const json& currency_json = exchange_json.value()["trading_pairs"];

    vector<CurrencyPair> trading_pairs;
    for (auto& cp_json : currency_json) trading_pairs.push_back(CurrencyPair(cp_json.get<string>()));

    p_exchange->setMode(trading_pairs, m_mode);

    if (m_mode == exchange_mode_t::SIMULATION) {
      ExchangeAccounts exchangeAccount(p_exchange->getExchangeID(), true);

      const json& accounts_json = exchange_json.value()["accounts"];
      for (auto& account_json : accounts_json.items()) {
        string currency = account_json.key();
        double amount = account_json.value().get<double>();

        exchangeAccount.addAccount(Account(Currency::sStringToCurrency(currency), amount));
      }

      p_exchange->castVirtualExchange()->addWallet(&exchangeAccount);
    }

    for (auto currency_pair : trading_pairs) mp_TradeAlgo->updateIndicator(p_exchange->getExchangeID(), currency_pair);
  }

  if (m_mode != exchange_mode_t::REAL) {
    m_sim_vol_fraction = a_json["universe"]["vol_fraction"].get<double>();
    m_sim_max_order_place_delay = Duration(0, 0, 0, 0, a_json["universe"]["max_order_delay_ms"].get<int>());
  }
}

void Controller::init(const json& a_json) {
  if (m_mode != exchange_mode_t::SIMULATION) {
    for (auto exchange_iter : m_exchanges) {
      Exchange* p_exchange = exchange_iter.second;
      p_exchange->updateAccounts(a_json["exchanges"][Exchange::sExchangeToString(p_exchange->getExchangeID())]);
    }
  }

  // create virtual wallet with same value as real wallet
  if (m_mode == exchange_mode_t::BOTH) {
    for (auto exchange_iter : m_exchanges) {
      Exchange* p_exchange = exchange_iter.second;
      p_exchange->castVirtualExchange()->addWallet(p_exchange->getExchangeAccounts());
    }
  }

  // lock trading
  m_algo_event_mutex.lock();

  // load data
  for (auto exchange_iter : m_exchanges) exchange_iter.second->initForTrading(m_history, m_start_time, m_end_time);

  // set exchange time
  for (auto exchange_iter : m_exchanges) {
    Exchange* p_exchange = exchange_iter.second;

    if (m_mode != exchange_mode_t::REAL) p_exchange->castVirtualExchange()->setSimulationTime(m_start_time);

    if (m_mode != exchange_mode_t::SIMULATION) p_exchange->setControllerTime(m_start_time);
  }

  // add indicators defined the trade algo
  addIndicators();

  // warm up trade algo
  mp_TradeAlgo->init(m_start_time, getConstPastTradeHistories());

  initLogs();
}

void Controller::executeOrders(const exchange_t a_exchange_id, const CurrencyPair& currency_pair) {
  if (m_mode != exchange_mode_t::REAL)
    getVirtualExchange(a_exchange_id)->executeOrders(m_sim_vol_fraction, currency_pair);
  else {
    // In exchange, order execution is handled by a different thread
  }
}

void Controller::run() {
  assert(m_start_time == getControllerTime());

  vector<TradeHistory*> market_histories;
  if (m_end_time > Time::sNow()) {
    for (auto exchange_iter : m_exchanges) {
      Exchange* p_exchange = exchange_iter.second;
      const vector<CurrencyPair>& trading_pairs = p_exchange->getTradingPairs();

      for (auto& currency_pair : trading_pairs) market_histories.push_back(p_exchange->getTradeHistory(currency_pair));
    }
  }

  // unlock trading
  m_algo_event_mutex.unlock();

  saveStatsSummaryHeaderInCSV();

  Time last_account_update = getControllerTime();
  Time last_stats_print = last_account_update;

  // run simulation
  while (!checkIfCurrentRunEnded()) {
    // simulation on historical data if end time is less than current time
    if (m_end_time < Time::sNow()) {
      if (!getNextTradeForSimulation()) break;
    }

    const Time cur_time = getControllerTime();

    // update real accounts in every 10 sec
    if ((m_mode != exchange_mode_t::SIMULATION) && ((cur_time - last_account_update) > 10_sec)) {
      for (auto exchange_iter : m_exchanges) {
        Exchange* p_exchange = exchange_iter.second;
        p_exchange->updateAccounts();
      }
      last_account_update = cur_time;
    }

    // print status in every 1 min
    if (((m_end_time > Time::sNow()) || (m_mode != exchange_mode_t::SIMULATION)) &&
        ((cur_time - last_stats_print) > 1_min)) {
      printCurrentStats();
      last_stats_print = cur_time;
    }

    // check for interval event(s)
    m_algo_event_mutex.lock();
    mp_TradeAlgo->checkForEvent(cur_time, true);
    m_algo_event_mutex.unlock();

    m_time_passed_last_tick++;
    if ((m_time_passed_last_tick % 10) == 0) {
      if (g_dump_trades_websocket)
        COUT << CYELLOW << "No new ticks for last " << (m_time_passed_last_tick / 10) << " sec" << endl;
      else
        COUT << CYELLOW << ".";
    } else if (!g_dump_trades_websocket && ((m_time_passed_last_tick % 10) == 0)) {
      COUT << " .";
    }

    if (m_time_passed_last_tick > 3000) {
      // If no new trades happen in 5 min, exit trading
      NO_TICK_ERROR(m_time_passed_last_tick / 600);
    }

    if (m_time_passed_last_tick > 1) {
      this_thread::sleep_for(chrono::milliseconds(100));
      adjustTimeAndCheckForIntervalEvents(Time::sNow());
    }
  }

  // lock trading
  m_algo_event_mutex.lock();

  // remove callback for new trade
  for (auto p_trade_history : market_histories) p_trade_history->setOngoingTrading(false);

  for (auto exchange_iter : m_exchanges) cancelOrders(exchange_iter.second->getExchangeID());

  printCurrentStats();
  mp_TradeAlgo->dumpRunTimeStats();
  dumpFinalStats();
}

Duration Controller::getOrderDelay() const {
  // order place time gets delayed by small amount (usually 0-100 ms)

  if (!g_random) return Duration(0, 0, 0, 0, 50, 0);

  return Duration(0, 0, 0, 0, 0, (rand() % m_sim_max_order_place_delay.getDuration()));
}

const Account& Controller::getAccount(const exchange_t a_exchange_id, const currency_t a_currency) const {
  Exchange* exchange = m_exchanges.at(a_exchange_id);

  // Real mode : exchange account
  // Simulation/both : virtual account
  // Right now, we are using virtual account in both mode.
  return ((m_mode != exchange_mode_t::REAL)
              ? exchange->castVirtualExchange()->getVirtualWallet()->getAccount(a_currency)
              : exchange->getExchangeAccounts()->getAccount(a_currency));
}

order_id_t Controller::placeOrder(const exchange_t a_exchange_id, CurrencyPair currency_pair, double amount,
                                  order_type_t type, order_direction_t direction, double target_price,
                                  const Time time_instant, bool& a_valid_order) {
  order_id_t order_id = "";
  a_valid_order = false;

  Exchange* p_exchange = m_exchanges[a_exchange_id];
  VirtualExchange* p_vir_exchange = p_exchange->castVirtualExchange();

  const double bid_price = getBidPrice(a_exchange_id, currency_pair);
  const double ask_price = getAskPrice(a_exchange_id, currency_pair);

  if (bid_price >= ask_price) {
    CT_CRIT_WARN << "Avoiding placing order because of bid price:" << bid_price << ", ask price:" << ask_price << endl;
    return "INVALID_QUOTE";
  }

  // update virtual accounts
  const currency_t order_currency =
      (direction == order_direction_t::BUY) ? currency_pair.getQuoteCurrency() : currency_pair.getBaseCurrency();

  const Account& acc = getAccount(a_exchange_id, order_currency);

  if (amount == DBL_MAX) {
    // get the whole amount
    amount = acc.getAvailable();

    if (m_mode == exchange_mode_t::REAL) {
      double acc_share = p_exchange->getExchangeAccounts()->getAccountShare(order_currency);
      amount *= acc_share;
    }
  } else if (amount > acc.getAvailable()) {
    CT_CRIT_WARN << "Order placed of " << amount << " amount for " << acc << endl;
    return "INVALID_AMOUNT";
  }

  // check for minimum order limit
  if (amount < p_exchange->getMinOrderAmount(order_currency)) {
    return "NO_BALANCE";
  }

  // taking minimum of virtual and real account
  if (m_mode == exchange_mode_t::BOTH) {
    int account_mismatch_count = 1;
    double real_acc_avail = -1;

    while (1) {
      double real_acc_share = p_exchange->getExchangeAccounts()->getAccountShare(order_currency);
      real_acc_avail = p_exchange->getExchangeAccounts()->getAccount(order_currency).getAvailable();
      real_acc_avail *= real_acc_share;

      double vir_acc_share = getVirtualExchange(a_exchange_id)->getVirtualWallet()->getAccountShare(order_currency);
      double vir_acc_avail =
          getVirtualExchange(a_exchange_id)->getVirtualWallet()->getAccount(order_currency).getAvailable();
      vir_acc_avail *= vir_acc_share;

      double vir_real_acc_ratio = (vir_acc_avail / real_acc_avail);
      if ((vir_real_acc_ratio > 0.9) && (vir_real_acc_ratio < 1.1)) break;

      CT_CRIT_WARN << "Virtual/real account ratio = " << int(vir_real_acc_ratio * 100) << "% " << getControllerTime()
                   << " [" << account_mismatch_count << "]\n";
      p_exchange->getExchangeAccounts()->printDetails();
      p_exchange->castVirtualExchange()->getVirtualWallet()->printDetails();

      // update account balance after 10 msec
      this_thread::sleep_for(chrono::milliseconds(10));
      p_exchange->updateAccounts();

      account_mismatch_count++;
    }

    if (amount > (real_acc_avail)) amount = real_acc_avail;

    // check again for minimum order limit
    if (amount < p_exchange->getMinOrderAmount(order_currency)) {
      return "NO_BALANCE";
    }
  }

  assert(amount <= acc.getAvailable());

  // adjust precision of order amount
  amount = getAdjustedAmount(direction, currency_pair.getQuoteCurrency(), amount);

  // adjust precision of trigger price
  if (target_price > 0) {
    if (direction == order_direction_t::BUY) {
      target_price = floor(target_price * 1e2) / 1e2;

      if (target_price > bid_price) {
        CT_CRIT_WARN << "target price > current bid price in buy order\n";
        return "INVALID";
      }
    } else {
      target_price = ceil(target_price * 1e2) / 1e2;

      if (target_price < ask_price) {
        CT_CRIT_WARN << "target price < current ask price in sell order\n";
        return "INVALID";
      }
    }
  }

  assert(amount <= acc.getAvailable());

  g_order_idx++;

  const double cur_price = ((direction == order_direction_t::BUY) ? ask_price : bid_price);

  if (m_mode != exchange_mode_t::SIMULATION) {
    order_id = p_exchange->placeOrder(currency_pair, amount, type, direction, target_price, time_instant, cur_price);

    if ((order_id == "NO_BALANCE") || (order_id == "")) {
      CT_CRIT_WARN << "Controller is not able place real order, error code: " << order_id << endl;
      return order_id;
    } else {
      fprintf(m_order_log, "%s,%s,%s,placed\n", time_instant.toISOTimeString(false).c_str(),
              Exchange::sExchangeToString(a_exchange_id).c_str(), order_id.c_str());
      fflush(m_order_log);
    }
  }

  if (m_mode != exchange_mode_t::REAL) {
    order_id_t vir_order_id = p_vir_exchange->placeVirtualOrder(currency_pair, amount, type, direction, target_price,
                                                                time_instant, cur_price);
    if (vir_order_id == "NO_BALANCE") {
      CT_CRIT_WARN << "Controller is not able place virtual order, error code: " << vir_order_id << endl;
      return "NO_BALANCE";
    }

    if (order_id == "") order_id = vir_order_id;
  }

  mp_TradeAlgo->populateOrder(order_id);
  a_valid_order = true;
  return order_id;
}

order_id_t Controller::refreshLimitOrder(const exchange_t exchange_id, CurrencyPair currency_pair, double amount,
                                         order_direction_t direction, double target_price, const Time time_instant,
                                         bool& a_valid_order) {
  const order_id_t prev_order_id = mp_TradeAlgo->getLastOrder();
  const Order* prev_order = getOrder(exchange_id, prev_order_id);

  const double adjusted_amount =
      ((amount == DBL_MAX) ? DBL_MAX : getAdjustedAmount(direction, currency_pair.getQuoteCurrency(), amount));

  double price_diff = (prev_order ? (target_price - prev_order->getTriggerPrice()) : target_price);
  if (price_diff < 0) price_diff *= (-1);

  if (prev_order && (prev_order->getAmount() > 0.001)) {
    if ((price_diff < 0.01) && (exchange_id == prev_order->getExchangeId()) &&
        (currency_pair == prev_order->getCurrencyPair()) && (prev_order->getOrderType() == order_type_t::LIMIT) &&
        (direction == prev_order->getOrderDir()) &&
        ((amount == DBL_MAX) || (adjusted_amount == prev_order->getAmount()))) {
      a_valid_order = false;
      return "DUPLICATE_ORDER";
    }
  }

  // cancel existing orders
  // COUT << CCYAN << "Cancelling all pending limit orders at " << Time::sNow() << endl;
  cancelLimitOrders(exchange_id, currency_pair, true);
  cancelLimitOrders(exchange_id, currency_pair, false);

  order_id_t order_id = placeOrder(exchange_id, currency_pair, amount, order_type_t::LIMIT, direction, target_price,
                                   time_instant, a_valid_order);
  return order_id;
}

void Controller::cancelLimitOrders(const exchange_t a_exchange_id, const CurrencyPair& currency_pair, const bool buy) {
  Exchange* p_exchange = m_exchanges[a_exchange_id];
  VirtualExchange* p_vir_exchange = p_exchange->castVirtualExchange();

  if (m_mode != exchange_mode_t::REAL) {
    if (buy)
      p_vir_exchange->cancelLimitVirtualBuyOrders(currency_pair);
    else
      p_vir_exchange->cancelLimitVirtualSellOrders(currency_pair);
  }

  if (m_mode != exchange_mode_t::SIMULATION) p_exchange->cancelLimitOrders(currency_pair, buy);
}

const Order* Controller::getOrder(const exchange_t a_exchange_id, const order_id_t a_order_id) const {
  return ((m_mode != exchange_mode_t::SIMULATION) ? m_exchanges.at(a_exchange_id)->getOrder(a_order_id)
                                                  : getVirtualExchange(a_exchange_id)->getVirtualOrder(a_order_id));
}

const Order* Controller::lastCancelledOrder(const exchange_t a_exchange_id) const {
  return ((m_mode != exchange_mode_t::SIMULATION) ? m_exchanges.at(a_exchange_id)->lastCancelledOrder()
                                                  : getVirtualExchange(a_exchange_id)->lastCancelledVirtualOrder());
}

void Controller::cancelOrders(const exchange_t a_exchange_id) {
  Exchange* p_exchange = m_exchanges[a_exchange_id];
  VirtualExchange* p_vir_exchange = p_exchange->castVirtualExchange();

  const vector<CurrencyPair>& trading_pairs = p_exchange->getTradingPairs();

  for (auto currency_pair : trading_pairs) {
    if (m_mode != exchange_mode_t::REAL) p_vir_exchange->cancelVirtualOrders(currency_pair);

    if (m_mode != exchange_mode_t::SIMULATION) p_exchange->cancelPendingOrders(currency_pair);
  }
}

bool Controller::getNextTradeForSimulation() {
  if (m_mode != exchange_mode_t::SIMULATION) return false;

  assert(m_end_time < (Time::sNow()));

  exchange_t exchange_id = exchange_t::NOEXCHANGE;

  Tick new_tick;
  CurrencyPair new_tick_currency_pair;

  for (auto exchange_iter : m_exchanges) {
    Exchange* p_exchange = exchange_iter.second;
    VirtualExchange* p_vir_exchange = p_exchange->castVirtualExchange();

    const vector<CurrencyPair>& trading_pairs = p_exchange->getTradingPairs();

    for (auto& currency_pair : trading_pairs) {
      // get full trades and delayed trades
      const TradeHistory* p_full_trade_history = p_vir_exchange->getFullTradeHistory(currency_pair);
      const TradeHistory* p_past_trade_history = p_vir_exchange->getCurrentTradeHistory(currency_pair);

      const TickPeriod* p_full_trade_period = p_full_trade_history->getTickPeriod();
      const TickPeriod* p_past_trade_period = p_past_trade_history->getTickPeriod();

      if (p_full_trade_period->size() > p_past_trade_period->size()) {
        const Tick& next_tick = *(p_full_trade_period->begin() + p_past_trade_period->size());
        if (((new_tick == Tick()) || (next_tick < new_tick)) && (next_tick.getTimeStamp() < m_end_time)) {
          new_tick = next_tick;
          new_tick_currency_pair = currency_pair;
          exchange_id = p_exchange->getExchangeID();
        }
      }
    }
  }

  // step forward
  if (exchange_id != exchange_t::NOEXCHANGE) {
    adjustTimeAndCheckForIntervalEvents(new_tick.getTimeStamp());

    VirtualExchange* p_vir_exchange = getVirtualExchange(exchange_id);

    // get full trades and delayed trades
    const TradeHistory* p_full_trade_history = p_vir_exchange->getFullTradeHistory(new_tick_currency_pair);
    TradeHistory* p_past_trade_history = p_vir_exchange->getCurrentTradeHistory(new_tick_currency_pair);

    updateTick(p_full_trade_history, p_past_trade_history);

    return true;
  }

  return false;
}

void Controller::doRealTimeTrading(const TradeHistory* ap_trade_history) {
  m_algo_event_mutex.lock();

  Exchange* p_exchange = m_exchanges[ap_trade_history->getExchangeId()];
  VirtualExchange* p_vir_exchange = p_exchange->castVirtualExchange();

  const CurrencyPair& currency_pair = ap_trade_history->getCurrencyPair();

  TradeHistory* p_past_trade_history = NULL;

  // get full trades and past (or delayed) trades
  p_past_trade_history = (m_mode != exchange_mode_t::REAL) ? p_vir_exchange->getCurrentTradeHistory(currency_pair)
                                                           : p_exchange->getDelayedTradeHistory(currency_pair);

  updateTick(ap_trade_history, p_past_trade_history);

  m_algo_event_mutex.unlock();
}

void Controller::updateTick(const TradeHistory* ap_full_trade_history, TradeHistory* ap_delayed_trade_history) {
  const exchange_t exchange_id = ap_full_trade_history->getExchangeId();
  const CurrencyPair& currency_pair = ap_full_trade_history->getCurrencyPair();

  assert((exchange_id == ap_delayed_trade_history->getExchangeId()) ||
         (currency_pair == ap_delayed_trade_history->getCurrencyPair()));

  const TickPeriod* p_full_trade_period = ap_full_trade_history->getTickPeriod();
  const TickPeriod* p_delayed_trade_period = ap_delayed_trade_history->getTickPeriod();

  assert(p_delayed_trade_period->size() != p_full_trade_period->size());

  Tick new_tick = *(p_full_trade_period->begin() + p_delayed_trade_period->size());
  assert(new_tick != Tick());

  ap_delayed_trade_history->appendTrade(new_tick);

  Exchange* p_exchange = m_exchanges[exchange_id];
  VirtualExchange* p_vir_exchange = p_exchange->castVirtualExchange();

  if (m_mode != exchange_mode_t::REAL) p_vir_exchange->setSimulationTime(p_delayed_trade_period->back().getTimeStamp());

  if (m_mode != exchange_mode_t::SIMULATION)
    p_exchange->setControllerTime(p_delayed_trade_period->back().getTimeStamp());

  // update global price
  if (currency_pair.getQuoteCurrency() == currency_t::USD)
    m_prices[exchange_id][currency_pair.getBaseCurrency()] = new_tick.getPrice();

  const double cur_total = getPortfolioValue();
  if ((m_starting_portfolio_value < 0) && (cur_total > 0)) {
    // set init portfolio value
    m_starting_portfolio_value = cur_total;
    m_initial_prices = m_prices;
  }

  // reset no tick counter
  m_time_passed_last_tick = 0;

  if (new_tick.getTimeStamp() < m_end_time) handleTickEvent(exchange_id, currency_pair);
}

void Controller::handleTickEvent(const exchange_t a_exchange_id, const CurrencyPair& currency_pair) {
  // if (cur_tick.getPrice() < 1)
  //	continue;

  executeOrders(a_exchange_id, currency_pair);

  mp_TradeAlgo->checkForEvent(getControllerTime(a_exchange_id));
}

vector<TradeHistory*> Controller::getPastTradeHistories() const {
  vector<TradeHistory*> trade_histories;

  for (auto exchange_iter : m_exchanges) {
    Exchange* p_exchange = exchange_iter.second;
    const vector<CurrencyPair>& trading_pairs = p_exchange->getTradingPairs();

    for (auto& currency_pair : trading_pairs) {
      if (m_mode != exchange_mode_t::REAL)
        trade_histories.push_back(p_exchange->castVirtualExchange()->getCurrentTradeHistory(currency_pair));
      else
        trade_histories.push_back(p_exchange->getDelayedTradeHistory(currency_pair));
    }
  }

  return trade_histories;
}

vector<const TradeHistory*> Controller::getConstPastTradeHistories() const {
  vector<const TradeHistory*> const_trade_histories;

  vector<TradeHistory*> trade_histories = getPastTradeHistories();
  for (auto p_trade_history : trade_histories) const_trade_histories.push_back(p_trade_history);

  return const_trade_histories;
}

void Controller::addIndicators() {
  for (auto exchange_iter : m_exchanges) {
    Exchange* p_exchange = exchange_iter.second;
    const vector<CurrencyPair>& trading_pairs = p_exchange->getTradingPairs();

    for (auto& currency_pair : trading_pairs) {
      TradeHistory* p_past_trade_history =
          ((m_mode != exchange_mode_t::REAL) ? p_exchange->castVirtualExchange()->getCurrentTradeHistory(currency_pair)
                                             : p_exchange->getDelayedTradeHistory(currency_pair));

      p_past_trade_history->addCandlePeriod(mp_TradeAlgo->getCandleStickDuration());

      for (auto interval : mp_TradeAlgo->getCandleStickDuration()) {
        DiscreteIndicatorA<>* indicator =
            mp_TradeAlgo->getIndicators(p_exchange->getExchangeID(), interval, currency_pair);
        if (indicator) p_past_trade_history->addIndicator(interval, indicator);
      }
    }
  }
}

void Controller::dumpFinalStats() const {
  vector<TradeHistory*> trade_histories = getPastTradeHistories();

  if (m_mode == exchange_mode_t::SIMULATION)
    COUT << CMAGENTA << endl << "Simulation complete : ";
  else
    COUT << CMAGENTA << endl << "Trading complete : ";

  COUT << CBLUE << m_start_time << CMAGENTA << " {";

  for (auto p_trade_history : trade_histories) {
    const Tick& first_tick = p_trade_history->getTickPeriod()->front();
    COUT << CGREEN << p_trade_history->getCurrencyPair() << ":" << first_tick.getUniqueID() << ", ";
  }

  COUT << CMAGENTA << "}" << endl << " -> ";
  COUT << CBLUE << m_end_time << " (" << getControllerTime() << ")" << CMAGENTA << " {";

  for (auto p_trade_history : trade_histories) {
    const Tick& last_tick = p_trade_history->getTickPeriod()->back();
    COUT << CGREEN << p_trade_history->getCurrencyPair() << ":" << last_tick.getUniqueID() << ", ";
  }

  COUT << CMAGENTA << "} : ";
  COUT << CYELLOW << m_starting_portfolio_value << " -> " << getPortfolioValue();

  if (m_mode == exchange_mode_t::BOTH) COUT << CMAGENTA << " (Virtual: " << getVirtualPortfolioValue() << ")";

  COUT << endl << endl;

#ifdef DEBUG
  for (auto p_trade_history : trade_histories) {
    // dumping tick data
    p_trade_history->saveTickDataToCSV(CTRL_LOG);
    p_trade_history->saveCandlestickDataToCSV(CTRL_LOG);
  }
#endif
}

bool Controller::checkIfCurrentRunEnded() const {
  for (auto exchange_iter : m_exchanges) {
    Exchange* p_exchange = exchange_iter.second;

    if (m_mode != exchange_mode_t::REAL) {
      if (p_exchange->castVirtualExchange()->getSimulationTime() < m_end_time) return false;
    } else {
      if (p_exchange->getControllerTime() < m_end_time) return false;
    }
  }

  return true;
}

double Controller::getPortfolioValue() const {
  double portfolio_val = 0;

  for (auto exchange_iter : m_exchanges) {
    Exchange* p_exchange = exchange_iter.second;

    double cur_val =
        ((m_mode == exchange_mode_t::SIMULATION) ? p_exchange->castVirtualExchange()->getVirtualWallet()->getUSDValue()
                                                 : p_exchange->getExchangeAccounts()->getUSDValue());

    if (cur_val < 0) {
      // all currency prices are not yet initialized
      return -1;
    }

    portfolio_val += cur_val;
  }

  return portfolio_val;
}

double Controller::getPortfolioValue(const exchange_t a_exchange_id) const {
  for (auto exchange_iter : m_exchanges) {
    Exchange* p_exchange = exchange_iter.second;

    if (p_exchange->getExchangeID() == a_exchange_id) {
      double portfolio_val = ((m_mode == exchange_mode_t::SIMULATION)
                                  ? p_exchange->castVirtualExchange()->getVirtualWallet()->getUSDValue()
                                  : p_exchange->getExchangeAccounts()->getUSDValue());

      return portfolio_val;
    }
  }
  return -1;
}

double Controller::getVirtualPortfolioValue() const {
  if (m_mode == exchange_mode_t::REAL) return -1;

  double portfolio_val = 0;

  for (auto exchange_iter : m_exchanges) {
    double cur_val = exchange_iter.second->castVirtualExchange()->getVirtualWallet()->getUSDValue();

    if (cur_val < 0) {
      // all currency prices are not yet initialized
      return -1;
    }

    portfolio_val += cur_val;
  }

  return portfolio_val;
}

double Controller::getCurrentPrice(const exchange_t a_exchange_id, const currency_t a_currency) const {
  if (m_prices.find(a_exchange_id) == m_prices.end()) return -1;

  const auto& currencies = m_prices.at(a_exchange_id);

  if (currencies.find(a_currency) == currencies.end()) return -1;

  return currencies.at(a_currency);
}

Time Controller::getControllerTime(const exchange_t a_exchange_id) const {
  return ((m_mode != exchange_mode_t::REAL) ? m_exchanges.at(a_exchange_id)->castVirtualExchange()->getSimulationTime()
                                            : m_exchanges.at(a_exchange_id)->getControllerTime());
}

Time Controller::getControllerTime() const {
  for (auto exchange_iter : m_exchanges) {
    Time exchange_time = getControllerTime(exchange_iter.second->getExchangeID());
    if (m_controller_time < exchange_time) m_controller_time = exchange_time;
  }

  return m_controller_time;
}

void Controller::initLogs() const {
  vector<TradeHistory*> trade_histories = getPastTradeHistories();
  for (auto p_trade_history : trade_histories) p_trade_history->initLogs(CTRL_LOG);
}

void Controller::saveHeaderInCSV(FILE* ap_file, const exchange_t a_exchange_id) const {
  saveControllerHeaderToCSV(ap_file);

  mp_TradeAlgo->saveHeaderToCSV(ap_file);

  Exchange* p_exchange = m_exchanges.at(a_exchange_id);

  if (m_mode == exchange_mode_t::SIMULATION)
    p_exchange->castVirtualExchange()->getVirtualWallet()->saveHeaderInCSV(ap_file);
  else
    p_exchange->getExchangeAccounts()->saveHeaderInCSV(ap_file);
}

void Controller::saveStatsInCSV(FILE* ap_file, const exchange_t a_exchange_id) {
  saveControllerDataToCSV(ap_file);

  mp_TradeAlgo->saveStateToCSV(ap_file);

  Exchange* p_exchange = m_exchanges.at(a_exchange_id);

  if (m_mode == exchange_mode_t::SIMULATION)
    p_exchange->castVirtualExchange()->getVirtualWallet()->saveInCSV(ap_file);
  else
    p_exchange->getExchangeAccounts()->saveInCSV(ap_file);
}

Quote Controller::getQuote(const exchange_t a_exchange_id, const CurrencyPair& a_currency_pair) const {
  if ((m_end_time > Time::sNow()) || (m_mode != exchange_mode_t::SIMULATION)) {
    return m_exchanges.at(a_exchange_id)->getQuote(a_currency_pair);
  }

  return getVirtualExchange(a_exchange_id)->getVirtualQuote(a_currency_pair);
}

double Controller::getBidPrice(const exchange_t a_exchange_id, const CurrencyPair& a_currency_pair) const {
  if ((m_end_time > Time::sNow()) || (m_mode != exchange_mode_t::SIMULATION)) {
    return m_exchanges.at(a_exchange_id)->getBidPrice(a_currency_pair);
  }

  return getVirtualExchange(a_exchange_id)->getVirtualQuote(a_currency_pair).getBidPrice();
}

double Controller::getAskPrice(const exchange_t a_exchange_id, const CurrencyPair& a_currency_pair) const {
  if ((m_end_time > Time::sNow()) || (m_mode != exchange_mode_t::SIMULATION)) {
    return m_exchanges.at(a_exchange_id)->getAskPrice(a_currency_pair);
  }

  return getVirtualExchange(a_exchange_id)->getVirtualQuote(a_currency_pair).getAskPrice();
}

void Controller::saveStatsSummaryHeaderInCSV() const {
  saveControllerHeaderToCSV(m_summary_file);

  mp_TradeAlgo->saveHeaderToCSV(m_summary_file);

  for (auto exchange_iter : m_exchanges) {
    Exchange* p_exchange = exchange_iter.second;

    if (m_mode != exchange_mode_t::SIMULATION) p_exchange->getExchangeAccounts()->saveHeaderInCSV(m_summary_file);

    if (m_mode != exchange_mode_t::REAL)
      p_exchange->castVirtualExchange()->getVirtualWallet()->saveHeaderInCSV(m_summary_file);
  }

  fprintf(m_summary_file, "\n");
}

void Controller::saveStatsSummaryInCSV() {
  saveControllerDataToCSV(m_summary_file);

  mp_TradeAlgo->saveStateToCSV(m_summary_file);

  for (auto exchange_iter : m_exchanges) {
    Exchange* p_exchange = exchange_iter.second;

    if (m_mode != exchange_mode_t::SIMULATION) p_exchange->getExchangeAccounts()->saveInCSV(m_summary_file);

    if (m_mode != exchange_mode_t::REAL)
      p_exchange->castVirtualExchange()->getVirtualWallet()->saveInCSV(m_summary_file);
  }

  fprintf(m_summary_file, "\n");
}

void Controller::printCurrentStats() const {
  COUT << "\nController_time : " << CBLUE << getControllerTime() << endl;

  const double portfolio_ratio = (getPortfolioValue() / m_starting_portfolio_value);

  for (auto exchange_iter : m_exchanges) {
    const exchange_t exchange_id = exchange_iter.first;
    Exchange* p_exchange = exchange_iter.second;

    set<currency_t> currencies;

    const vector<CurrencyPair>& trading_pairs = p_exchange->getTradingPairs();
    for (auto& currency_pair : trading_pairs) {
      currencies.insert(currency_pair.getBaseCurrency());

      if (currency_pair.getQuoteCurrency() != currency_t::USD) currencies.insert(currency_pair.getQuoteCurrency());
    }

    for (auto currency : currencies) {
      double benchmark = 1;

      if (m_initial_prices.find(exchange_id) != m_initial_prices.end()) {
        auto currency_iter = m_initial_prices.at(exchange_id);

        if (currency_iter.find(currency) != currency_iter.end()) {
          const double price_ratio = (m_prices.at(exchange_id).at(currency) / currency_iter.at(currency));

          if (portfolio_ratio > 0) {
            benchmark = (portfolio_ratio / price_ratio);
          }
        }
      }
      COUT << "benchmark(" << Exchange::sExchangeToString(exchange_id) << "-" << Currency::sCurrencyToString(currency)
           << ") : " << CRED << benchmark << endl;
    }

    if (m_mode == exchange_mode_t::SIMULATION)
      p_exchange->castVirtualExchange()->getVirtualWallet()->printDetails();
    else
      p_exchange->getExchangeAccounts()->printDetails();
  }

  mp_TradeAlgo->printStats();
}

// start time aligned to minimum algo interval
void Controller::adjustStartTime() {
  Duration algo_min_interval = mp_TradeAlgo->getMinInterval();
  Duration duration_adjustment = (m_start_time % algo_min_interval);
  if (duration_adjustment == Duration()) return;
  duration_adjustment = (algo_min_interval - duration_adjustment);

  m_history = (m_history + duration_adjustment);
  m_start_time += duration_adjustment;
  m_end_time += duration_adjustment;

  // wait for adjusted time
  if (m_end_time > Time::sNow()) this_thread::sleep_for(chrono::seconds(duration_adjustment.getDurationInSec() + 1));
}

void Controller::adjustTimeAndCheckForIntervalEvents(const Time& a_tick_time) {
  m_algo_event_mutex.lock();

  Duration algo_min_interval = mp_TradeAlgo->getMinInterval();
  Duration tick_time_adjustment = (a_tick_time % algo_min_interval);
  Duration controller_time_adjustment = (getControllerTime() % algo_min_interval);

  if ((a_tick_time - tick_time_adjustment) >= (m_controller_time - controller_time_adjustment + algo_min_interval)) {
    m_controller_time += (algo_min_interval - controller_time_adjustment);
    mp_TradeAlgo->checkForEvent(m_controller_time, true);
  }

  while ((a_tick_time - tick_time_adjustment) >= (m_controller_time + algo_min_interval)) {
    m_controller_time += algo_min_interval;
    mp_TradeAlgo->checkForEvent(m_controller_time, true);
  }

  m_algo_event_mutex.unlock();
}

void Controller::saveControllerHeaderToCSV(FILE* a_file) const {
  fprintf(a_file, "Controller_time");

  for (auto exchange_iter : m_exchanges) {
    Exchange* p_exchange = exchange_iter.second;

    set<currency_t> currencies;

    const vector<CurrencyPair>& trading_pairs = p_exchange->getTradingPairs();
    for (auto& currency_pair : trading_pairs) {
      currencies.insert(currency_pair.getBaseCurrency());

      if (currency_pair.getQuoteCurrency() != currency_t::USD) currencies.insert(currency_pair.getQuoteCurrency());
    }

    for (auto currency : currencies) {
      fprintf(a_file, ",benchmark(%s-%s)", Exchange::sExchangeToString(exchange_iter.first).c_str(),
              Currency::sCurrencyToString(currency).c_str());
    }
  }
}

void Controller::saveControllerDataToCSV(FILE* a_file) const {
  fprintf(a_file, "%s", getControllerTime().toISOTimeString(false).c_str());

  const double portfolio_ratio = (getPortfolioValue() / m_starting_portfolio_value);

  for (auto exchange_iter : m_exchanges) {
    const exchange_t exchange_id = exchange_iter.first;
    Exchange* p_exchange = exchange_iter.second;

    set<currency_t> currencies;

    const vector<CurrencyPair>& trading_pairs = p_exchange->getTradingPairs();
    for (auto& currency_pair : trading_pairs) {
      currencies.insert(currency_pair.getBaseCurrency());

      if (currency_pair.getQuoteCurrency() != currency_t::USD) currencies.insert(currency_pair.getQuoteCurrency());
    }

    for (auto currency : currencies) {
      if (m_initial_prices.find(exchange_id) != m_initial_prices.end()) {
        auto currency_iter = m_initial_prices.at(exchange_id);

        if (currency_iter.find(currency) != currency_iter.end()) {
          const double price_ratio = (m_prices.at(exchange_id).at(currency) / currency_iter.at(currency));

          if (portfolio_ratio > 0) {
            fprintf(a_file, ",%lf", (portfolio_ratio / price_ratio));
            continue;
          }
        }
      }
      fprintf(a_file, ",1.0");
    }
  }
}

void Controller::LockTrading() {
  // lock trading
  m_algo_event_mutex.lock();
}

void Controller::UnlockTrading() {
  // unlock trading
  m_algo_event_mutex.unlock();
}

void Controller::logOrderCancellation(const exchange_t a_exchange_id, const order_id_t& order_id) const {
  fprintf(m_order_log, "%s,%s,%s,cancelled\n", getControllerTime().toISOTimeString(false).c_str(),
          Exchange::sExchangeToString(a_exchange_id).c_str(), order_id.c_str());
  fflush(m_order_log);
}

void Controller::cancelPendingOrdersFromPreviousRun() {
  if (!TradeUtils::isValidPath(ORDER_LOG_FILE)) {
    return;
  }

  ifstream file(ORDER_LOG_FILE);
  string line;

  unordered_map<exchange_t, unordered_set<order_id_t>> pending_orders;

  while (getline(file, line)) {
    istringstream iss(line);

    vector<string> data;
    int num_elements = TradeUtils::parseCSVLine(data, line);
    assert(num_elements == 4);

    const exchange_t exchange_id = Exchange::sStringToExchange(data[1]);
    const order_id_t order_id = data[2];

    if (data[3] == "placed") {
      if (pending_orders.find(exchange_id) == pending_orders.end())
        pending_orders[exchange_id] = {order_id};
      else {
        unordered_set<order_id_t>& orders = pending_orders[exchange_id];
        assert(orders.find(order_id) == orders.end());
        orders.insert(order_id);
      }
    } else if (data[3] == "cancelled") {
      unordered_set<order_id_t>& orders = pending_orders[exchange_id];
      assert(orders.find(order_id) != orders.end());
      orders.erase(order_id);
    } else
      assert(0);
  }

  const TraderBot* trader_bot = TraderBot::getInstance();

  for (auto order_iter : pending_orders) {
    const exchange_t exchange_id = order_iter.first;
    unordered_set<order_id_t>& orders = order_iter.second;

    for (auto order : orders) {
      trader_bot->getExchange(exchange_id)->cancelOrder(order, false);
    }
  }
}

double Controller::getAdjustedAmount(order_direction_t direction, const currency_t quote_currency,
                                     const double orig_amount) const {
  // adjust precision of order amount
  const double amount = (((direction == order_direction_t::BUY) && (quote_currency == currency_t::USD))
                             ? (floor(orig_amount * 1e2) / 1e2)
                             : (floor(orig_amount * 1e5) / 1e5));

  if (amount > orig_amount) {
    // due to C++ precision
    return getAdjustedAmount(direction, quote_currency, (orig_amount * .999));
  }

  assert(amount > 0);
  return amount;
}
