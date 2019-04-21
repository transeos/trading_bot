//
// Created by Subhagato Dutta on 10/24/17.
//

#include "Database.h"
#include "CMCandleStick.h"
#include "Candlestick.h"
#include "CoinAPITick.h"
#include "Tick.h"
#include "exchanges/Exchange.h"
#include <fstream>

using namespace std;

// Example:
//    string str_format = R"(   {
//      "table_prefix": "crypto",
//      "table_name": "btc_to_usd_gdax",
//      "members":
//                {
//                    "partition_key": [ {"day": "int"} ],
//                    "clustering_key": [ {"timestamp": "bigint"}, {"trade_id": "bigint"}],
//                    "other_fields": [ {"price": "double"}, {"size": "double"} ]
//                 }
//    }
//    )";
//
// table name : Database<Tick>::getTableName(exchange_t::GDAX, {currency_t::BTC, currency_t::USD}, true);

template <typename T>
Database<T>::Database(const string& key_space, const string& name, const bool a_consecutive, const bool a_check_insert)
    : m_key_space(key_space), m_name(name), m_consecutive(a_consecutive), m_check_insert(a_check_insert) {
  m_table_name = key_space + "." + name;
  m_start_key = primary_key_t{0, 0, 0};
  m_end_key = primary_key_t{0, 0, 0};

  generateCreateTableQuery();
  generateInsertQuery();
  generateSelectQuery();
  generateDeleteQuery();

  if (T::m_unique_id.first != "")
    m_primary_key_fields = 5;
  else
    m_primary_key_fields = 3;

  // COUT<<m_create_table_cql<<endl;
  // COUT<<m_insert_cql<<endl;
  // COUT<<m_select_cql<<endl;
  // COUT<<m_delete_cql<<endl;

  m_db_metadata = new dbMetadata(key_space, name, T::m_type);
}

template <typename T>
Database<T>::~Database() {
  // COUT<<"Database " + m_name + " destructed\n" << endl;

  DELETE(m_db_metadata)
}

template <typename T>
string Database<T>::getTableName(exchange_t exchange_id, CurrencyPair currency_pair, bool lower) {
  string symbol = currency_pair.toString("_", lower) + "_" + Exchange::sExchangeToString(exchange_id, lower);
  return symbol;
}

template <typename T>
bool Database<T>::generateCreateTableQuery() {
  stringstream query;
  stringstream primary_key;

  field_t row_fields;

  query << "CREATE TABLE IF NOT EXISTS " + m_table_name;
  query << " (date int, time bigint";

  // unique id is empty if time stamp is the unique it
  if (T::m_unique_id.first != "") query << ", " << T::m_unique_id.first << " " << T::m_unique_id.second << " ";

  for (const auto& field : T::m_fields) query << ", " << field.first << " " << field.second;

  query << ", PRIMARY KEY (date, time ";

  // unique id is empty if time stamp is the unique it
  if (T::m_unique_id.first != "") query << ", " << T::m_unique_id.first;

  query << "))";

  m_create_table_cql = query.str();

  return true;
}

template <typename T>
bool Database<T>::generateInsertQuery(bool check_for_existance) {
  stringstream query;

  query << "INSERT INTO " + m_table_name;

  int num_fields = 0;

  query << " (date, time";

  // unique id is empty if time stamp is the unique it
  if (T::m_unique_id.first != "") {
    query << ", " << T::m_unique_id.first;
    num_fields++;
  }

  num_fields += 2;

  for (const auto& field : T::m_fields) {
    query << ", " << field.first;
    num_fields++;
  }

  query << ") VALUES (";

  for (int i = 0; i < num_fields; i++) {
    query << "?,";
  }

  query.seekp(-1, query.cur);

  if (check_for_existance)
    query << ") IF NOT EXISTS;";
  else
    query << ");";

  m_insert_cql = query.str();

  return true;
}

template <typename T>
bool Database<T>::generateSelectQuery(bool greater_equal, bool less_equal) {
  stringstream query;
  string left_equal = greater_equal ? "=" : "";
  string right_equal = less_equal ? "=" : "";

  // unique id is empty if time stamp is the unique it
  if (T::m_unique_id.first != "") {
    query << "SELECT * FROM " + m_table_name + " WHERE date = ? AND (time," << T::m_unique_id.first << ") >"
          << left_equal << " (?,?) AND (time," << T::m_unique_id.first << ") <" << right_equal << " (?,?)";
  } else {
    query << "SELECT * FROM " + m_table_name + " WHERE date = ? AND time >= ? AND time < ?";
  }

  m_select_cql = query.str();

  // COUT<<m_select_cql<<endl;

  return true;
}

template <typename T>
bool Database<T>::generateDeleteQuery(bool greater_equal, bool less_equal, bool delete_all) {
  stringstream query;
  string left_equal = greater_equal ? "=" : "";
  string right_equal = less_equal ? "=" : "";

  if (delete_all) {
    query << "DROP TABLE " + m_table_name;
  } else {
    // unique id is empty if time stamp is the unique it
    if (T::m_unique_id.first != "") {
      query << "DELETE FROM " + m_table_name + " WHERE date = ? AND (time," << T::m_unique_id.first << ") >"
            << left_equal << " (?,?) AND (time," << T::m_unique_id.first << ") <" << right_equal << " (?,?)";
    } else {
      query << "DELETE FROM " + m_table_name + " WHERE date = ? AND time >= ? AND time < ?";
    }
  }

  m_delete_cql = query.str();

  // COUT<<m_select_cql<<endl;

  return true;
}

template <typename T>
void Database<T>::executeCassStatements(vector<CassStatement*>& cass_statements, vector<bool>& result, size_t start,
                                        size_t end) {
  CassFuture* cass_future;
  bool applied_result = true;

  for (size_t i = start; i < cass_statements.size(); i++) {
    if (i == end) break;

    cass_future = cass_session_execute(g_cass_session, cass_statements[i]);

    cass_statement_free(cass_statements[i]);

    const CassError rc = cass_future_error_code(cass_future);

    if (rc == CASS_OK) {
      const CassResult* cass_result = cass_future_get_result(cass_future);

      const CassRow* cass_row = cass_result_first_row(cass_result);

      if (cass_row) {
        const CassValue* applied = cass_row_get_column(cass_row, 0);

        cass_value_get_bool(applied, (cass_bool_t*)&applied_result);

        result[i] = applied_result;
      }

      cass_result_free(cass_result);

    } else {
      CassServer::printError(cass_future);
      result[i] = false;
    }

    cass_future_free(cass_future);
  }

  return;
}

template <typename T>
bool Database<T>::addStatementToCassBatch(T& row) {
  if (!g_cass_session) return false;

  int num_fields = (T::m_fields.size() + 2);

  if (T::m_unique_id.first != "") num_fields++;

  CassStatement* cass_statement = cass_statement_new(m_insert_cql.c_str(), num_fields);
  bindRowToCassStatement(cass_statement, row);

  cass_batch_add_statement(m_cass_batch, cass_statement);
  cass_statement_free(cass_statement);

  return true;
}

template <typename T>
bool Database<T>::insertSingleRowToDatabase(T& row) {
  if (!g_cass_session) return false;

  g_critcal_task.lock();

  vector<bool> result(1);

  int num_fields = (T::m_fields.size() + 2);

  if (T::m_unique_id.first != "") num_fields++;

  generateInsertQuery(true);

  vector<CassStatement*> cass_statement = {cass_statement_new(m_insert_cql.c_str(), num_fields)};

  bindRowToCassStatement(cass_statement[0], row);

  executeCassStatements(cass_statement, result);

  deque<T> v_row = {row};

  updateMetadata(v_row.begin(), v_row.end(), result);

  g_critcal_task.unlock();

  return result[0];
}

template <typename T>
size_t Database<T>::insertMultipleRowsToDatabase(typename deque<T>::iterator start_itr,
                                                 typename deque<T>::iterator end_itr, bool check_if_exists) {
  if (!g_cass_session) return 0;

  g_critcal_task.lock();

  int num_fields = (T::m_fields.size() + 2);

  if (T::m_unique_id.first != "") num_fields++;

#ifndef DISABLE_THREAD
  uint32_t concurentThreadsSupported = thread::hardware_concurrency();

  // if possible decrease the cpu load.
  if (concurentThreadsSupported > 1) concurentThreadsSupported--;
#endif

  size_t num_entries_inserted = 0;

#ifndef DISABLE_THREAD
  vector<thread> threads;
#endif

  size_t num_rows = 0;

  vector<CassStatement*> cass_statements;

  generateInsertQuery(check_if_exists);

  for (auto row_itr = start_itr; row_itr != end_itr; row_itr++) {
    if (row_itr->getTimeStamp() == Time(0)) continue;  // skip if time is 0
    cass_statements.emplace_back(cass_statement_new(m_insert_cql.c_str(), num_fields));
    bindRowToCassStatement(cass_statements.back(), *row_itr);
    num_rows++;
  }

  vector<bool> result(num_rows, false);

#ifdef DISABLE_THREAD
  executeCassStatements(cass_statements, result, 0, num_rows);
#else
  size_t step = ceil((float)num_rows / concurentThreadsSupported);

  for (size_t i = 0; i < num_rows; i += step) {
    size_t start = i;
    size_t end = i + step;

    if (end > num_rows) end = num_rows;

    threads.push_back(thread(&Database<T>::executeCassStatements, this, ref(cass_statements), ref(result), start, end));
  }

  for (auto&& t : threads) t.join();
#endif

  num_entries_inserted = updateMetadata(start_itr, end_itr, result, check_if_exists);

  g_critcal_task.unlock();

  return num_entries_inserted;
}

template <typename T>
bool Database<T>::createBatchSession() {
  m_cass_batch = cass_batch_new(CASS_BATCH_TYPE_LOGGED);  // Unlogged for performance, atomicity is not guaranteed
  return true;
}

template <typename T>
bool Database<T>::executeBatchSession() const {
  CassFuture* batch_future = cass_session_execute_batch(g_cass_session, m_cass_batch);

  /* Batch objects can be freed immediately after being executed */
  cass_batch_free(m_cass_batch);

  /* This will block until the query has finished */
  CassError rc = cass_future_error_code(batch_future);

  if (rc != CASS_OK) {
    CassServer::printError(batch_future);
    // COUT<<"cass_future_custom_payload_item_count = "<<cass_future_custom_payload_item_count(batch_future)<<endl;
    cass_future_free(batch_future);
    return false;
  } else {
    cass_future_free(batch_future);
    return true;
  }
}

template <typename T>
size_t Database<T>::updateMetadata(typename deque<T>::iterator start_itr, typename deque<T>::iterator end_itr,
                                   vector<bool>& results, bool check_if_exists) {
  primary_key_t min_entry_key = m_db_metadata->getMinEntryKey();
  primary_key_t max_entry_key = m_db_metadata->getMaxEntryKey();

  primary_key_t current_entry_key;

  int i = 0;
  size_t num_entries_added = 0;

  for (auto row_itr = start_itr; row_itr != end_itr; row_itr++) {
    if (results[i] == true) {
      num_entries_added++;

      current_entry_key = row_itr->getPrimaryKey();
      if (current_entry_key > max_entry_key) max_entry_key = current_entry_key;

      if (current_entry_key < min_entry_key) min_entry_key = current_entry_key;
      // COUT<<"num_entries_added = "<<num_entries_added<<endl;
    } else {
      // COUT<<"didn't add = "<<row_itr->getPrimaryKey()<<endl;
    }

    i++;
  }

  if (check_if_exists) {
    int num_entries_exists = m_db_metadata->getNumEntries();

    if (num_entries_exists == 0 || min_entry_key != m_db_metadata->getMinEntryKey())
      m_db_metadata->setMinEntryKey(min_entry_key);

    if (num_entries_exists == 0 || max_entry_key != m_db_metadata->getMaxEntryKey())
      m_db_metadata->setMaxEntryKey(max_entry_key);

    if (num_entries_added > 0) m_db_metadata->setNumEntries(num_entries_exists + num_entries_added);

    if (m_check_insert && results.size() > num_entries_added)
      CT_CRIT_WARN << "Number of entries attempted = " << results.size() << " but inserted = " << num_entries_added
                   << endl;
  }

  // COUT<<"updateMetadata: max_date = " << max_date << ", min_date = " << min_date;
  // COUT<<", max_unique_id = " << max_unique_id << ", min_unique_id = " << min_unique_id << endl;

  return num_entries_added;
}

template <typename T>
size_t Database<T>::storeData(deque<T>& data, Time start_time, Time end_time, bool check_if_exists) {
  bool in_session = false;
  int num_rows_to_insert = 0;
  size_t count = 0;

  if (!g_update_cass) {
    CT_CRIT_WARN << m_name << "Database update disabled\n";
    return 0;
  }

  if (data.size() == 0) return 0;

  TickPeriodT<T>::sFixTimestamps(data);

  typename deque<T>::iterator start_itr = data.begin();
  typename deque<T>::iterator end_itr = data.end();

  for (auto row_itr = data.begin(); row_itr != data.end(); row_itr++) {
    Time row_time = row_itr->getTimeStamp();

    if (((start_time == Time()) || (row_time >= start_time)) && ((end_time == Time()) || (row_time < end_time))) {
      if (!in_session) {
        start_itr = row_itr;
        in_session = true;
      }

      num_rows_to_insert++;
      continue;
    }

    if (in_session == true) {
      end_itr = row_itr;
      in_session = false;
    }
  }

  if (num_rows_to_insert > 0) count = insertMultipleRowsToDatabase(start_itr, end_itr, check_if_exists);

  return count;
}

template <typename T>
bool Database<T>::storeDataInBatch(deque<T>& a_data) {
  bool in_session = false;
  int num_statements = 0;
  bool rc = false;

  if (!g_update_cass) {
    CT_CRIT_WARN << m_name << "Database update disabled\n";
    return false;
  }

  if (a_data.size() == 0) return false;

  generateInsertQuery(false);

  for (auto& row : a_data) {
    if (!in_session) {
      createBatchSession();
      in_session = true;
    }

    addStatementToCassBatch(row);
    num_statements++;

    // inserting 25 elements at a time
    if (num_statements >= CASS_MAX_BATCH_STATEMENTS) {
      rc = executeBatchSession();
      in_session = false;

      if (!rc) {
        return false;
      }

      // COUT << "Last Inserted data:" << row << endl;
      num_statements = 0;
    }
  }

  if (num_statements > 0) {
    rc = executeBatchSession();

    if (!rc) {
      return false;
    }
  }

  return true;
}

template <typename T>
size_t Database<T>::storeData(typename deque<T>::iterator start_itr, typename deque<T>::iterator end_itr) {
  size_t count = 0;

  if (!g_update_cass) {
    CT_CRIT_WARN << m_name << "Database update disabled\n";
    return 0;
  }

  count = insertMultipleRowsToDatabase(start_itr, end_itr);

  return count;
}

template <typename T>
bool Database<T>::createTable() const {
  // COUT<<m_create_table_cql<<endl;

  if (!g_update_cass) {
    return false;
  }

  // create table if it doesn't exists
  CassServer::executeQuery(g_cass_session, m_create_table_cql.c_str(), false);

  return true;
}

template <typename T>
int Database<T>::loadData(deque<T>& data, Time start_time, Time end_time) {
  COUT << "loading data from database (" << m_name << "): start_time = " << start_time << " end_time = " << end_time
       << endl;

  int32_t start_date = start_time.days_since_epoch();
  int32_t end_date = end_time.days_since_epoch();

  Time start_time_of_day, end_time_of_day;

  int count = 0;

  CassFuture* future;

  if (start_date == 0) {
    start_date = m_db_metadata->getMinDate();
    start_time_of_day = 0;
  }

  if (end_date == 0) {
    end_date = m_db_metadata->getMaxDate();
    end_time_of_day = Time() + Duration(1, 0, 0, 0, 0, -1);  // end of day
  }

  for (int32_t d = start_date; d <= end_date; d++) {
    m_cass_statement = cass_statement_new(m_select_cql.c_str(), m_primary_key_fields);

    if (d == start_date)
      start_time_of_day = start_time.micros_since_midnight();
    else
      start_time_of_day = 0;  // start of day

    if (d == end_date) {
      end_time_of_day = end_time.micros_since_midnight();
      if (end_time_of_day == Time(0)) break;
    } else {
      end_time_of_day = Time() + Duration(1, 0, 0, 0, 0, -1);  // end of day
    }

    bindQueryParamsToCassStatement(m_cass_statement, d, start_time_of_day, end_time_of_day);

    future = cass_session_execute(g_cass_session, m_cass_statement);

    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK)
      CassServer::printError(future);
    else {
      const CassResult* result = cass_future_get_result(future);
      CassIterator* iterator = cass_iterator_from_result(result);

      while (cass_iterator_next(iterator)) {
        const CassRow* row = cass_iterator_get_row(iterator);
        data.push_back(getRowFromCassIterator(row));
        count++;
      }

      cass_result_free(result);
      cass_iterator_free(iterator);
    }

    cass_statement_free(m_cass_statement);
  }

  return count;
}

template <typename T>
bool Database<T>::getSingleRow(string statement, T& row) {
  CassFuture* future;

  bool found = false;

  // COUT<<"statement = "<<statement<<endl;

  m_cass_statement = cass_statement_new(statement.c_str(), 0);

  future = cass_session_execute(g_cass_session, m_cass_statement);

  cass_future_wait(future);

  CassError rc = cass_future_error_code(future);

  if (rc != CASS_OK) {
    CassServer::printError(future);
  } else {
    const CassResult* result = cass_future_get_result(future);
    CassIterator* iterator = cass_iterator_from_result(result);

    if (cass_iterator_next(iterator)) {
      const CassRow* cass_row = cass_iterator_get_row(iterator);
      row = getRowFromCassIterator(cass_row);
      found = true;
    }

    cass_result_free(result);
    cass_iterator_free(iterator);
  }

  return found;
}

template <typename T>
void Database<T>::bindRowToCassStatement(CassStatement* cass_statement, T& row) const {
  int32_t date = row.getDate();
  int64_t time = row.getTime();
  int64_t unique_id = row.getUniqueID();

  ASSERT(date >= 0);
  ASSERT(time >= 0);

  int num_keys = 2;

  DbUtils::cass_statement_bind_type(cass_statement, 0, date);
  DbUtils::cass_statement_bind_type(cass_statement, 1, time);

  if (T::m_unique_id.first != "") {
    DbUtils::cass_statement_bind_type(cass_statement, 2, unique_id);
    num_keys++;
  }

  size_t i = 0;

  for (auto& row_field : T::m_fields) {
    DbUtils::cass_statement_bind_type(cass_statement, (i + num_keys), row_field.second, row.getMemberPtr(i));
    i++;
  }
}

template <typename T>
T Database<T>::getRowFromCassIterator(const CassRow* cass_row) const {
  T row;
  int32_t date;  // days since 1970-1-1
  int64_t time;  // microseconds since midnight

  DbUtils::cass_value_get_type(cass_row, "date", "int", &date);
  DbUtils::cass_value_get_type(cass_row, "time", "bigint", &time);

  // unique id is empty if time stamp is the unique it
  if (T::m_unique_id.first != "")
    DbUtils::cass_value_get_type(cass_row, T::m_unique_id.first, T::m_unique_id.second, row.getUniqueIdPtr());

  // COUT<<"date = "<<date<<" time = "<<time<<endl;

  row.setTimeStamp(Time(date, time));

  size_t i = 0;
  for (auto& row_field : T::m_fields)
    DbUtils::cass_value_get_type(cass_row, row_field.first, row_field.second, row.getMemberPtr(i++));

  return row;
}

template <typename T>
void Database<T>::bindQueryParamsToCassStatement(CassStatement* cass_statement, int32_t date, Time time_start,
                                                 Time time_end) const {
  assert(date >= 0);
  assert(time_start < time_end);
  assert(time_start >= Time());
  assert(time_end >= Time());

  size_t count = 0;

  DbUtils::cass_statement_bind_type(cass_statement, count++, date);
  DbUtils::cass_statement_bind_type(cass_statement, count++, time_start.micros_since_midnight());
  if (T::m_unique_id.first != "") DbUtils::cass_statement_bind_type(cass_statement, count++, static_cast<int64_t>(0));
  DbUtils::cass_statement_bind_type(cass_statement, count++, time_end.micros_since_midnight());
  if (T::m_unique_id.first != "") DbUtils::cass_statement_bind_type(cass_statement, count, INT64_MAX);
}

template <typename T>
void Database<T>::bindQueryParamsToCassStatement(CassStatement* cass_statement, primary_key_t start_key,
                                                 primary_key_t end_key) const {
  ASSERT(end_key.date == start_key.date);
  ASSERT(end_key >= start_key);

  size_t count = 0;

  DbUtils::cass_statement_bind_type(cass_statement, count++, start_key.date);
  DbUtils::cass_statement_bind_type(cass_statement, count++, start_key.time);
  if (T::m_unique_id.first != "") DbUtils::cass_statement_bind_type(cass_statement, count++, start_key.unique_id);
  DbUtils::cass_statement_bind_type(cass_statement, count++, end_key.time);
  if (T::m_unique_id.first != "") DbUtils::cass_statement_bind_type(cass_statement, count, end_key.unique_id);
}

template <typename T>
void Database<T>::restoreDataFromCSVFile(const string& a_filename, std::vector<int8_t> mapping) {
  if (!TradeUtils::isValidPath(a_filename)) {
    CT_CRIT_WARN << "file not found : " << a_filename << endl;
    return;
  }

  deque<T> data_arr;

  ifstream file(a_filename.c_str());
  string line;

  getline(file, line);  // read header

  while (getline(file, line)) {
    istringstream iss(line);

    if (mapping.size() > 0)
      data_arr.push_back(T(line, mapping));
    else
      data_arr.push_back(T(line));

    // inserting 25000 elements at a time
    if (data_arr.size() == (CASS_MAX_BATCH_STATEMENTS * 1000)) {
      storeData(data_arr, Time(), Time(), false);

      COUT << "Last Inserted data in " << m_name << ":" << data_arr.back() << endl;

      data_arr.clear();
    }
  }

  storeData(data_arr);

  COUT << CYELLOW << "Repairing " << m_name << " database after loading CSV file" << endl;
  primary_key_t last_examined;
  primary_key_t min_key;
  primary_key_t max_key;
  int64_t num_entries;
  vector<pair<primary_key_t, primary_key_t>> discontinuities;

  fixDatabase(false, true, min_key, max_key, last_examined, num_entries, discontinuities);

  if (!discontinuities.size()) getMetadata()->setConsistentEntryKey(last_examined);
}

template <typename T>
void Database<T>::saveDataToCSVFile(const string& a_filename, const Time a_start_time, const Time a_end_time) {
  FILE* p_file = fopen(a_filename.c_str(), "w");
  ASSERT(p_file);

  DbUtils::writeHeaderToCSVLine<T>(p_file);

  deque<T> data_arr;

  loadData(data_arr, a_start_time, a_end_time);

  for (auto& data : data_arr) {
    DbUtils::writeToCSVLine<T>(p_file, data);
    // fflush(p_file);	// optional
  }

  fclose(p_file);
}

template <typename T>
string Database<T>::generateSingleRowQuery(int32_t date, bool latest) const {
  stringstream query;

  if (latest) {
    // unique id is empty if time stamp is the unique it
    if (T::m_unique_id.first != "")
      query << "SELECT * FROM " + m_table_name + " WHERE date = " << date << " ORDER BY time DESC, "
            << T::m_unique_id.first << " DESC LIMIT 1";
    else
      query << "SELECT * FROM " + m_table_name + " WHERE date = " << date << " ORDER BY time DESC LIMIT 1";
  } else
    query << "SELECT * FROM " + m_table_name + " WHERE date = " << date << " LIMIT 1";

  return query.str();
}

template <typename T>
bool Database<T>::getNewestRow(T& row) {
  string statement;

  statement = generateSingleRowQuery(m_db_metadata->getMaxDate(), true);

  return getSingleRow(statement, row);
}

template <typename T>
bool Database<T>::getOldestRow(T& row) {
  string statement;

  statement = generateSingleRowQuery(m_db_metadata->getMinDate(), false);

  return getSingleRow(statement, row);
}

template <typename T>
bool Database<T>::checkIfFilledFromBeginning() {
  return (m_db_metadata->getOldestEntryKey() == m_db_metadata->getMinEntryKey());
}

template <typename T>
bool Database<T>::updateOldestEntryMetadata() {
  T oldestRow;
  getOldestRow(oldestRow);
  m_db_metadata->setOldestEntryKey(oldestRow.getPrimaryKey());
  return true;
}

template <typename T>
bool Database<T>::fixDatabase(bool full_scan, bool fix_metadata, primary_key_t& min_key, primary_key_t& max_key,
                              primary_key_t& last_key, int64_t& num_entries,
                              vector<pair<primary_key_t, primary_key_t>>& discontinuity_id_pairs) {
  COUT << CRED << "Fixing database " << CMAGENTA << m_name << ": " << endl;

  primary_key_t start_key;
  primary_key_t end_key;
  primary_key_t curr_key;
  primary_key_t prev_key;

  int64_t entries_skipped = 0;
  int64_t last_unique_id = -1;
  int64_t current_unique_id;
  int64_t entries_examined = 0;
  uint64_t total_entries;

  min_key = m_db_metadata->getMinEntryKey();
  max_key = min_key;
  last_key = m_db_metadata->getConsistentEntryKey();

  bool repair_needed = false;

  full_scan = full_scan || (last_key == primary_key_t());

  if (full_scan) {
    start_key = {dbMetadata::kInitDate, 0, 0};
    entries_skipped = 0;
    last_unique_id = -1;
  } else {
    start_key = last_key;
    last_unique_id = last_key.unique_id;
    entries_skipped = last_unique_id - min_key.unique_id + 1;
  }
  Time time_now = Time::sNow();

  end_key = {time_now.days_since_epoch(), time_now.micros_since_midnight(), INT64_MAX};

  setScope(start_key, end_key, false, false);

  for (auto tick : *this) {
    current_unique_id = tick.getUniqueID();
    curr_key = tick.getPrimaryKey();

    if (last_unique_id < 0) min_key = tick.getPrimaryKey();

    if (m_consecutive && last_unique_id > 0 && current_unique_id != (last_unique_id + 1)) {
      discontinuity_id_pairs.emplace_back(make_pair(prev_key, curr_key));
      COUT << discontinuity_id_pairs.size() << ". Discontinuity from [" << prev_key.date << "," << last_unique_id
           << "] to [" << curr_key.date << "," << current_unique_id << "] " << (current_unique_id - last_unique_id - 1)
           << " entries." << endl;
      repair_needed = true;
    }

    last_unique_id = current_unique_id;
    prev_key = curr_key;
    entries_examined++;

    if (curr_key > max_key) max_key = curr_key;
  }

  generateSelectQuery();  // restore back default search query

  total_entries = (entries_examined + entries_skipped);

  if (fix_metadata) {
    g_critcal_task.lock();
    if (total_entries == 0) {
      COUT << "Database is empty!!" << endl;
      m_db_metadata->setNumEntries(0);
      m_db_metadata->setMinEntryKey({INT32_MAX, 0, INT64_MAX});
      m_db_metadata->setMaxEntryKey({0, 0, 0});
      return false;
    }

    if (m_db_metadata->getNumEntries() != total_entries) {
      COUT << "Fixing number of entries from " << m_db_metadata->getNumEntries() << " to " << total_entries << endl;
      m_db_metadata->setNumEntries(total_entries);
      repair_needed = true;
    }

    // This should inequality check (!=) instead of greater than (>) check
    if (entries_examined && (max_key != m_db_metadata->getMaxEntryKey())) {
      COUT << "Fixing minimum entry...\n";
      m_db_metadata->setMaxEntryKey(max_key);
      repair_needed = true;
    }

    if (min_key < m_db_metadata->getMinEntryKey()) {
      COUT << "Fixing maximum entry...\n";
      m_db_metadata->setMinEntryKey(min_key);
      repair_needed = true;
    }
    COUT << *m_db_metadata << endl;
    g_critcal_task.unlock();
  } else {
    if (m_db_metadata->getNumEntries() != total_entries || max_key > m_db_metadata->getMaxEntryKey() ||
        min_key < m_db_metadata->getMinEntryKey()) {
      repair_needed = true;
    }
  }

  if (entries_examined > 0) last_key = curr_key;

  return repair_needed;
}

template <typename T>
size_t Database<T>::deleteData(Time start_time, Time end_time) {
  if (!g_cass_session) return 0;

  if (!g_update_cass) {
    CT_CRIT_WARN << m_name << "Database update disabled\n";
    return 0;
  }

  COUT << "delete data from database (" << m_name << "): from = " << start_time << " to = " << end_time << endl;

  int32_t start_date = start_time.days_since_epoch();
  int32_t end_date = end_time.days_since_epoch();

  Time start_time_of_day, end_time_of_day;

  CassFuture* future;

  int count = 0;

  if (start_time == Time() && end_time == Time()) {
    assert(0);  // disable full table delete for now

    generateDeleteQuery(true, false, true);

    m_cass_statement = cass_statement_new(m_delete_cql.c_str(), 0);

    future = cass_session_execute(g_cass_session, m_cass_statement);

    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK) CassServer::printError(future);

    cass_statement_free(m_cass_statement);

    return 1;

  } else {
    generateDeleteQuery(true, false, false);
  }

  if (start_date == 0) {
    start_date = m_db_metadata->getMinDate();
    start_time_of_day = 0;
  }

  if (end_date == 0) {
    end_date = m_db_metadata->getMaxDate();
    end_time_of_day = Time() + Duration(1, 0, 0, 0, 0, -1);  // end of day
  }

  for (int32_t d = start_date; d <= end_date; d++) {
    m_cass_statement = cass_statement_new(m_delete_cql.c_str(), m_primary_key_fields);

    if (d == start_date)
      start_time_of_day = start_time.micros_since_midnight();
    else
      start_time_of_day = 0;  // start of day

    if (d == end_date) {
      end_time_of_day = end_time.micros_since_midnight();
      if (end_time_of_day == Time(0)) break;
    } else {
      end_time_of_day = Time() + Duration(1, 0, 0, 0, 0, -1);  // end of day
    }

    bindQueryParamsToCassStatement(m_cass_statement, d, start_time_of_day, end_time_of_day);

    future = cass_session_execute(g_cass_session, m_cass_statement);

    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK) CassServer::printError(future);

    cass_statement_free(m_cass_statement);
  }

  return count;
}

template class Database<Tick>;
template class Database<Candlestick>;
template class Database<CMCandleStick>;
template class Database<CoinAPITick>;
