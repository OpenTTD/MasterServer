/* $Id$ */

/*
 * This file is part of OpenTTD's content service.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONTENT_SERVER_H
#define CONTENT_SERVER_H

#include "shared/server.h"
#include "shared/network/core/tcp_content.h"

#include <time.h>

/**
 * @file contentserver/contentserver.h Configuration and classes used by the content server
 */

/** Timeout for idle sockets: 2 minutes. */
static const time_t IDLE_SOCKET_TIMEOUT = 60 * 2;

/**
 * Gets the monotonic time. The time will never jump back.
 * @return the time.
 * @note The following holds: time_t a = GetTime(); time_t b = GetTime(); assert(a <= b);
 */
time_t GetTime();

/* Forward declare  */
class ServerNetworkContentSocketHandler;

/**
 * The content server "serves" content to the clients. Content can be
 * NewGRFs, base graphics, AIs (including libraries) and scenarios.
 */
class ContentServer : public Server {
protected:
	friend class ServerNetworkContentSocketHandler;
	virtual void RealRun();

	/**
	 * Accept clients as long the the listen socket has some waiting
	 * @param listen_socket the socket that is accepting
	 */
	void AcceptClients(SOCKET listen_socket);

	SocketList listen_sockets;                ///< Sockets we are listening on
	ServerNetworkContentSocketHandler *first; ///< The first socket, part of linked list
public:
	/**
	 * Create a new ContentServer given an SQL connection and host
	 * @param sql the SQL server to use for persistent data storage
	 * @param address the host to bind on
	 */
	ContentServer(SQL *sql, NetworkAddressList &addresses);

	/** The obvious destructor */
	~ContentServer();
};

/** Handler for the query socket of the cs */
class ServerNetworkContentSocketHandler : public NetworkContentSocketHandler {
protected:
	friend class ContentServer;
	ContentServer *cs;                       ///< The content server associated with this socket
	ServerNetworkContentSocketHandler *next; ///< Linked list of socket handlers

	ContentInfo *contentQueue; ///< Queue of content (files) to send to the client
	FILE *contentFile;         ///< The currently read file
	uint contentFileId;        ///< The Id of the currently read file
	uint contentQueueIter;     ///< Iterator over the contentQueue
	uint contentQueueLength;   ///< Number of items in the contentQueue

	time_t last_activity;      ///< The last time this socket got any activity

	virtual bool Receive_CLIENT_INFO_LIST(Packet *p);
	virtual bool Receive_CLIENT_INFO_ID(Packet *p);
	virtual bool Receive_CLIENT_INFO_EXTID(Packet *p);
	virtual bool Receive_CLIENT_INFO_EXTID_MD5(Packet *p);
	virtual bool Receive_CLIENT_CONTENT(Packet *p);

	/**
	 * Send a number of info "structs" over the network.
	 * @param count the number to send.
	 * @param infos the information to send.
	 */
	void SendInfo(uint32 count, const ContentInfo *infos);
public:
	/**
	 * Create a new cs socket handler for a given cs
	 * @param cs the cs this socket is related to
	 * @param s  the socket we are connected with
	 * @param sin IP etc. of the client
	 */
	ServerNetworkContentSocketHandler(ContentServer *cs, SOCKET s, const NetworkAddress &sin);

	/** The obvious destructor */
	virtual ~ServerNetworkContentSocketHandler();

	/**
	 * Send the data of the content queue
	 */
	void SendQueue();

	/**
	 * Check whether we have a content queue pending.
	 * @return true if there is a queue pending.
	 */
	bool HasQueue();
};

#endif /* CONTENT_SERVER_H */
