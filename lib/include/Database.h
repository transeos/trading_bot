//
// Created by Subhagato Dutta on 10/24/17.
//

#ifndef CRYPTOTRADER_DATABASES_H
#define CRYPTOTRADER_DATABASES_H

#define CASS_MAX_BATCH_STATEMENTS 25

#include "CurrencyPair.h"
#include "dbMetadata.h"
#include "iterator_tpl.h"
#include "utils/TimeUtils.h"
#include "utils/TraderUtils.h"

/*******
 *
   {
      "table_prefix": "crypto",
      "table_name": "btc_usd_gdax",
      "members":
                {
                    "partition_key": [ {"day": "int"} ],
                    "clustering_key": [ {"timestamp": "bigint"}, {"trade_id": "bigint"}],
                    "other_fields": [ {"price": "double"}, {"size": "double"} ]
                 }
    }
*/

template <typename T>
class Database {
 private:
  field_t m_row_fields;
  std::string m_create_table_cql;
  std::string m_insert_cql;
  std::string m_select_cql;
  std::string m_delete_cql;
  std::string m_table_name;
  std::string m_key_space;
  std::string m_name;

  bool m_consecutive;
  bool m_check_insert;

  primary_key_t m_start_key;
  primary_key_t m_end_key;

  CassStatement* m_cass_statement;
  CassBatch* m_cass_batch;
  size_t m_primary_key_fields;

  dbMetadata* m_db_metadata;

  bool generateInsertQuery(bool check_for_existance = false);

  bool generateSelectQuery(bool greater_equal = true, bool less_equal = false);

  bool generateCreateTableQuery();

  bool generateDeleteQuery(bool greater_equal = true, bool less_equal = false, bool delete_all = false);

  std::string generateSingleRowQuery(int32_t date, bool latest = true) const;

  void bindRowToCassStatement(CassStatement* cass_statement, T& row) const;

  void bindQueryParamsToCassStatement(CassStatement* cass_statement, int32_t date, Time time_start,
                                      Time time_end) const;

  void bindQueryParamsToCassStatement(CassStatement* cass_statement, primary_key_t start_key,
                                      primary_key_t end_key) const;

  T getRowFromCassIterator(const CassRow* cass_row) const;

  bool addStatementToCassBatch(T& row);

  bool insertSingleRowToDatabase(T& row);

  size_t insertMultipleRowsToDatabase(typename std::deque<T>::iterator start_itr,
                                      typename std::deque<T>::iterator end_itr, bool check_if_exists = true);

  bool createBatchSession();

  bool executeBatchSession() const;

  void executeCassStatements(std::vector<CassStatement*>& cass_statements, std::vector<bool>& result, size_t start = 0,
                             size_t end = SIZE_MAX);

  bool getSingleRow(std::string statement, T& row);

  size_t updateMetadata(typename std::deque<T>::iterator start_itr, typename std::deque<T>::iterator end_itr,
                        std::vector<bool>& results, bool check_if_exists = true);

  bool storeDataInBatch(std::deque<T>& a_data);

 public:
  Database(const std::string& key_space, const std::string& name, const bool a_consecutive = true,
           const bool a_check_insert = true);
  Database(const std::string& key_space, exchange_t exchange_id, CurrencyPair currency_pair,
           const bool a_consecutive = true, const bool a_check_insert = true)
      : Database(key_space, getTableName(exchange_id, currency_pair), a_consecutive, a_check_insert) {}

  ~Database();

#if 0
	// Warning: this code deletes all data from the database
	void deleteTable()
	{
		CassServer::executeQuery(g_cass_session, m_delete_cql.c_str());
	}
#endif

  static std::string getTableName(exchange_t exchange_id, CurrencyPair currency_pair, bool lower = true);

  bool createTable() const;

  // If possible following two function can be made private and accessed through friend class.
  int loadData(std::deque<T>& data, Time start_time = Time(),
               Time end_time = Time());  // by default it will load all data from database

  // by default it wil store all data to database
  size_t storeData(std::deque<T>& data, Time start_time = Time(), Time end_time = Time(), bool check_if_exists = true);

  size_t storeData(typename std::deque<T>::iterator start_itr, typename std::deque<T>::iterator end_itr);

  // by default it wil delete all data from database
  size_t deleteData(Time start_time = Time(), Time end_time = Time());

  void restoreDataFromCSVFile(const std::string& a_filename, std::vector<int8_t> mapping = {});
  void saveDataToCSVFile(const std::string& a_filename, const Time a_start_time, const Time a_end_time);

  bool getNewestRow(T& row);
  bool getOldestRow(T& row);

  bool fixDatabase(bool full_scan, bool fix_metadata, primary_key_t& min_key, primary_key_t& max_key,
                   primary_key_t& last_key, int64_t& num_entries,
                   std::vector<std::pair<primary_key_t, primary_key_t>>& discontinuity_id_pairs);

  bool checkIfFilledFromBeginning();

  bool updateOldestEntryMetadata();

  int64_t getNewestUniqueID() {
    return m_db_metadata->getMaxUniqueID();
  }
  int64_t getOldestUniqueID() {
    return m_db_metadata->getMinUniqueID();
  }

  int64_t getNumEntries() {
    return m_db_metadata->getNumEntries();
  }

  dbMetadata* getMetadata() const {
    return m_db_metadata;
  }

  // Iterators from https://github.com/VinGarcia/Simple-Iterator-Template/

  STL_TYPEDEFS(T);  // (Optional)

  bool setScopeAll() {
    m_start_key = {dbMetadata::kInitDate, 0, 0};
    m_end_key = {Time::sNow().days_since_epoch(), Time::sNow().micros_since_midnight(), INT64_MAX};
    generateSelectQuery();

    return true;
  }

  bool setScope(Time start_time, Time end_time, bool include_first = true, bool include_last = false) {
    m_start_key.date = start_time.days_since_epoch();
    m_start_key.time = start_time.micros_since_midnight();
    m_start_key.unique_id = 0;

    m_end_key.date = end_time.days_since_epoch();
    m_end_key.time = end_time.micros_since_midnight();
    m_end_key.unique_id = INT64_MAX;

    generateSelectQuery(include_first, include_last);

    return true;
  }

  bool setScope(primary_key_t start_key, primary_key_t end_key, bool include_first = true, bool include_last = false) {
    m_start_key = start_key;
    m_end_key = end_key;

    generateSelectQuery(include_first, include_last);

    return true;
  }

  struct it_state {
    CassIterator* cass_iterator;
    CassResult* cass_result;

    primary_key_t start_day_key;
    primary_key_t end_day_key;
    int32_t current_date;
    bool end_of_table;
    bool next_exist;

    primary_key_t start_key;
    primary_key_t end_key;

    inline void next(const Database* ref) {
      for (; current_date <= end_key.date; current_date++) {
        if (!next_exist) {
          CassStatement* cass_statement = cass_statement_new(ref->m_select_cql.c_str(), ref->m_primary_key_fields);

          if (current_date == start_key.date)
            start_day_key = ref->m_start_key;
          else
            start_day_key = {current_date, 0, 0};  // start of day

          if (current_date == end_key.date) {
            end_day_key = ref->m_end_key;
            if (end_day_key.time == 0) {
              end_of_table = true;
              break;
            }

          } else {
            end_day_key = {current_date, static_cast<int64_t>(Duration(1, 0, 0, 0, 0, -1)), INT64_MAX};  // end of day
          }

          ref->bindQueryParamsToCassStatement(cass_statement, start_day_key, end_day_key);

          CassFuture* cass_future = cass_session_execute(g_cass_session, cass_statement);

          cass_future_wait(cass_future);

          CassError rc = cass_future_error_code(cass_future);

          if (rc != CASS_OK) {
            CassServer::printError(cass_future);
            end_of_table = true;
            cass_future_free(cass_future);
            cass_statement_free(cass_statement);
            break;
          } else {
            cass_result = const_cast<CassResult*>(cass_future_get_result(cass_future));
            cass_iterator = cass_iterator_from_result(cass_result);
            // cass_result_free(cass_result);
            cass_future_free(cass_future);
            cass_statement_free(cass_statement);
          }
        }

        next_exist = cass_iterator_next(cass_iterator);

        if (next_exist) {
          break;
        } else {
          if (current_date == end_key.date) {
            end_of_table = true;
          }
          try_to_free_iterator();
        }
      }
    }

    /* reverse traversal is not supported now
    inline void prev(const Database* ref) {
            --pos;
    } // (Optional) */

    inline void begin(const Database* ref) {
      start_key = ref->m_start_key;
      end_key = ref->m_end_key;

      if (ref->m_start_key == primary_key_t()) {
        start_key = ref->m_db_metadata->getMinEntryKey();
      }

      if (ref->m_end_key == primary_key_t()) {
        end_key = ref->m_db_metadata->getMaxEntryKey();
      }

      current_date = start_key.date;
      end_of_table = false;
      next_exist = false;

      try_to_free_iterator();

      next(ref);
    }
    inline void end(const Database* ref) {
      end_of_table = true;
    }
    inline T get(Database* ref) {
      if (!end_of_table) {
        const CassRow* row = cass_iterator_get_row(cass_iterator);
        return ref->getRowFromCassIterator(row);
      } else {
        throw std::runtime_error("End of table!");
      }
    }
    inline const T get(const Database* ref) {
      if (!end_of_table) {
        const CassRow* row = cass_iterator_get_row(cass_iterator);
        return ref->getRowFromCassIterator(row);
      } else {
        throw std::runtime_error("End of table!");
      }
    }
    inline bool cmp(const it_state& s) const {
      return end_of_table != s.end_of_table;
    }

    inline void try_to_free_iterator() {
      if (cass_iterator != nullptr) {
        cass_iterator_free(cass_iterator);
        cass_result_free(cass_result);
        cass_iterator = nullptr;
      }
    }

    it_state() {
      cass_iterator = nullptr;
    }
  };
  SETUP_ITERATORS(Database, T, it_state);
  //	SETUP_REVERSE_ITERATORS(Database, T, it_state); // (Optional, not implemented)
};

#endif  // CRYPTOTRADER_DATABASES_H
