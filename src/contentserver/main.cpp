/* $Id$ */

#include "shared/stdafx.h"
#include "shared/mysql.h"
#include "shared/debug.h"
#include "contentserver.h"

int main(int argc, char *argv[])
{
	bool fork = false;
	NetworkAddressList addresses;

	ParseCommandArguments(argc, argv, addresses, NETWORK_CONTENT_SERVER_PORT, &fork, "contentserver");

	SQL *sql = new MySQL(MYSQL_CONTENT_HOST, MYSQL_CONTENT_USER, MYSQL_CONTENT_PASS, MYSQL_CONTENT_DB, MYSQL_CONTENT_PORT);
	Server *server = new ContentServer(sql, addresses);
	server->Run("contentserver.log", "contentserver", fork);
	delete server;
	delete sql;

	return 0;
}
