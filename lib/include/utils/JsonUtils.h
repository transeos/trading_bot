//
// Created by Subhagato Dutta on 9/20/17.
//

#ifndef CRYPTOTRADER_JSONUTIL_H
#define CRYPTOTRADER_JSONUTIL_H

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// getJsonValue() : overloaded function to handle different data types of json output.
static void getJsonValue(const json& j, std::string key, std::string& val) {
  val = j[key].get<std::string>();
}
static void getJsonValue(const json& j, std::string key, int& val) {
  val = std::stoi(j[key].get<std::string>());
}
static void getJsonValue(const json& j, std::string key, double& val) {
  val = std::stod(j[key].get<std::string>());
}
static void getJsonValue(const json& j, std::string key, time_t& val) {
  val = std::stoi(j[key].get<std::string>());
}

// getJsonValue() : overloaded function to handle different data types of json output.
template <typename T, typename U>
static U getJsonValueT(const json& j, std::string key) {
  std::stringstream ss;
  U val;

  ss << j[key].get<T>();
  ss >> val;

  return val;
}

#endif  // CRYPTOTRADER_JSONUTIL_H
