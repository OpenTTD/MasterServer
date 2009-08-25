/* $Id$ */

/*
 * This file is part of OpenTTD's master server/updater and content service.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"
#include "udp_server.h"
#include "sql.h"

/**
 * @file sql.cpp Implementation of some 'core class' -> string/number conversion functions
 */

void SQL::MakeServerOnline(QueriedServer *server)
{
	NetworkAddress *address = server->GetServerAddress();
	this->MakeServerOnline(address->GetHostname(), address->GetPort(),
			address->GetAddress()->ss_family == AF_INET6, server->GetSessionKey());
}

void SQL::MakeServerOffline(QueriedServer *server)
{
	NetworkAddress *address = server->GetServerAddress();
	this->MakeServerOffline(address->GetHostname(), address->GetPort());
}

void SQL::UpdateNetworkGameInfo(QueriedServer *server, NetworkGameInfo *info)
{
	NetworkAddress *address = server->GetServerAddress();
	this->UpdateNetworkGameInfo(address->GetHostname(), address->GetPort(), info);
}
