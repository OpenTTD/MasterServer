/* $Id$ */

#include "stdafx.h"
#include "udp_server.h"
#include "debug.h"

/**
 * @file udp_server.cpp Shared UDP server (master server/updater) related functionality
 */

UDPServer::UDPServer(SQL *sql, const char *host, NetworkUDPSocketHandler *query_socket) : Server(sql)
{
	this->query_socket = query_socket;
	this->frame        = 0;

	if (!this->query_socket->Listen(inet_addr(host), 0, false)) {
		error("Could not bind to %s:0\n", host);
	}
}

UDPServer::~UDPServer()
{
	/* Clean stuff up */
	delete this->query_socket;
}

void UDPServer::ReceivePackets()
{
	this->query_socket->ReceivePackets();
}

void UDPServer::CheckServers()
{
	QueriedServerMap::iterator iter = this->queried_servers.begin();

	for (; iter != this->queried_servers.end();) {
		QueriedServer *srv = iter->second;
		iter++;
		srv->DoAttempt(this);
	}
}

void UDPServer::RealRun()
{
	byte small_frame = 0;

	while (!this->stop_server) {
		small_frame++;
		/* We want _frame to be in (approximatelly) seconds (not 0.1 seconds) */
		if (small_frame % 10 == 0) {
			this->frame++;
			small_frame = 0;

			/* Check if we have servers that are expired */
			this->CheckServers();
		}

		/* Check if we have any data on the socket */
		this->ReceivePackets();
		CSleep(100);
	}
}

QueriedServer *UDPServer::GetQueriedServer(const struct sockaddr_in *addr)
{
	QueriedServerMap::iterator iter = this->queried_servers.find(addr);
	if (iter == this->queried_servers.end()) return NULL;
	return iter->second;
}

QueriedServer *UDPServer::AddQueriedServer(QueriedServer *qs)
{
	QueriedServer *ret = this->RemoveQueriedServer(qs);

	this->queried_servers[qs->GetServerAddress()] = qs;
	return ret;
}

QueriedServer *UDPServer::RemoveQueriedServer(QueriedServer *qs)
{
	QueriedServerMap::iterator iter = this->queried_servers.find(qs->GetServerAddress());
	if (iter == this->queried_servers.end()) return NULL;

	QueriedServer *ret = iter->second;
	this->queried_servers.erase(iter);

	return ret;
}

QueriedServer::QueriedServer(uint32 address, uint16 port, uint frame)
{
	this->server_address.sin_family      = AF_INET;
	this->server_address.sin_addr.s_addr = address;
	this->server_address.sin_port        = port;

	this->attempts = 0;
	this->frame    = frame;
}

void QueriedServer::SendFindGameServerPacket(NetworkUDPSocketHandler *socket)
{
	/* Send game info query */
	Packet packet(PACKET_UDP_CLIENT_FIND_SERVER);
	socket->SendPacket(&packet, &this->server_address);
}
