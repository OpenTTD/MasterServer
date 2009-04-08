/* $Id$ */

#include "stdafx.h"
#include "udp_server.h"
#include "debug.h"

/**
 * @file udp_server.cpp Shared UDP server (master server/updater) related functionality
 */

UDPServer::UDPServer(SQL *sql) : Server(sql)
{
	this->frame        = 0;
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

QueriedServer *UDPServer::GetQueriedServer(NetworkAddress *addr)
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

QueriedServer::QueriedServer(NetworkAddress address, uint frame) :
	server_address(address),
	attempts(0),
	frame(frame)
{
}

void QueriedServer::SendFindGameServerPacket(NetworkUDPSocketHandler *socket)
{
	/* Send game info query */
	Packet packet(PACKET_UDP_CLIENT_FIND_SERVER);
	socket->SendPacket(&packet, &this->server_address);
}
