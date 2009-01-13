/* $Id$ */

#include "shared/stdafx.h"
#include "shared/mysql.h"
#include "shared/debug.h"
#include "masterserver.h"

int main(int argc, char *argv[])
{
	char hostname[NETWORK_HOSTNAME_LENGTH];
	bool fork = false;

	ParseCommandArguments(argc, argv, hostname, sizeof(hostname), &fork, "masterserver");

	SQL *sql = new MySQL(MYSQL_MSU_HOST, MYSQL_MSU_USER, MYSQL_MSU_PASS, MYSQL_MSU_DB, MYSQL_MSU_PORT);
	Server *server = new MasterServer(sql, hostname, NETWORK_MASTER_SERVER_PORT);
	server->Run("masterserver.log", "masterserver", fork);
	delete server;
	delete sql;

  return 0;
}
