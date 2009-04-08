/* $Id$ */

#include "shared/stdafx.h"
#include "shared/mysql.h"
#include "shared/debug.h"
#include "updater.h"

int main(int argc, char *argv[])
{
	bool fork = false;
	NetworkAddressList addresses;

	ParseCommandArguments(argc, argv, addresses, 0, &fork, "updater");

	SQL *sql = new MySQL(MYSQL_MSU_HOST, MYSQL_MSU_USER, MYSQL_MSU_PASS, MYSQL_MSU_DB, MYSQL_MSU_PORT);
	Server *server = new Updater(sql, &addresses);
	server->Run("updater.log", "updater", fork);
	delete server;
	delete sql;

	return 0;
}
