//
// Created by subhagato on 11/24/17.
//

#ifndef CRYPTOTRADER_DBMETADATA_H
#define CRYPTOTRADER_DBMETADATA_H

#include "DataTypes.h"
#include "Globals.h"
#include "dbServer.h"
#include "utils/Logger.h"
#include "utils/dbUtils.h"

template <typename T>
class Database;

class dbMetadata {
 private:
  template <typename T>
  friend class Database;

  std::string m_key_space;
  std::string m_table_name;

  size_t m_num_entries;

  primary_key_t m_oldest_entry_key;
  primary_key_t m_min_entry_key;
  primary_key_t m_max_entry_key;
  primary_key_t m_cons_entry_key;

  std::string m_table_type;

  bool getMetadata() {
    CassFuture* future;
    CassStatement* cass_statement;
    std::string statement;
    std::stringstream query;

    bool found = false;

    query << "SELECT * FROM " << m_key_space << ".metadata WHERE table_name = '" << m_table_name << "'";

    statement = query.str();

    // COUT<<"statement = "<<statement<<std::endl;

    cass_statement = cass_statement_new(statement.c_str(), 0);

    future = cass_session_execute(g_cass_session, cass_statement);

    cass_future_wait(future);

    CassError rc = cass_future_error_code(future);

    if (rc != CASS_OK) {
      CassServer::printError(future);
    } else {
      const CassResult* result = cass_future_get_result(future);
      CassIterator* iterator = cass_iterator_from_result(result);

      if (cass_iterator_next(iterator)) {
        const CassRow* cass_row = cass_iterator_get_row(iterator);
        const CassValue* oldest_entry_tuple_value = cass_row_get_column_by_name(cass_row, "oldest_entry");
        const CassValue* min_entry_tuple_value = cass_row_get_column_by_name(cass_row, "min_entry");
        const CassValue* max_entry_tuple_value = cass_row_get_column_by_name(cass_row, "max_entry");
        const CassValue* consistant_till_tuple_value = cass_row_get_column_by_name(cass_row, "consistent_till");

        /* Create an iterator for the UDT value */
        CassIterator* oldest_entry_tuple_iterator = cass_iterator_from_tuple(oldest_entry_tuple_value);
        CassIterator* min_entry_tuple_iterator = cass_iterator_from_tuple(min_entry_tuple_value);
        CassIterator* max_entry_tuple_iterator = cass_iterator_from_tuple(max_entry_tuple_value);
        CassIterator* consistant_till_tuple_iterator = cass_iterator_from_tuple(consistant_till_tuple_value);

        /* Iterate over the tuple field oldest_entry */
        cass_iterator_next(oldest_entry_tuple_iterator);
        cass_value_get_int32(cass_iterator_get_value(oldest_entry_tuple_iterator), &m_oldest_entry_key.date);
        cass_iterator_next(oldest_entry_tuple_iterator);
        cass_value_get_int64(cass_iterator_get_value(oldest_entry_tuple_iterator), &m_oldest_entry_key.time);
        cass_iterator_next(oldest_entry_tuple_iterator);
        cass_value_get_int64(cass_iterator_get_value(oldest_entry_tuple_iterator), &m_oldest_entry_key.unique_id);

        /* Iterate over the tuple field min_entry */
        cass_iterator_next(min_entry_tuple_iterator);
        cass_value_get_int32(cass_iterator_get_value(min_entry_tuple_iterator), &m_min_entry_key.date);
        cass_iterator_next(min_entry_tuple_iterator);
        cass_value_get_int64(cass_iterator_get_value(min_entry_tuple_iterator), &m_min_entry_key.time);
        cass_iterator_next(min_entry_tuple_iterator);
        cass_value_get_int64(cass_iterator_get_value(min_entry_tuple_iterator), &m_min_entry_key.unique_id);

        /* Iterate over the tuple field max_entry */
        cass_iterator_next(max_entry_tuple_iterator);
        cass_value_get_int32(cass_iterator_get_value(max_entry_tuple_iterator), &m_max_entry_key.date);
        cass_iterator_next(max_entry_tuple_iterator);
        cass_value_get_int64(cass_iterator_get_value(max_entry_tuple_iterator), &m_max_entry_key.time);
        cass_iterator_next(max_entry_tuple_iterator);
        cass_value_get_int64(cass_iterator_get_value(max_entry_tuple_iterator), &m_max_entry_key.unique_id);

        /* Iterate over the tuple field max_entry */
        cass_iterator_next(consistant_till_tuple_iterator);
        cass_value_get_int32(cass_iterator_get_value(consistant_till_tuple_iterator), &m_cons_entry_key.date);
        cass_iterator_next(consistant_till_tuple_iterator);
        cass_value_get_int64(cass_iterator_get_value(consistant_till_tuple_iterator), &m_cons_entry_key.time);
        cass_iterator_next(consistant_till_tuple_iterator);
        cass_value_get_int64(cass_iterator_get_value(consistant_till_tuple_iterator), &m_cons_entry_key.unique_id);

        DbUtils::cass_value_get_type(cass_row, "num_entries", "bigint", &m_num_entries);
        DbUtils::cass_value_get_type(cass_row, "type", "ascii", &m_table_type);

        cass_iterator_free(oldest_entry_tuple_iterator);
        cass_iterator_free(min_entry_tuple_iterator);
        cass_iterator_free(max_entry_tuple_iterator);
        cass_iterator_free(consistant_till_tuple_iterator);

        found = true;
      }

      cass_result_free(result);
      cass_iterator_free(iterator);
    }

    cass_statement_free(cass_statement);
    cass_future_free(future);

    return found;
  }

  bool setMetadata() {
    if (!g_update_cass) return false;

    std::string statement;
    std::stringstream query;

    query << "UPDATE " << m_key_space << ".metadata SET";

    query << " oldest_entry = (" << m_oldest_entry_key.date << "," << m_oldest_entry_key.time << ","
          << m_oldest_entry_key.unique_id << ")";
    query << ", min_entry = (" << m_min_entry_key.date << "," << m_min_entry_key.time << ","
          << m_min_entry_key.unique_id << ")";
    query << ", max_entry = (" << m_max_entry_key.date << "," << m_max_entry_key.time << ","
          << m_max_entry_key.unique_id << ")";
    query << ", consistent_till = (" << m_cons_entry_key.date << "," << m_cons_entry_key.time << ","
          << m_cons_entry_key.unique_id << ")";

    query << ", num_entries = " << m_num_entries;
    query << ", type = '" << m_table_type << "'";

    query << " WHERE table_name = '" << m_table_name << "'";
    statement = query.str();

    // COUT<<"statement = "<<statement<<std::endl;

    CassServer::executeQuery(g_cass_session, statement.c_str(), false);

    return true;
  }

 public:
  static const int32_t kInitDate = 14600;

  dbMetadata(std::string key_space, std::string table_name, std::string table_type)
      : m_key_space(key_space), m_table_name(table_name), m_table_type(table_type) {
    if (g_update_cass) {
      std::stringstream query;

      query << "CREATE TABLE IF NOT EXISTS " << key_space << ".metadata (table_name ascii, "
                                                             "oldest_entry tuple<int,bigint,bigint>, "
                                                             "min_entry tuple<int,bigint,bigint>, "
                                                             "max_entry tuple<int,bigint,bigint>, "
                                                             "num_entries bigint, "
                                                             "consistent_till tuple<int,bigint,bigint>, "
                                                             "type ascii, "
                                                             "PRIMARY KEY (table_name))";

      // create table if it doesn't exists
      CassServer::executeQuery(g_cass_session, query.str().c_str(), false);

    } else {
      // WARNING<<"Cassandra Database will be not updated\n";
    }

    if (!getMetadata()) {
      m_oldest_entry_key = {0, 0, 0};
      m_min_entry_key = {INT32_MAX, 0, INT64_MAX};
      m_max_entry_key = {0, 0, 0};
      m_cons_entry_key = {0, 0, 0};
      m_num_entries = 0;
      setMetadata();
    }
  }

  const std::string& getTableName() const {
    return m_table_name;
  }

  void setTableName(const std::string& table_name) {
    m_table_name = table_name;
    setMetadata();
  }

  primary_key_t getOldestEntryKey() const {
    return m_oldest_entry_key;
  }

  primary_key_t getMinEntryKey() const {
    return m_min_entry_key;
  }

  primary_key_t getMaxEntryKey() const {
    return m_max_entry_key;
  }

  primary_key_t getConsistentEntryKey() const {
    return m_cons_entry_key;
  }

  void setOldestEntryKey(primary_key_t key) {
    m_oldest_entry_key = key;
    setMetadata();
  }

  void setMinEntryKey(primary_key_t key) {
    m_min_entry_key = key;
    setMetadata();
  }

  void setMaxEntryKey(primary_key_t key) {
    m_max_entry_key = key;
    setMetadata();
  }

  void setConsistentEntryKey(primary_key_t key) {
    m_cons_entry_key = key;
    setMetadata();
  }

  int32_t getMinDate() const {
    return m_min_entry_key.date;
  }

  int64_t getMinTime() const {
    return m_min_entry_key.time;
  }

  void setMinDate(int32_t min_date) {
    m_min_entry_key.date = min_date;
    setMetadata();
  }

  void setMinTime(int64_t min_time) {
    m_min_entry_key.time = min_time;
    setMetadata();
  }

  int32_t getMaxDate() const {
    return m_max_entry_key.date;
  }

  int64_t getMaxTime() const {
    return m_max_entry_key.time;
  }

  void setMaxDate(int32_t max_date) {
    m_max_entry_key.date = max_date;
    setMetadata();
  }

  void setMaxTime(int64_t max_time) {
    m_max_entry_key.time = max_time;
    setMetadata();
  }

  int64_t getMinUniqueID() const {
    return m_min_entry_key.unique_id;
  }

  void setMinUniqueID(int64_t min_unique_id) {
    m_min_entry_key.unique_id = min_unique_id;
    setMetadata();
  }

  int64_t getMaxUniqueID() const {
    return m_max_entry_key.unique_id;
  }

  void setMaxUniqueID(int64_t max_unique_id) {
    m_max_entry_key.unique_id = max_unique_id;
    setMetadata();
  }

  size_t getNumEntries() const {
    return m_num_entries;
  }

  void setNumEntries(int64_t num_entries) {
    m_num_entries = num_entries;
    setMetadata();
  }

  const std::string& getType() const {
    return m_table_type;
  }

  void setType(const std::string& type) {
    m_table_type = type;
    setMetadata();
  }

  friend std::ostream& operator<<(std::ostream& os, const dbMetadata& m) {
    os << "min_entry_key        = " << m.m_min_entry_key << std::endl;
    os << "max_entry_key        = " << m.m_max_entry_key << std::endl;
    os << "consistent_entry_key = " << m.m_cons_entry_key << std::endl;
    os << "num_entries          = " << m.m_num_entries;

    return os;
  }

  void print() const {
    TradeUtils::displayT(*this);
  }
};

#endif  // CRYPTOTRADER_DBMETADATA_H
