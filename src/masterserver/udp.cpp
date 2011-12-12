/* $Id$ */

/*
 * This file is part of OpenTTD's master server.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "shared/stdafx.h"
#include "shared/debug.h"
#include "shared/string_func.h"
#include "masterserver.h"

/**
 * @file masterserver/udp.cpp Handler of incoming UDP master server packets
 */

void QueryNetworkUDPSocketHandler::Receive_SERVER_RESPONSE(Packet *p, NetworkAddress *client_addr)
{
	MSQueriedServer *qs = this->ms->GetQueriedServer(client_addr);

	/* We were NOT waiting for this server.. drop it */
	if (qs == NULL) {
		DEBUG(net, 0, "received an unexpected 'server response' from %s", client_addr->GetAddressAsString());
		return;
	}

	DEBUG(net, 3, "received a 'server response' from %s", client_addr->GetAddressAsString());
	DEBUG(net, 9, " ... sending ack to %s", qs->GetReplyAddress()->GetAddressAsString());

	/* Send an okay-signal to the server */
	this->ms->SendAck(qs);

	/* Add the server to the list with online servers */
	this->ms->GetSQLBackend()->MakeServerOnline(qs);
	delete this->ms->RemoveQueriedServer(qs);
}

void MasterNetworkUDPSocketHandler::Receive_SERVER_REGISTER(Packet *p, NetworkAddress *client_addr)
{
	char welcome_message[NETWORK_NAME_LENGTH];

	/* Check if we understand this client */
	p->Recv_string(welcome_message, sizeof(welcome_message));
	if (strncmp(welcome_message, NETWORK_MASTER_SERVER_WELCOME_MESSAGE, sizeof(welcome_message)) != 0) {
		return;
	}

	/* See what kind of server we have (protocol wise) */
	byte master_server_version = p->Recv_uint8();
	if (master_server_version < 1 || master_server_version > 2) {
		/* We do not know this master server version */
		DEBUG(net, 0, "received a registration request with unknown master server version from %s", client_addr->GetHostname());
		return;
	}

	NetworkAddress reply_addr(*client_addr);
	NetworkAddress query_addr(*client_addr);
	query_addr.SetPort(p->Recv_uint16());

	DEBUG(net, 3, "received a registration request from %s", reply_addr.GetAddressAsString());
	DEBUG(net, 9, " ... for %s", query_addr.GetAddressAsString());

	uint64 session_key;
	if (master_server_version == 2) {
		session_key = p->Recv_uint64();
		if (session_key == 0) {
			DEBUG(net, 9, "sending session key to %s", reply_addr.GetAddressAsString());

			/* Send a new session key */
			Packet packet(PACKET_UDP_MASTER_SESSION_KEY);
			packet.Send_uint64(this->ms->NextSessionKey());
			this->SendPacket(&packet, &reply_addr);
			return;
		}
		if (session_key < (1ULL << (32 + 16 + 1)) || session_key > this->ms->GetSessionKey()) {
			DEBUG(net, 0, "received an invalid session key from %s", reply_addr.GetAddressAsString());
			return;
		}
	} else if (query_addr.GetAddress()->ss_family != AF_INET) {
		DEBUG(net, 0, "received non IPv4 registration with version 1 from %s", client_addr->GetHostname());
		return;
	} else { // master_server_version == 1
		/* Paste together the IPv4 address + it's port to generate the session key */
		session_key = ntohl(((sockaddr_in*)query_addr.GetAddress())->sin_addr.s_addr) | (uint64)query_addr.GetPort() << 32;
	}

	/* Shouldn't happen ofcourse, but still ... */
	if (this->HasClientQuit()) return;

	MSQueriedServer *qs = new MSQueriedServer(query_addr, reply_addr, session_key, this->ms->GetFrame());

	/* Now request some data from the server to see if it is really alive */
	qs->SendFindGameServerPacket(this->ms->GetQuerySocket());

	/* Register the server to the list of quering servers. */
	delete this->ms->AddQueriedServer(qs);
}

void MasterNetworkUDPSocketHandler::Receive_SERVER_UNREGISTER(Packet *p, NetworkAddress *client_addr)
{
	/* See what kind of server we have (protocol wise) */
	uint8 master_server_version = p->Recv_uint8();
	if (master_server_version < 1 || master_server_version > 2) {
		/* We do not know this version, bail out */
		DEBUG(net, 0, "received a unregistration request from %s with unknown master server version",
				client_addr->GetAddressAsString());

		return;
	}

	client_addr->SetPort(p->Recv_uint16());

	DEBUG(net, 3, "received a unregistration request from %s", client_addr->GetAddressAsString());

	/* Shouldn't happen ofcourse, but still ... */
	if (this->HasClientQuit()) return;

	QueriedServer *qs = new QueriedServer(*client_addr, this->ms->GetFrame());

	/* Remove the server from the list of online servers */
	this->ms->GetSQLBackend()->MakeServerOffline(qs);
	delete this->ms->RemoveQueriedServer(qs);
	delete qs;
}

void MasterNetworkUDPSocketHandler::Receive_CLIENT_GET_LIST(Packet *p, NetworkAddress *client_addr)
{
	uint8 master_server_version = p->Recv_uint8();
	if (master_server_version < 1 || master_server_version > 2) {
		/* We do not know this version, bail out */
		DEBUG(net, 0, "received a request for the game server list from %s with unknown master server version",
				client_addr->GetAddressAsString());

		return;
	}

	DEBUG(net, 3, "received a request for the game server list from %s", client_addr->GetAddressAsString());
	ServerListType type = SLT_IPv4;
	if (master_server_version == 2) {
		type = (ServerListType)p->Recv_uint8();
		if (type >= SLT_AUTODETECT) type = client_addr->IsFamily(AF_INET) ? SLT_IPv4 : SLT_IPv6;
	}

	for (Packet *p = this->ms->GetServerListPacket(type); p != NULL; p = p->next) {
		Packet *next = p->next;
		p->next = NULL;
		this->SendPacket(p, client_addr);
		p->next = next;
	}
}
