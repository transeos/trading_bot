// -*- C++ -*-
//
//*****************************************************************
//
// WARRANTY:
// Use all material in this file at your own risk.
//
// Created by Hiranmoy on 15/10/17
//
//*****************************************************************

#ifndef DBSERVER_H
#define DBSERVER_H

#include <cassandra.h>
#include <iostream>

class CassServer {
 private:
  CassCluster* m_cluster;
  CassSession* m_session;

  CassCluster* createCluster(const char* hosts, int port = 9042);
  CassError connectSession(CassSession* session, const CassCluster* cluster);

 public:
  CassServer();
  ~CassServer();

  static CassError executeQuery(CassSession* session, const char* query, bool verbose = true);
  static void printError(CassFuture* future);
};

#endif  // DBSERVER_H
