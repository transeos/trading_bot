//
// Created by subhagato on 11/24/17.
//

#ifndef CRYPTOTRADER_DBUTILS_H
#define CRYPTOTRADER_DBUTILS_H

#include "TimeUtils.h"
#include <cassandra.h>

namespace DbUtils {

void cass_statement_bind_type(CassStatement* statement, size_t index, int32_t value);

void cass_statement_bind_type(CassStatement* statement, size_t index, int64_t value);

void cass_statement_bind_type(CassStatement* statement, size_t index, float value);

void cass_statement_bind_type(CassStatement* statement, size_t index, double value);

void cass_statement_bind_type(CassStatement* statement, size_t index, const char* value);

void cass_statement_bind_type(CassStatement* statement, size_t index, std::string type, void* value);

void cass_value_get_type(const CassRow* cass_row, std::string field_name, std::string field_type, void* value);

void writeInCSV(FILE* ap_file, int32_t value);

void writeInCSV(FILE* ap_file, int64_t value);

void writeInCSV(FILE* ap_file, float value);

void writeInCSV(FILE* ap_file, double value);

void writeInCSV(FILE* ap_file, const char* value);

void writeInCSV(FILE* ap_file, Time value, bool millisecond = false);

void writeInCSV(FILE* ap_file, std::string type, const void* value);

void printData(std::string type, const void* value);

template <typename T>
void writeToCSVLine(FILE* ap_file, T& a_data, const bool complete_line = true) {
  if (complete_line) writeInCSV(ap_file, a_data.getTimeStamp(), true);

  if (T::m_unique_id.first != "") {
    fprintf(ap_file, ",");
    writeInCSV(ap_file, a_data.getUniqueID());
  }

  int i = 0;
  for (auto& field : T::m_fields) {
    fprintf(ap_file, ",");
    writeInCSV(ap_file, field.second, a_data.getMemberPtr(i++));
  }

  if (complete_line) fprintf(ap_file, "\n");
}

template <typename T>
void writeHeaderToCSVLine(FILE* ap_file, const bool complete_line = true) {
  if (complete_line) fprintf(ap_file, "Time");

  if (T::m_unique_id.first != "") fprintf(ap_file, ",%s", T::m_unique_id.first.c_str());

  for (auto& field : T::m_fields) fprintf(ap_file, ",%s", field.first.c_str());

  if (complete_line) fprintf(ap_file, "\n");
}
}

#endif  // CRYPTOTRADER_DBUTILS_H
