/* $Id$ */

/*
 * This file is part of OpenTTD's content service.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "shared/stdafx.h"
#include "shared/mysql.h"
#include "shared/debug.h"
#include "contentserver.h"

#include "shared/safeguards.h"

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
