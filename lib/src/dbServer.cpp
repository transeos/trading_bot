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

#include "dbServer.h"
#include "Globals.h"
#include "utils/Logger.h"
#include <cassert>

#define CASS_PORT 9042

using namespace std;

CassServer::CassServer() {
  m_cluster = NULL;
  m_session = cass_session_new();

  string hosts = g_cassandra_ip;

  m_cluster = createCluster(hosts.c_str(), CASS_PORT);

  // setting request time out to 10 min (default: 2 sec) to handle large requests
  cass_cluster_set_request_timeout(m_cluster, 600000);

  if (connectSession(m_session, m_cluster) != CASS_OK) {
    cass_cluster_free(m_cluster);
    cass_session_free(m_session);
    m_session = NULL;

    CT_CRIT_WARN << "not able to connect to cassanda database\n";
    return;
  }

  executeQuery(m_session,
               "CREATE KEYSPACE IF NOT EXISTS crypto WITH replication = { 'class': 'SimpleStrategy', "
               "'replication_factor': '1' };");

  g_cass_session = m_session;
}

CassServer::~CassServer() {
  if (m_session) {
    CassFuture* close_future = cass_session_close(m_session);
    cass_future_wait(close_future);
    cass_future_free(close_future);

    cass_cluster_free(m_cluster);
    cass_session_free(m_session);
  }

  m_session = NULL;
}

void CassServer::printError(CassFuture* future) {
  const char* message;
  size_t message_length;
  cass_future_error_message(future, &message, &message_length);
  CT_CRIT_WARN << "Cass_Error: " << message << endl;
}

CassCluster* CassServer::createCluster(const char* hosts, int port) {
  CassCluster* cluster = cass_cluster_new();
  cass_cluster_set_contact_points(cluster, hosts);
  cass_cluster_set_port(cluster, port);
  return cluster;
}

CassError CassServer::connectSession(CassSession* session, const CassCluster* cluster) {
  CassFuture* future = cass_session_connect(session, cluster);
  cass_future_wait(future);

  CassError rc = cass_future_error_code(future);
  if (rc != CASS_OK) printError(future);

  cass_future_free(future);
  return rc;
}

CassError CassServer::executeQuery(CassSession* session, const char* query, bool verbose) {
  // COUT<<"cql_query = " << query << endl;

  CassStatement* statement = cass_statement_new(query, 0);

  CassFuture* future = cass_session_execute(session, statement);
  cass_future_wait(future);

  CassError rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    if (verbose) printError(future);
  }

  cass_future_free(future);
  cass_statement_free(statement);
  return rc;
}
