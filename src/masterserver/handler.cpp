/* $Id$ */

/*
 * This file is part of OpenTTD's master server.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "shared/stdafx.h"
#include "shared/debug.h"
#include "shared/mysql.h"
#include "masterserver.h"
#include <time.h>

#include "shared/safeguards.h"

/**
 * @file masterserver/handler.cpp Handler of retries and updating the server list packet sent to clients
 */

/* Requerying of game servers */

MSQueriedServer::MSQueriedServer(const NetworkAddress &query_address, const NetworkAddress &reply_address, uint64 session_key, uint frame) : QueriedServer(query_address, frame)
{
	this->reply_address = reply_address;
	this->session_key = session_key;
}

void MSQueriedServer::DoAttempt(UDPServer *server)
{
	/* Not yet waited long enough for a next attempt */
	if (this->frame + SERVER_QUERY_TIMEOUT > server->GetFrame()) return;

	/* The server did not respond in time, retry */
	this->attempts++;

	if (this->attempts > SERVER_QUERY_ATTEMPTS) {
		/* We tried too many times already */
		DEBUG(net, 4, "[retry] too many server query attempts for %s", this->server_address.GetAddressAsString());

		server->RemoveQueriedServer(this);
		delete this;
		return;
	}

	DEBUG(net, 4, "[retry] querying %s", this->server_address.GetAddressAsString());

	/* Resend query */
	this->SendFindGameServerPacket(server->GetQuerySocket());

	this->frame = server->GetFrame();
}

MasterServer::MasterServer(SQL *sql, NetworkAddressList *addresses) : UDPServer(sql)
{
	for (uint i = 0; i < SLT_END; i++) {
		this->serverlist_packet[i]        = NULL;
		this->update_serverlist_packet[i] = true;
		this->next_serverlist_frame[i]    = 0;
	}
	/* The first range of 32+16 bits (IPv4 + port) needs to be free for
	 * backward compatability. As currently time already is beyond 2^31,
	 * we only need 17 more 'bits'. The other 3 are to make the session
	 * keys distinguisable between 'old' and new. */
	this->session_key              = time(NULL) << 20;
	srandom(this->session_key);

	this->master_socket = new MasterNetworkUDPSocketHandler(this, addresses);

	/* Bind master socket*/
	if (!this->master_socket->Listen()) error("Could not bind listening socket\n");

	for (NetworkAddress *addr = addresses->Begin(); addr != addresses->End(); addr++) {
		addr->SetPort(0);
	}

	this->query_socket = new QueryNetworkUDPSocketHandler(this, addresses);
	if (!this->query_socket->Listen()) error("Could not bind query socket\n");
}

MasterServer::~MasterServer()
{
	delete this->master_socket;
	for (uint i = 0; i < SLT_END; i++) {
		delete this->serverlist_packet[i];
	}
}

void MasterServer::SendAck(MSQueriedServer *qs)
{
	Packet packet(PACKET_UDP_MASTER_ACK_REGISTER);
	this->master_socket->SendPacket(&packet, qs->GetReplyAddress());
}

void MasterServer::ReceivePackets()
{
	UDPServer::ReceivePackets();
	this->master_socket->ReceivePackets();
}

uint64 MasterServer::NextSessionKey()
{
	this->session_key += 1 + (random() & 0xFF);
	return this->session_key;
}

/**
 * Helper to create a packet with addresses.
 * @param servers The server addresses to send.
 * @param count   The amount of servers to send.
 * @param type    The type of address to send.
 * @return The just created packet with addresses.
 */
Packet *CreateAddressPacket(NetworkAddress *servers, int count, ServerListType type)
{
	Packet *p = new Packet(PACKET_UDP_MASTER_RESPONSE_LIST);
	p->Send_uint8(type + 1);
	/* Fill the packet */
	p->Send_uint16(count);
	for (int i = 0; i < count; i++) {
		if (type == SLT_IPv6) {
			byte *addr = (byte*)&((sockaddr_in6*)servers[i].GetAddress())->sin6_addr;
			for (uint i = 0; i < sizeof(in6_addr); i++) p->Send_uint8(*addr++);
		} else {
			p->Send_uint32(((sockaddr_in*)servers[i].GetAddress())->sin_addr.s_addr);
		}
		p->Send_uint16(servers[i].GetPort());
	}

	return p;
}

/**
 * Returns the packets with the game server list. This packet will
 * be updated/regenerated whenever needed, i.e. when we know a server
 * went online/offline or after a timeout as the updater can change
 * the state of a server too.
 * @param type the type of addresses to return.
 * @return the serverlist packet
 * @post return != NULL
 */
Packet *MasterServer::GetServerListPacket(ServerListType type)
{
	/* Rebuild the packet only once in a while */
	if (this->update_serverlist_packet[type] || this->next_serverlist_frame[type] < this->frame) {
		DEBUG(net, 4, "[server list] rebuilding the IPv%d server list", 4 + type * 2);
		NetworkAddress servers[1024];
		uint16 count = this->sql->GetActiveServers(servers, lengthof(servers), type == SLT_IPv6);

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
		 *
		 * For IPv6 addresses we send an in6_addr and for IPv4 address an in_addr.
		 */
		static const uint16 max_count[SLT_END] = {
			(SAFE_MTU - sizeof(PacketSize) - sizeof(PacketType) - sizeof(count)) / (sizeof(in_addr) + sizeof(uint16)),
			(SAFE_MTU - sizeof(PacketSize) - sizeof(PacketType) - sizeof(count)) / (sizeof(in6_addr) + sizeof(uint16))
		};

		Packet *p = this->serverlist_packet[type];
		while (p != NULL) {
			Packet *cur = p;
			p = p->next;
			delete cur;
		}
		this->serverlist_packet[type] = NULL;

		NetworkAddress *cur_servers = servers;
		do {
			uint cur_count = min(count, max_count[type]);
			Packet *p = CreateAddressPacket(cur_servers, cur_count, type);
			count -= cur_count;
			cur_servers += cur_count;

			p->next = this->serverlist_packet[type];
			this->serverlist_packet[type] = p;
		} while (count > 0);

		/* Schedule the next retry */
		this->next_serverlist_frame[type]    = this->frame + GAME_SERVER_LIST_AGE;
		this->update_serverlist_packet[type] = false;
	}

	return this->serverlist_packet[type];
}
