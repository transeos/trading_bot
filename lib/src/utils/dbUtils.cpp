//
// Created by subhagato on 11/24/17.
//

#include "utils/dbUtils.h"
#include "utils/ErrorHandling.h"

using namespace std;

namespace DbUtils {

void cass_statement_bind_type(CassStatement* statement, size_t index, int32_t value) {
  cass_statement_bind_int32(statement, index, (cass_int32_t)(value));
}
void cass_statement_bind_type(CassStatement* statement, size_t index, int64_t value) {
  cass_statement_bind_int64(statement, index, (cass_int64_t)(value));
}
void cass_statement_bind_type(CassStatement* statement, size_t index, float value) {
  cass_statement_bind_float(statement, index, (cass_float_t)(value));
}
void cass_statement_bind_type(CassStatement* statement, size_t index, double value) {
  cass_statement_bind_double(statement, index, (cass_double_t)(value));
}
void cass_statement_bind_type(CassStatement* statement, size_t index, const char* value) {
  cass_statement_bind_string(statement, index, value);
}

void cass_statement_bind_type(CassStatement* statement, size_t index, string type, void* value) {
  if (type == "int") {
    cass_statement_bind_int32(statement, index, *(cass_int32_t*)(value));
  } else if (type == "bigint") {
    cass_statement_bind_int64(statement, index, *(cass_int64_t*)(value));
  } else if (type == "float") {
    cass_statement_bind_float(statement, index, *(cass_float_t*)(value));
  } else if (type == "double") {
    cass_statement_bind_double(statement, index, *(cass_double_t*)(value));
  } else if (type == "ascii") {
    cass_statement_bind_string(statement, index, (const char*)(value));
  } else {
    CASS_BIND_TYPE_ERROR(type);
  }
}

void cass_value_get_type(const CassRow* cass_row, string field_name, string field_type, void* value) {
  if (field_type == "int") {
    cass_value_get_int32(cass_row_get_column_by_name(cass_row, field_name.c_str()), static_cast<cass_int32_t*>(value));
  } else if (field_type == "bigint") {
    cass_value_get_int64(cass_row_get_column_by_name(cass_row, field_name.c_str()), static_cast<cass_int64_t*>(value));
  } else if (field_type == "float") {
    cass_value_get_float(cass_row_get_column_by_name(cass_row, field_name.c_str()), static_cast<cass_float_t*>(value));
  } else if (field_type == "double") {
    cass_value_get_double(cass_row_get_column_by_name(cass_row, field_name.c_str()),
                          static_cast<cass_double_t*>(value));
  } else if (field_type == "ascii") {
    const char* str_data;
    size_t str_len;
    cass_value_get_string(cass_row_get_column_by_name(cass_row, field_name.c_str()), &str_data, &str_len);
    static_cast<string*>(value)->assign(str_data, str_len);
  } else {
    CASS_GETVAL_TYPE_ERROR(field_type);
  }
}

void writeInCSV(FILE* ap_file, Time value, bool millisecond) {
  fprintf(ap_file, "%s", value.toISOTimeString(millisecond).c_str());
}

void writeInCSV(FILE* ap_file, int32_t value) {
  fprintf(ap_file, "%d", value);
}
void writeInCSV(FILE* ap_file, int64_t value) {
  fprintf(ap_file, "%ld", value);
}
void writeInCSV(FILE* ap_file, float value) {
  fprintf(ap_file, "%f", value);
}
void writeInCSV(FILE* ap_file, double value) {
  fprintf(ap_file, "%lf", value);
}
void writeInCSV(FILE* ap_file, const char* value) {
  fprintf(ap_file, "%s", value);
}

void writeInCSV(FILE* ap_file, string type, const void* value) {
  if (type == "bool")
    writeInCSV(ap_file, *(static_cast<const bool*>(value)));
  else if (type == "int")
    writeInCSV(ap_file, *(static_cast<const int32_t*>(value)));
  else if (type == "bigint")
    writeInCSV(ap_file, *(static_cast<const int64_t*>(value)));
  else if (type == "float")
    writeInCSV(ap_file, *(static_cast<const float*>(value)));
  else if (type == "double")
    writeInCSV(ap_file, *(static_cast<const double*>(value)));
  else if (type == "ascii")
    writeInCSV(ap_file, static_cast<const char*>(value));
  else {
    CSV_DATA_TYPE_ERROR(type);
  }
}

void printData(string type, const void* value) {
  if (type == "bool")
    COUT << *(static_cast<const bool*>(value));
  else if (type == "int")
    COUT << *(static_cast<const int32_t*>(value));
  else if (type == "bigint")
    COUT << *(static_cast<const int64_t*>(value));
  else if (type == "float")
    COUT << *(static_cast<const float*>(value));
  else if (type == "double")
    COUT << *(static_cast<const double*>(value));
  else if (type == "ascii")
    COUT << static_cast<const char*>(value);
  else {
    CSV_DATA_TYPE_ERROR(type);
  }
}
}
