//
// Created by Subhagato Dutta on 12/28/17.
//

#include "CoinAPI.h"
#include "CoinAPITick.h"
#include "CurrencyPair.h"
#include "Database.h"
#include "utils/ErrorHandling.h"
#include "utils/Logger.h"
#include "utils/RestAPI2JSON.h"
#include "utils/TraderUtils.h"
#include "utils/Websocket2JSON.h"

using namespace std;
using namespace nlohmann;

//
// ============================================== CoinAPISubs class ===========================================
//
CoinAPISubs::CoinAPISubs(const exchange_t a_exchange_id, const CurrencyPair a_currency_pair)
    : m_exchange_id(a_exchange_id), m_currency_pair(a_currency_pair) {
  m_last_reset = Time::sNow();
  m_average_delay = Duration();
  m_tick_count = 0;
}

void CoinAPISubs::addDelay(const Time a_exchange_time) {
  const Duration delay = (Time::sNow() - a_exchange_time);
  if (delay.getDuration() < 0) {
    CT_WARN << "CoinAPI: exchange time(" << a_exchange_time << ") is less than current time(" << Time::sNow()
            << ") for " << Exchange::sExchangeToString(m_exchange_id) << ":" << m_currency_pair << endl;

    return;
  }

  m_average_delay = (((m_average_delay * m_tick_count) + delay) / (m_tick_count + 1));
  m_tick_count++;

  if ((Time::sNow() - m_last_reset) > COINAPI_SYNC_INTERVAL) {
    m_last_reset = Time::sNow();
    resetDelay();
  }
}

void CoinAPISubs::resetDelay() {
  if (m_average_delay == Duration()) return;

  // dump current data
  COUT << CBLUE << setw(10) << Exchange::sExchangeToString(m_exchange_id) << " : ";
  COUT << CYELLOW << setw(8) << m_currency_pair;
  COUT << " (" << m_tick_count << ") delay = " << m_average_delay.getDurationInSec() << " sec" << endl;

  // reset
  m_average_delay = Duration();
  m_tick_count = 0;
}

//
// ============================================== CoinAPI class ===========================================
//
CoinAPI::CoinAPI(string api_key) {
  m_api_key = api_key;

  m_rest_api_endpoint = "https://rest.coinapi.io/v1";
  m_websocket_endpoint = "wss://ws.coinapi.io/v1";

  m_rest_api_handle = new RestAPI2JSON(m_rest_api_endpoint);
  m_websocket_handle = new Websocket2JSON(true);

  m_websocket_handle->bindCallback(bind(&CoinAPI::websocketCallback, this, _1));

  m_auth_headers["X-CoinAPI-Key:"] = api_key;

  m_websocket_connected = false;

#ifdef DEBUG
  string csv_file = Exchange::sExchangeToString(exchange_t::COINAPI) + ".csv";
  mp_tick_data_file = fopen(csv_file.c_str(), "w");
  ASSERT(mp_tick_data_file);
#else
  mp_tick_data_file = NULL;
#endif

  loadDataFiles();

  parseConfigFile();
}

CoinAPI::~CoinAPI() {
  DELETE(m_rest_api_handle);
  DELETE(m_websocket_handle);

  if (mp_tick_data_file) fclose(mp_tick_data_file);
}

void CoinAPI::loadDataFiles() {
  // Supported currencies in the exchange
  const string currencies_json_filename = g_trader_home + "/configs/coinapi/coinio_currencies.json";
  const string currency_pairs_json_filename = g_trader_home + "/configs/coinapi/coinio_currency_pairs.json";
  const string exchanges_json_filename = g_trader_home + "/configs/coinapi/coinio_exchanges.json";

  ifstream currencies_json_file(currencies_json_filename);
  ifstream currency_pairs_json_file(currency_pairs_json_filename);
  ifstream exchanges_json_file(exchanges_json_filename);

  json json_response;

  try {
    // json_response = m_rest_api_handle->getJSON_GET("/assets", &m_auth_headers); // take the first element
    json_response = json::parse(currencies_json_file);

    for (auto j : json_response)  // access by value, the type of i is int
    {
      // COUT<<<<j["asset_id"].get<string>()<<", "<<static_cast<bool>(j["type_is_crypto"].get<int>())<<endl;

      Currency currency = Currency(j["asset_id"].get<string>(), static_cast<bool>(j["type_is_crypto"].get<int>()));
      if (m_currencies.find(currency) != m_currencies.end()) {
        INVALID_JSON_ERROR(currencies_json_filename);
      }
      m_currencies.insert(currency);
    }
  } catch (detail::type_error err) {  // NOLINT
    JSON_EXCEPTION_ERROR(err.what(), currencies_json_filename);
  } catch (runtime_error err) {  // NOLINT
    JSON_EXCEPTION_ERROR(err.what(), currencies_json_filename);
  }

  try {
    // json_response = m_rest_api_handle->getJSON_GET("/exchanges", &m_auth_headers); // take the first element
    json_response = json::parse(exchanges_json_file);

    for (auto j : json_response)  // access by value, the type of i is int
    {
      exchange_t exchange_id = Exchange::sStringToExchange(j["exchange_id"].get<string>());
      if (m_currency_pairs.find(exchange_id) != m_currency_pairs.end()) {
        INVALID_JSON_ERROR(exchanges_json_filename);
      }
      m_currency_pairs[exchange_id] = unordered_set<CurrencyPair>();
    }
  } catch (detail::type_error err) {  // NOLINT
    JSON_EXCEPTION_ERROR(err.what(), exchanges_json_filename);
  } catch (runtime_error err) {  // NOLINT
    JSON_EXCEPTION_ERROR(err.what(), exchanges_json_filename);
  }

  try {
    // json_response = m_rest_api_handle->getJSON_GET("/symbols", &m_auth_headers); // take the first element
    json_response = json::parse(currency_pairs_json_file);

    for (auto j : json_response)  // access by value, the type of i is int
    {
      exchange_t exchange_id = Exchange::sStringToExchange(j["exchange_id"].get<string>());
      CurrencyPair currency_pair =
          CurrencyPair(j["asset_id_base"].get<string>() + "-" + j["asset_id_quote"].get<string>());

      unordered_set<CurrencyPair>& currency_pairs = m_currency_pairs[exchange_id];
      if (currency_pairs.find(currency_pair) != currency_pairs.end()) {
        // WARNING("Duplicate entry in " << Exchange::sExchangeToString(exchange_id) << " for " <<
        // Currency::sCurrencyToString(currency_pair.getBaseCurrency()) << "-" <<
        // Currency::sCurrencyToString(currency_pair.getQuoteCurrency()));
        // ERROR(2)<<"json parsing in " << g_trader_home << "/data/coinio_currency_pairs.json";
      }
      currency_pairs.insert(currency_pair);
    }
  } catch (detail::type_error err) {  // NOLINT
    JSON_EXCEPTION_ERROR(err.what(), currency_pairs_json_filename);
  } catch (runtime_error err) {  // NOLINT
    JSON_EXCEPTION_ERROR(err.what(), currency_pairs_json_filename);
  }
}

bool CoinAPI::connectWebsocket() {
  if (m_websocket_connected) return true;

  try {
    m_websocket_handle->connect(m_websocket_endpoint);
  } catch (const exception& e) {
    CT_CRIT_WARN << e.what() << endl;
    return false;
  } catch (websocketpp::lib::error_code e) {
    CT_CRIT_WARN << e.message() << endl;
    return false;
  } catch (...) {
    CT_CRIT_WARN << "other exception\n";
    return false;
  }

  m_websocket_connected = true;

  thread t_heartbeat(&CoinAPI::processHeartbeat, this);
  t_heartbeat.detach();

  thread t_ticker(&CoinAPI::processTrade, this);
  t_ticker.detach();

  thread t_unnecessary(&CoinAPI::popUnnecessary, this);
  t_unnecessary.detach();

  return true;
}

bool CoinAPI::subscribeToTopics() {
  json message;

  if (!m_websocket_connected) {
    WEBSOCKET_CONN_ERROR("subscribe to topic(s)", Exchange::sExchangeToString(exchange_t::COINAPI));
  }

  json json_topic = {{"type", "hello"},
                     {"apikey", ""},
                     {"heartbeat", true},
                     {"subscribe_data_type", {}},
                     {"subscribe_filter_symbol_id", {}}};

  json_topic["apikey"] = m_api_key;
  json_topic["subscribe_data_type"].push_back("trade");

  for (auto symbol_iter : m_subs_symbols) {
    string symbol = symbol_iter.first;
    json_topic["subscribe_filter_symbol_id"].push_back(symbol);
  }

  try {
    m_websocket_handle->send(json_topic);
  } catch (const exception& e) {
    CT_CRIT_WARN << e.what() << endl;
    return false;
  } catch (websocketpp::lib::error_code e) {
    CT_CRIT_WARN << e.message() << endl;
    return false;
  } catch (...) {
    CT_CRIT_WARN << "other exception" << endl;
    return false;
  }

  return true;
}

void CoinAPI::websocketCallback(json message) {
  if (g_exiting) {
    // WARNING << "Exiting CoinAPI::websocketCallback" << endl;
    this_thread::yield();
    return;
  }

  string message_type = message["type"];

  if (message_type == "trade")
    m_message_queues["trade"].push(message);
  else if (message_type == "quote")
    m_message_queues["quote"].push(message);
  else if (message_type == "book20")
    m_message_queues["book20"].push(message);
  else if (message_type == "book50")
    m_message_queues["book50"].push(message);
  else if (message_type == "hearbeat")
    m_message_queues["hearbeat"].push(message);
  else
    m_message_queues[" "].push(message);

  // COUT<<GREEN<< message["type"].get<string>() << endl;
}

void CoinAPI::processHeartbeat() {
  json message;

  while (m_websocket_connected) {
    if (g_exiting) {
      CT_CRIT_WARN << "forced exit in CoinAPI::processHeartbeat" << endl;
      return;
    }
    try {
      message = m_message_queues["hearbeat"].try_pop(chrono::milliseconds(1100));
      if (message == json()) {
        // WARNING<<"Websocket connection disconnected"<<endl;
      } else {
        COUT << CGREEN << "Coin API heartbeat" << endl;
      }
    } catch (...) {
      CT_CRIT_WARN << "error in processing JSON on CoinAPI heart beat" << endl;
    }

    this_thread::yield();
  }
}

void CoinAPI::processTrade() {
  json message;

  while (m_websocket_connected) {
    if (g_exiting) {
      CT_CRIT_WARN << "forced exit in CoinAPI::processTrade" << endl;
      return;
    }

    try {
      message = m_message_queues["trade"].pop();

      const string symbol_id = message["symbol_id"].get<string>();
      // COUT<<CMAGENTA<< symbol_id;

      Time exchange_time = message["time_exchange"].get<string>();
      Time coinapi_time = message["time_coinapi"].get<string>();
      double price = message["price"].get<double>();
      double size = message["size"].get<double>();
      string uuid = message["uuid"].get<string>();

      const string taker_side = message["taker_side"].get<string>();
      if (taker_side == "SELL") size *= (-1);

      // if (m_subs_symbols.at(symbol_id).getExchangeId() == exchange_t::COINBASE)
      //	COUT << Exchange::sExchangeToString(m_subs_symbols.at(symbol_id).getExchangeId()) << ":" << tick << "="
      //<< uuid << endl;

      if (mp_tick_data_file) {
        DbUtils::writeInCSV(mp_tick_data_file, exchange_time, true);
        fprintf(mp_tick_data_file, ",");
        DbUtils::writeInCSV(mp_tick_data_file, coinapi_time, true);
        fprintf(mp_tick_data_file, ",");
        DbUtils::writeInCSV(mp_tick_data_file,
                            Exchange::sExchangeToString(m_subs_symbols.at(symbol_id).getExchangeId()).c_str());
        fprintf(mp_tick_data_file, ",");
        DbUtils::writeInCSV(mp_tick_data_file, m_subs_symbols.at(symbol_id).getCurrencyPair().toString().c_str());
        fprintf(mp_tick_data_file, ",");
        DbUtils::writeInCSV(mp_tick_data_file, price);
        fprintf(mp_tick_data_file, ",");
        DbUtils::writeInCSV(mp_tick_data_file, size);
        fprintf(mp_tick_data_file, ",");
        DbUtils::writeInCSV(mp_tick_data_file, uuid.c_str());
        fprintf(mp_tick_data_file, ",\n");
      }

      if (m_subs_symbols.find(symbol_id) == m_subs_symbols.end()) {
        CT_CRIT_WARN << "Unexpected subscription : " << symbol_id << endl;
        continue;
      }

      const CoinAPITick tick(exchange_time, price, size, m_subs_symbols.at(symbol_id).getExchangeId(), uuid);

      m_history.appendTrade(tick, m_subs_symbols.at(symbol_id).getCurrencyPair());

      m_subs_symbols.at(symbol_id).addDelay(exchange_time);
    } catch (...) {
      COINAPI_EXCEPTION_ERROR("trade processing");
    }

    this_thread::yield();
  }
}

void CoinAPI::popUnnecessary() {
  json message;

  while (m_websocket_connected) {
    if (g_exiting) {
      CT_CRIT_WARN << "forced exit in CoinAPI::popUnnecessary" << endl;
      return;
    }

    try {
      message = m_message_queues[" "].pop();
      COUT << CRED << message.dump(3) << endl;
    } catch (...) {
      COINAPI_EXCEPTION_ERROR("unnecessary pops");
    }

    this_thread::yield();
  }
}

void CoinAPI::parseConfigFile() {
  string config_file = g_trader_home + "/configs/coinApi.config";

  ifstream file(config_file.c_str());
  int line_num = 0;
  string line;
  while (getline(file, line)) {
    istringstream iss(line);
    line_num++;

    int commented = line.find("#");
    if (commented > -1) continue;

    bool whiteSpacesOnly = line.find_first_not_of(' ') == string::npos;
    if (whiteSpacesOnly) continue;

    vector<string> components = StringUtils::split(line, '_');
    if (components.size() != 4) {
      COINAPI_PARSING_ERROR("not able parse", line, line_num, config_file);
    }

    exchange_t exchange_id = Exchange::sStringToExchange(components[0]);
    if (exchange_id == exchange_t::NOEXCHANGE) {
      COINAPI_PARSING_ERROR("incorrect exchange- " << components[0], line, line_num, config_file);
    }

    if (components[1] != "SPOT") {
      COINAPI_PARSING_ERROR("SPOT not found in", line, line_num, config_file);
    }

    currency_t base_currency = Currency::sStringToCurrency(components[2]);
    currency_t quote_currency = Currency::sStringToCurrency(components[3]);
    if ((base_currency == currency_t::UNDEF) || (quote_currency == currency_t::UNDEF)) {
      COINAPI_PARSING_ERROR("incorrect currency(s)- " << components[2] << "_" << components[3], line, line_num,
                            config_file);
    }

    if (m_currency_pairs.find(exchange_id) == m_currency_pairs.end()) {
      COINAPI_PARSING_ERROR("exchange not found in coinapi list- " << components[0], line, line_num, config_file);
    }

    const unordered_set<CurrencyPair>& currency_pairs = m_currency_pairs[exchange_id];
    CurrencyPair currency_pair = CurrencyPair{base_currency, quote_currency};
    if (currency_pairs.find(currency_pair) == currency_pairs.end()) {
      COINAPI_PARSING_ERROR("currency pair not found in coinapi list- " << components[2] << "_" << components[3], line,
                            line_num, config_file);
    }

    // COUT<<line << " : " << components[0] << "," << components[2] << "," << components[3] << endl;

    if (m_subs_symbols.find(line) != m_subs_symbols.end()) {
      COINAPI_PARSING_ERROR("duplicate entry", line, line_num, config_file);
    }

    const CoinAPISubs coinAPISubs = CoinAPISubs(exchange_id, currency_pair);
    m_subs_symbols.insert(make_pair(line, coinAPISubs));
    m_history.addEntry(currency_pair);
  }
}
