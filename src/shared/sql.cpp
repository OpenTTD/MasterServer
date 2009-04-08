/* $Id$ */

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
