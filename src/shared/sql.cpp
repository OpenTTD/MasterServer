/* $Id$ */

#include "stdafx.h"
#include "server.h"
#include "sql.h"

/**
 * @file sql.cpp Implementation of some 'core class' -> string/number conversion functions
 */

void SQL::MakeServerOnline(QueriedServer *server)
{
	this->MakeServerOnline(inet_ntoa(server->GetServerAddress()->sin_addr), ntohs(server->GetServerAddress()->sin_port));
}

void SQL::MakeServerOffline(QueriedServer *server)
{
	this->MakeServerOffline(inet_ntoa(server->GetServerAddress()->sin_addr), ntohs(server->GetServerAddress()->sin_port));
}

void SQL::UpdateNetworkGameInfo(QueriedServer *server, NetworkGameInfo *info)
{
	this->UpdateNetworkGameInfo(inet_ntoa(server->GetServerAddress()->sin_addr), ntohs(server->GetServerAddress()->sin_port), info);
}

void SQL::AddServerAddress(ServerAddress result[], int index, const char *ip, uint16 port)
{
	result[index].ip   = inet_addr(ip);
	result[index].port = port;
}

