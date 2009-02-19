/* $Id$ */

#ifndef CONTENT_SERVER_H
#define CONTENT_SERVER_H

#include "shared/server.h"
#include "shared/network/core/tcp_content.h"

/**
 * @file contentserver/contentserver.h Configuration and classes used by the content server
 */

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

	/** Accept clients as long the the listen socket has some waiting */
	void AcceptClients();

	SOCKET listen_socket;                     ///< Socket we are listening on
	ServerNetworkContentSocketHandler *first; ///< The first socket, part of linked list
public:
	/**
	 * Create a new ContentServer given an SQL connection and host
	 * @param sql  the SQL server to use for persistent data storage
	 * @param host the host to bind on
	 * @param port the port to bind on
	 */
	ContentServer(SQL *sql, const char *host, uint16 port);

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
	uint contentQueueIter;     ///< Iterator over the contentQueue
	uint contentQueueLength;   ///< Number of items in the contentQueue

	DECLARE_CONTENT_RECEIVE_COMMAND(PACKET_CONTENT_CLIENT_INFO_LIST);
	DECLARE_CONTENT_RECEIVE_COMMAND(PACKET_CONTENT_CLIENT_INFO_ID);
	DECLARE_CONTENT_RECEIVE_COMMAND(PACKET_CONTENT_CLIENT_INFO_EXTID);
	DECLARE_CONTENT_RECEIVE_COMMAND(PACKET_CONTENT_CLIENT_INFO_EXTID_MD5);
	DECLARE_CONTENT_RECEIVE_COMMAND(PACKET_CONTENT_CLIENT_CONTENT);

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
	ServerNetworkContentSocketHandler(ContentServer *cs, SOCKET s, const struct sockaddr_in *sin);

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
