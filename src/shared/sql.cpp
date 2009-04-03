/* $Id$ */

#include "stdafx.h"
#include "udp_server.h"
#include "sql.h"

/**
 * @file sql.cpp Implementation of some 'core class' -> string/number conversion functions
 */

void SQL::MakeServerOnline(QueriedServer *server)
{
	this->MakeServerOnline(server->GetServerAddress()->GetHostname(), server->GetServerAddress()->GetPort());
}

void SQL::MakeServerOffline(QueriedServer *server)
{
	this->MakeServerOffline(server->GetServerAddress()->GetHostname(), server->GetServerAddress()->GetPort());
}

void SQL::UpdateNetworkGameInfo(QueriedServer *server, NetworkGameInfo *info)
{
	this->UpdateNetworkGameInfo(server->GetServerAddress()->GetHostname(), server->GetServerAddress()->GetPort(), info);
}

void SQL::AddServerAddress(ServerAddress result[], int index, const char *ip, uint16 port)
{
	result[index].ip   = inet_addr(ip);
	result[index].port = port;
}

