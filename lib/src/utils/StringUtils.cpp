// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 31/04/18.
//

#include "utils/StringUtils.h"
#include "Globals.h"
#include <random>
#include <set>
#include <sstream>

using namespace std;

namespace StringUtils {

int s_str_offset = 0;

vector<string> split(const string& s, char delimiter) {
  vector<string> tokens;
  string token;
  istringstream tokenStream(s);
  while (getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

string generateRandomHEXstring(const int length) {
  if (g_random) {
    string s;
    const string allowed_characters = "abcdef0123456789";
    random_device rd;   // Will be used to obtain a seed for the random number engine
    mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
    uniform_int_distribution<> dis(0, allowed_characters.size() - 1);

    for (int i = 0; i < length; i++) {
      s.push_back(allowed_characters[dis(gen)]);
    }
    return s;
  } else {
    string s;
    for (int idx = 0; idx < length; idx++) s.push_back('a');

    for (size_t idx = 0; idx < s.length(); ++idx) {
      s[idx] = s[idx] + (s_str_offset / pow(6, (s.length() - idx - 1)));
    }

    s_str_offset++;
    return s;
  }
}

string trimString(string str) {
  stringstream trimmer;
  trimmer << str;
  trimmer >> str;

  return str;
}
};
