// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 24/10/17.
//

#include "exchanges/Exchange.h"
#include "Controller.h"
#include "Order.h"
#include "Tick.h"
#include "TraderBot.h"
#include "utils/ErrorHandling.h"
#include "utils/RestAPI2JSON.h"

using namespace std;

Exchange::Exchange(const exchange_t a_id, const json& a_config) : m_id(a_id), m_config(a_config) {
  m_accounts = NULL;
  m_controller_time = Time(0);

  m_websocket_connected = false;

  m_apikey = APIKey(m_config["api-credential"]);
  m_rest_api_endpoint = m_config["rest_api_endpoint"].get<string>();
  m_websocket_endpoint = m_config["ws_api_endpoint"].get<string>();

  populateMinOrderAmount();

  m_fee.maker_fee = m_config["fees"]["maker"].get<double>();
  m_fee.taker_fee = m_config["fees"]["taker"].get<double>();

  m_consecutive = m_config["consecutive_trades"].get<bool>();

  poupulateResidualAmountsData();
}

Exchange::~Exchange() {
  DELETE(m_query_handle);

  for (auto& ticks_iter : m_ticks_buffer) DELETE(ticks_iter.second);
  m_ticks_buffer.clear();

  DELETE(m_websocket_handle);

  if (m_mode != exchange_mode_t::REAL) {
    for (auto currency_pair : m_trading_pairs) m_markets.erase(currency_pair);
  } else {
    for (auto market : m_markets) {
      DELETE(market.second);
    }

    for (auto iter : m_histories_till_ctrl_time) {
      DELETE(iter.second);
    }
  }

  m_markets.clear();
  m_histories_till_ctrl_time.clear();

  for (auto iter : m_pending_orders) clearOrder(iter.first);

  DELETE(m_accounts);
}

string Exchange::sExchangeToString(const exchange_t exchange_id, bool lower) {
  const int offset = static_cast<int>(exchange_t::NOEXCHANGE);

  const int exchange_id_val = static_cast<const int>(exchange_id);
  assert((exchange_id_val >= offset) && ((exchange_id_val - offset) < static_cast<int64_t>(g_exchange_strs.size())));

  string c_str = g_exchange_strs[exchange_id_val - offset];

  if (lower) transform(c_str.begin(), c_str.end(), c_str.begin(), ::tolower);

  return c_str;
}

exchange_t Exchange::sStringToExchange(string a_exchange) {
  const int offset = static_cast<int>(exchange_t::NOEXCHANGE);

  exchange_t retEx = exchange_t::NOEXCHANGE;
  transform(a_exchange.begin(), a_exchange.end(), a_exchange.begin(), ::toupper);

  for (int idx = (g_exchange_strs.size() - 1); idx > -1; --idx) {
    if (g_exchange_strs[idx] == a_exchange) {
      retEx = static_cast<exchange_t>(idx + offset);
      break;
    }
  }

  return retEx;
}

bool Exchange::saveToCSV(const string& a_exported_data_name, const Time a_start_time, const Time a_end_time,
                         const string& a_database_dir) {
  for (auto market_iter : m_markets) {
    Market* p_market = market_iter.second;
    TradeHistory* p_trade_history = p_market->getTradeHistory();
    if (!p_trade_history) continue;

    if (p_trade_history->getFullSymbolStr() == a_exported_data_name) {
      p_trade_history->saveToCSV(a_database_dir, a_start_time, a_end_time);
      return true;
    }
  }
  return false;
}

void Exchange::restoreFromCSV(const string& a_csv_file_dir) {
  for (auto market_iter : m_markets) {
    Market* p_market = market_iter.second;
    TradeHistory* p_trade_history = p_market->getTradeHistory();
    if (!p_trade_history) continue;

    p_trade_history->restoreFromCSV(a_csv_file_dir);
  }
}

size_t Exchange::getCurrencyPairs(vector<CurrencyPair>& currency_pairs) const {
  currency_pairs.clear();
  for (auto& market : m_markets) {
    currency_pairs.push_back(market.first);
  }

  return currency_pairs.size();
}

double Exchange::getUsableBalance(const currency_t currency) const {
  double balance = 0.0;

  try {
    balance = m_accounts->getAccount(currency).getAvailable();
  } catch (const out_of_range& oor) {
    cerr << "Out of Range error: " << oor.what() << '\n';
  }

  return balance;
}

double Exchange::getTotalBalance(const currency_t currency) const {
  double balance = 0.0;

  try {
    balance = m_accounts->getAccount(currency).getBalance();
  } catch (const out_of_range& oor) {
    cerr << "Out of Range error: " << oor.what() << '\n';
  }

  return balance;
}

order_id_t Exchange::placeOrder(CurrencyPair currency_pair, double amount, order_type_t type,
                                order_direction_t direction, double target_price, const Time time_instant,
                                const double current_price) {
  // update accounts
  const currency_t order_currency =
      (direction == order_direction_t::BUY) ? currency_pair.getQuoteCurrency() : currency_pair.getBaseCurrency();
  Account& acc = m_accounts->getAccount(order_currency);

  assert((amount <= acc.getAvailable()) && (amount > 0));

  const Time order_place_time = Time::sNow();
  Order* p_order = new Order(g_order_idx, target_price, type, direction, m_id, currency_pair, amount, order_place_time,
                             current_price);

  // place order
  order_id_t order_id = order(*p_order);

  if (p_order->getAmount() == 0)
    completeOrder(p_order);
  else
    dumpAccountStatus(p_order);

  return order_id;
}

void Exchange::dumpAccountStatus(const Order* ap_order) {
  const TickPeriod* p_trade_period = getTrades(ap_order->getCurrencyPair());

  COUT << CBLUE << Time::sNow() << " (" << p_trade_period->back().getUniqueID() << "): ";

  COUT << CCYAN << *m_accounts;

  ap_order->printLastStatus();

  COUT << endl;
}

void Exchange::printTradeFillStatus(const TickPeriod& a_trades, const CurrencyPair& a_currency_pair) const {
  if (a_trades.empty()) return;

  COUT << CRED << sExchangeToString(m_id) + ": " << CGREEN << " Saving " << CYELLOW << a_currency_pair.toString()
       << CRESET << " (" << a_trades.size() << ")"
       << " data from " << CRESET << a_trades.front() << endl
       << CGREEN << " to " << CRESET << a_trades.back() << "." << endl;
}

void Exchange::printPendingTradeFillStatus(const TickPeriod& a_trades, const CurrencyPair& a_currency_pair,
                                           int a_num_trades_added) const {
  // Dump details if number of pending trades >= 10000
  if (a_trades.size() < 10000) return;

  if (a_num_trades_added % 1000) return;

  COUT << "Adding " << a_num_trades_added << " trades for " << a_currency_pair.toString() << " pair.\n"
       << "Last : " << a_trades.front() << endl;
}

void Exchange::initForTrading(const Duration history_duration, const Time start_time, const Time end_time,
                              const bool called_from_controller) {
  // Start time can't be greater than end time
  // Start time can't be in future for now
  assert((start_time <= end_time) && (start_time <= Time::sNow()));

  TradeHistory* th_full = NULL;
  TradeHistory* th_past = NULL;

  Time present_time = Time::sNow();

  if ((present_time - start_time) < 1_min)  // within 1 minute consider realtime
  {
    for (auto trading_pair : m_trading_pairs) {
      th_full = m_markets[trading_pair]->getTradeHistory();
      th_full->loadFromDatabase(start_time - history_duration, start_time);

      COUT << "Data loaded from " << (start_time - history_duration) << " to "
           << th_full->getLatestTick().getTimeStamp() << endl
           << endl;

      TickPeriod trades_to_fill;
      fillTrades(trades_to_fill, trading_pair, 0, (th_full->getLatestTick().getUniqueID() + 1));
      th_full->appendTrades(trades_to_fill);

      if (m_mode != exchange_mode_t::REAL) getVirFullHistory().insert(make_pair(trading_pair, th_full));

      th_past = new TradeHistory(exchange_t::COINBASE, trading_pair);
      th_past->appendTrades(*th_full->getTickPeriod());

      if (m_mode != exchange_mode_t::SIMULATION) m_histories_till_ctrl_time.insert(make_pair(trading_pair, th_past));

      if (m_mode != exchange_mode_t::REAL) getVirPartHistory().insert(make_pair(trading_pair, th_past));
    }

    bool res = connectWebsocket();
    if (res) {
      for (auto& trading_pair : m_trading_pairs) {
        m_markets[trading_pair]->getTradeHistory()->setOngoingTrading(called_from_controller);
        m_ticks_buffer.insert(make_pair(trading_pair, new queue<Tick>()));
      }

      subscribeToTopic();

      m_stream_live_data = true;

      thread t_fill_realtime_trades(&Exchange::fillRealtimeTrades, this);
      t_fill_realtime_trades.detach();
    } else {
      LIVE_TRADING_CONN_ERROR(sExchangeToString(m_id));
    }
  } else {
    if (m_mode != exchange_mode_t::SIMULATION) {
      CTRL_TIME_RANGE_ERROR(start_time, present_time);
    }

    if (end_time > present_time) {
      SIMULATON_TIME_RANGE_ERROR(start_time, end_time);
    }

    for (auto trading_pair : m_trading_pairs) {
      th_full = new TradeHistory(exchange_t::COINBASE, trading_pair);
      th_past = m_markets[trading_pair]->getTradeHistory();

      th_full->loadFromDatabase(start_time - history_duration, end_time);
      th_past->loadFromDatabase(start_time - history_duration, start_time);

      getVirFullHistory().insert(make_pair(trading_pair, th_full));
      getVirPartHistory().insert(make_pair(trading_pair, th_past));
    }
  }
}

void Exchange::setMode(const vector<CurrencyPair>& trading_pairs, const exchange_mode_t mode) {
  m_mode = mode;
  m_trading_pairs = trading_pairs;
}

void Exchange::fillRealtimeTrades() {
  Tick tick;
  TickPeriod trades_to_fill;

  while (m_websocket_connected) {
    unique_lock<mutex> mlock(m_match_mutex);

    for (auto& buffer : m_ticks_buffer) {
      CurrencyPair cp = buffer.first;
      queue<Tick>* tick_queue = buffer.second;

      while (!tick_queue->empty()) {
        tick = tick_queue->front();
        tick_queue->pop();

        if (!m_markets[cp]->getTradeHistory()->appendTrade(tick)) {
          const Tick latest_tick_on_th = m_markets[cp]->getTradeHistory()->getLatestTick();

          fillTrades(trades_to_fill, cp, tick.getUniqueID(), latest_tick_on_th.getUniqueID() + 1);

          m_markets[cp]->getTradeHistory()->appendTrades(trades_to_fill);

          m_markets[cp]->getTradeHistory()->appendTrade(tick);

          trades_to_fill.clear();
        }
      }
    }

    m_match_notify_cv.wait(mlock);  // wait for processMatch thread to signal, avoids busy waiting
  }
}

bool Exchange::checkIfIncludedInTradingPairs(const currency_t a_currency) {
  for (auto& currency_pair : m_trading_pairs) {
    if ((currency_pair.getBaseCurrency() == a_currency) || (currency_pair.getQuoteCurrency() == a_currency))
      return true;
  }

  return false;
}

void Exchange::populateMinOrderAmount() {
  for (auto& cp_json : m_config["min_order_amount"].items()) {
    string currency_str = cp_json.key();
    double val = cp_json.value().get<double>();
    m_min_order_amounts[Currency::sStringToCurrency(currency_str)] = val;
  }
}

void Exchange::initMarkets() {
  for (auto cp_json : m_config["markets"].items()) {
    CurrencyPair cp = CurrencyPair(cp_json.key());
    double val = cp_json.value().get<double>();

    m_markets[cp]->setTradeHistory(new TradeHistory(getExchangeID(), cp, m_consecutive));
    m_markets[cp]->setSpread(val);
  }
}

void Exchange::poupulateResidualAmountsData() {
  for (auto& cp_json : m_config["order_remaining_amount"].items()) {
    string currency_str = cp_json.key();
    const json& data = cp_json.value();

    double avg = data["avg"].get<double>();
    double sd = data["sd"].get<double>();

    m_residual_amounts[Currency::sStringToCurrency(currency_str)] = residual_amount_t{avg, sd};
  }
}

// order needs get fully executed before calling this function
bool Exchange::completeOrder(Order* ap_order) {
  assert(ap_order->getAmount() == 0);

#ifndef DEBUG
  if ((ap_order->getOrderType() == order_type_t::MARKET)
  {
    // DELETE(ap_order);
    return true;
  }
#endif

  order_id_t order_id = "";
  for (auto iter : m_pending_orders)
  {
    if (iter.second->getOrderId() == ap_order->getOrderId()) {
      order_id = iter.first;
      break;
    }
  }

  if (order_id != "")
  {
    m_pending_orders.erase(order_id);

    Controller* p_Controller = TraderBot::getInstance()->getController();
    if (p_Controller) p_Controller->logOrderCancellation(m_id, order_id);
  }
  else
    assert(ap_order->getOrderType() == order_type_t::MARKET);

  //DELETE(ap_order);

  return true;
}

bool Exchange::cancelLimitOrders(const CurrencyPair& currency_pair, const bool buy) {
  bool ret_val = true;

  // create a cpoy
  auto pending_orders = m_pending_orders;

  for (auto iter : pending_orders) {
    order_id_t order_id = iter.first;
    Order* p_order = iter.second;

    if (p_order->getOrderType() != order_type_t::LIMIT) continue;

    if (p_order->getCurrencyPair() != currency_pair) continue;

    bool cancel = false;
    if (buy && (p_order->getOrderDir() == order_direction_t::BUY)) {
      assert(p_order->getTiggerType() == trigger_type_t::MIN);
      cancel = true;
    } else if (!buy && (p_order->getOrderDir() == order_direction_t::SELL)) {
      assert(p_order->getTiggerType() == trigger_type_t::MAX);
      cancel = true;
    }

    if (cancel) {
      bool success = cancelOrder(order_id);
      ret_val &= success;

      if (success) m_cancelled_orders.push_back(p_order);
    }
  }

  return ret_val;
}

void Exchange::clearOrder(const order_id_t order_id) {
  Order* p_order = m_pending_orders[order_id];
  ASSERT(p_order);

  p_order->discard();
  // DELETE(p_order);

  m_pending_orders.erase(order_id);

  Controller* p_Controller = TraderBot::getInstance()->getController();
  if (p_Controller) {
    p_Controller->logOrderCancellation(m_id, order_id);
    updateAccounts();
  }
}

bool Exchange::cancelPendingOrders(const CurrencyPair& currency_pair) {
  bool ret_val = true;

  // create a cpoy
  auto pending_orders = m_pending_orders;

  for (auto iter : pending_orders) {
    order_id_t order_id = iter.first;
    const Order* order = iter.second;

    if (order->getCurrencyPair() != currency_pair) continue;

    bool success = cancelOrder(order_id);
    ret_val &= success;

    if (success) m_cancelled_orders.push_back(order);
  }

  assert(!ret_val || (m_pending_orders.size() == 0));

  return ret_val;
}

double Exchange::getBidPrice(const CurrencyPair& currency_pair) const {
  if (!m_websocket_connected || m_markets.at(currency_pair)->getBidPrice() == 0) {
    double ticker_val = getTicker(currency_pair);
    return (ticker_val - 0.01);
  }

  return m_markets.at(currency_pair)->getBidPrice();
}

double Exchange::getAskPrice(const CurrencyPair& currency_pair) const {
  if (!m_websocket_connected || m_markets.at(currency_pair)->getBidPrice() == 0) {
    double ticker_val = getTicker(currency_pair);
    return (ticker_val + 0.01);
  }

  return m_markets.at(currency_pair)->getAskPrice();
}
