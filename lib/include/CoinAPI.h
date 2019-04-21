//
// Created by Subhagato Dutta on 12/28/17.
//

#ifndef CRYPTOTRADER_COINAPI_H
#define CRYPTOTRADER_COINAPI_H

#include "CoinAPIHistory.h"
#include "exchanges/Exchange.h"
#include "utils/ConcurrentQueue.h"
#include <unordered_set>

// sync interval = 10 sec
#define COINAPI_SYNC_INTERVAL 10_sec

class Websocket2JSON;

class CoinAPISubs {
 private:
  exchange_t m_exchange_id;
  CurrencyPair m_currency_pair;

  Time m_last_reset;
  Duration m_average_delay;
  int m_tick_count;

  void resetDelay();

 public:
  CoinAPISubs(const exchange_t a_exchange_id, const CurrencyPair a_currency_pair);
  ~CoinAPISubs() {}

  inline exchange_t getExchangeId() const {
    return m_exchange_id;
  }
  inline CurrencyPair getCurrencyPair() const {
    return m_currency_pair;
  }

  void addDelay(const Time a_exchange_time);
  Duration getDelay() const {
    return m_average_delay;
  };
};

class CoinAPI {
 private:
  std::string m_api_key;
  http_header_t m_auth_headers;

  std::string m_rest_api_endpoint;
  std::string m_websocket_endpoint;

  RestAPI2JSON* m_rest_api_handle;
  Websocket2JSON* m_websocket_handle;

  std::unordered_set<Currency> m_currencies;
  std::unordered_map<exchange_t, std::unordered_set<CurrencyPair>> m_currency_pairs;

  // subscription symbols
  std::map<std::string, CoinAPISubs> m_subs_symbols;

  bool m_websocket_connected;

  std::map<std::string, ConcurrentQueue<json>> m_message_queues;

  CoinAPIHistory m_history;

  FILE* mp_tick_data_file;

  // Loads supported currency pairs of different exchanges
  void loadDataFiles();

  void websocketCallback(json message);
  void processHeartbeat();
  void processTrade();
  void popUnnecessary();

 public:
  CoinAPI(std::string api_key);
  ~CoinAPI();

  void parseConfigFile();

  bool connectWebsocket();
  bool subscribeToTopics();

  // TODO
  bool disconnectWebsocket() {
    assert(0);
  }

  CoinAPIHistory& getHistory() {
    return m_history;
  }
};

#endif  // CRYPTOTRADER_COINAPI_H
