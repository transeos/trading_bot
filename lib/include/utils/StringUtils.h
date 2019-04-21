//
// Created by Subhagato Dutta on 12/24/17.
//

#ifndef CRYPTOTRADER_STRINGUTILS_H
#define CRYPTOTRADER_STRINGUTILS_H

#include <iostream>
#include <vector>

namespace StringUtils {

extern int s_str_offset;

std::vector<std::string> split(const std::string& s, char delimiter);

std::string generateRandomHEXstring(const int length);

std::string trimString(std::string str);
}

#endif  // CRYPTOTRADER_STRINGUTILS_H
