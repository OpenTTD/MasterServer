/* $Id$ */

/*
 * This file is part of OpenTTD's master server.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "shared/stdafx.h"
#include "shared/mysql.h"
#include "shared/debug.h"
#include "masterserver.h"

int main(int argc, char *argv[])
{
	bool fork = false;
	NetworkAddressList addresses;

	ParseCommandArguments(argc, argv, addresses, NETWORK_MASTER_SERVER_PORT, &fork, "masterserver");

	SQL *sql = new MySQL(MYSQL_MSU_HOST, MYSQL_MSU_USER, MYSQL_MSU_PASS, MYSQL_MSU_DB, MYSQL_MSU_PORT);
	Server *server = new MasterServer(sql, &addresses);
	server->Run("masterserver.log", "masterserver", fork);
	delete server;
	delete sql;

  return 0;
}
