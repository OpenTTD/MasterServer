/* $Id$ */

#include "shared/stdafx.h"
#include "shared/debug.h"
#include "shared/mysql.h"
#include "masterserver.h"

/**
 * @file masterserver/handler.cpp Handler of retries and updating the server list packet sent to clients
 */

/* Requerying of game servers */

MSQueriedServer::MSQueriedServer(const sockaddr_in *query_address, uint16 server_port) : QueriedServer(query_address->sin_addr.s_addr, server_port)
{
	this->query_address = *(sockaddr_in*)query_address;
}

void MSQueriedServer::DoAttempt(Server *server)
{
	/* Not yet waited long enough for a next attempt */
	if (this->frame + SERVER_QUERY_TIMEOUT > server->GetFrame()) return;

	/* The server did not respond in time, retry */
	this->attempts++;

	if (this->attempts > SERVER_QUERY_ATTEMPTS) {
		/* We tried too many times already */
		DEBUG(net, 4, "[retry] too many server query attempts for %s:%d", inet_ntoa(this->server_address.sin_addr), ntohs(this->server_address.sin_port));

		server->RemoveQueriedServer(this);
		delete this;
		return;
	}

	DEBUG(net, 4, "[retry] querying %s:%d", inet_ntoa(this->server_address.sin_addr), ntohs(this->server_address.sin_port));

	/* Resend query */
	this->SendFindGameServerPacket(server->GetQuerySocket());

	this->frame = server->GetFrame();
}

MasterServer::MasterServer(SQL *sql, const char *host, uint16 master_port) : Server(sql, host, new QueryNetworkUDPSocketHandler(this))
{
	this->serverlist_packet        = NULL;
	this->update_serverlist_packet = true;
	this->next_serverlist_frame    = 0;

	this->master_socket = new MasterNetworkUDPSocketHandler(this);

	/* Bind master socket*/
	if (!this->master_socket->Listen(inet_addr(host), NETWORK_MASTER_SERVER_PORT, false)) {
		error("Could not bind to %s:%d\n", host, NETWORK_MASTER_SERVER_PORT);
	}
}

MasterServer::~MasterServer()
{
	delete this->master_socket;
	delete this->serverlist_packet;
}

void MasterServer::ReceivePackets()
{
	Server::ReceivePackets();
	this->master_socket->ReceivePackets();
}

/**
 * Returns the packets with the game server list. This packet will
 * be updated/regenerated whenever needed, i.e. when we know a server
 * went online/offline or after a timeout as the updater can change
 * the state of a server too.
 * @return the serverlist packet
 * @post return != NULL
 */
Packet *MasterServer::GetServerListPacket()
{
	/* Rebuild the packet only once in a while */
	if (this->update_serverlist_packet || this->next_serverlist_frame < this->frame) {
		uint16 count;

		/*
		 * Due to the limited size of the packet, and the fact that we only send
		 * one packet with advertised servers, we have to limit the amount of
		 * servers we can put into the packet.
		 *
		 * For this we use the maximum size of the packet, substract the bytes
		 * needed for the PacketSize, PacketType information (as in all packets)
		 * and the count of servers. This gives the number of bytes we can use
		 * to place the advertised servers in. Which is then divided by the
		 * amount of bytes needed to encode the IP address and the port of a
		 * single server.
		 */
		static const uint16 max_count = (sizeof(this->serverlist_packet->buffer) - sizeof(PacketSize) - sizeof(PacketType) - sizeof(count)) / (sizeof(uint32) + sizeof(uint16));

		DEBUG(net, 4, "[server list] rebuilding the server list");

		delete this->serverlist_packet;
		this->serverlist_packet = NetworkSend_Init(PACKET_UDP_MASTER_RESPONSE_LIST);
		this->serverlist_packet->Send_uint8(NETWORK_MASTER_SERVER_VERSION);

		ServerAddress servers[max_count];
		count = this->sql->GetActiveServers(servers, max_count);

		/* Fill the packet */
		this->serverlist_packet->Send_uint16(count);
		for (int i = 0; i < count; i++) {
			this->serverlist_packet->Send_uint32(servers[i].ip);
			this->serverlist_packet->Send_uint16(servers[i].port);
		}

		/* Schedule the next retry */
		this->next_serverlist_frame    = this->frame + GAME_SERVER_LIST_AGE;
		this->update_serverlist_packet = false;
	}

	return this->serverlist_packet;
}
