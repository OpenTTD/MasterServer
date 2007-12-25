/* $Id$ */

#include "shared/stdafx.h"
#include "shared/debug.h"
#include "masterserver.h"

/**
 * @file masterserver/udp.cpp Handler of incoming UDP master server packets
 */

DEF_UDP_RECEIVE_COMMAND(Query, PACKET_UDP_SERVER_RESPONSE)
{
	MSQueriedServer *qs = this->ms->GetQueriedServer(client_addr);

	/* We were NOT waiting for this server.. drop it */
	if (qs == NULL) {
		DEBUG(net, 0, "received an unexpected 'server response' from %s:%d",
				inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
		return;
	}

	DEBUG(net, 3, "received a 'server response' from %s:%d",
			inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));

	/* Send an okay-signal to the server */
	Packet packet(PACKET_UDP_MASTER_ACK_REGISTER);
	this->SendPacket(&packet, qs->GetQueryAddress());

	/* Add the server to the list with online servers */
	this->ms->GetSQLBackend()->MakeServerOnline(qs);
	delete this->ms->RemoveQueriedServer(qs);
}

DEF_UDP_RECEIVE_COMMAND(Master, PACKET_UDP_SERVER_REGISTER)
{
	char welcome_message[NETWORK_NAME_LENGTH];

	/* Check if we understand this client */
	p->Recv_string(welcome_message, sizeof(welcome_message));
	if (strncmp(welcome_message, NETWORK_MASTER_SERVER_WELCOME_MESSAGE, sizeof(welcome_message)) != 0) {
		return;
	}

	/* See what kind of server we have (protocol wise) */
	byte master_server_version = p->Recv_uint8();
	if (master_server_version != 1) {
		/* We do not know this master server version */
		DEBUG(net, 0, "received a registration request with unknown master server version from %s:%d",
				inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
		return;
	}

	DEBUG(net, 3, "received a registration request from %s:%d",
			inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));

	MSQueriedServer *qs = new MSQueriedServer(client_addr, htons(p->Recv_uint16()));

	/* Shouldn't happen ofcourse, but still ... */
	if (this->HasClientQuit()) {
		delete qs;
		return;
	}

	/* Now request some data from the server to see if it is really alive */
	qs->SendFindGameServerPacket(this->ms->GetQuerySocket());

	/* Register the server to the list of quering servers. */
	delete this->ms->AddQueriedServer(qs);
}

DEF_UDP_RECEIVE_COMMAND(Master, PACKET_UDP_SERVER_UNREGISTER)
{
	/* See what kind of server we have (protocol wise) */
	uint8 master_server_version = p->Recv_uint8();
	if (master_server_version != 1) {
		/* We do not know this version, bail out */
		DEBUG(net, 0, "received a unregistration request from %s:%d with unknown master server version",
				inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));

		return;
	}

	DEBUG(net, 3, "received a unregistration request from %s:%d",
			inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));

	QueriedServer *qs = new QueriedServer(client_addr->sin_addr.s_addr, htons(p->Recv_uint16()));

	/* Shouldn't happen ofcourse, but still ... */
	if (this->HasClientQuit()) {
		delete qs;
		return;
	}

	/* Remove the server from the list of online servers */
	this->ms->GetSQLBackend()->MakeServerOffline(qs);
	delete this->ms->RemoveQueriedServer(qs);
	delete qs;
}

DEF_UDP_RECEIVE_COMMAND(Master, PACKET_UDP_CLIENT_GET_LIST)
{
	DEBUG(net, 3, "received a request for the game server list from %s:%d",
			inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));

	this->SendPacket(this->ms->GetServerListPacket(), client_addr);
}