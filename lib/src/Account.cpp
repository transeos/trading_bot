// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy Basak on 17/04/18.
//

#include "Account.h"
#include <iomanip>

using namespace std;

void Account::saveHeaderInCSV(FILE* ap_file) const {
  string acc_id = (m_is_virtual ? m_id : m_id.substr(0, 8) + "[" + Currency::sCurrencyToString(m_currency) + "]");

  fprintf(ap_file, ",%s-balance", acc_id.c_str());
  fprintf(ap_file, ",%s-available", acc_id.c_str());
  fprintf(ap_file, ",%s-hold", acc_id.c_str());
}

void Account::saveInCSV(FILE* ap_file) const {
  fprintf(ap_file, ",%lf,%lf,%lf", getBalance(), m_available, m_hold);
}
