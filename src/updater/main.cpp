/* $Id$ */

#include "shared/stdafx.h"
#include "shared/mysql.h"
#include "shared/debug.h"
#include "updater.h"

int main(int argc, char *argv[])
{
	char hostname[NETWORK_HOSTNAME_LENGTH];
	bool fork = false;

	ParseCommandArguments(argc, argv, hostname, sizeof(hostname), &fork, "updater");

	SQL *sql = new MySQL();
	Server *server = new Updater(sql, hostname);
	server->Run("updater.log", "updater", fork);
	delete server;
	delete sql;

	return 0;
}
