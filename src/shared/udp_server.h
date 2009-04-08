/* $Id$ */

#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <map>
#include <vector>

#include "server.h"
#include "shared/network/core/udp.h"

/**
 * @file udp_server.h Shared udp server (master server/updater) related functionality
 */

/**
 * A queried server is a server that is already queried and is awaiting
 * reply. It has a DoAttempt that is called every frame, which can be
 * used to either retry or remove the queried server from the list of
 * waiting queried servers
 */
class QueriedServer {
protected:
	NetworkAddress server_address; ///< Address of the running game server
	uint attempts;                 ///< Number of attempts trying to reach the server
	uint frame;                    ///< Last frame we did an attempt
public:
	/**
	 * Creates a new queried server with the given address and port
	 * @param address the address of the server
	 * @param frame   the last frame of the attempt
	 */
	QueriedServer(NetworkAddress address, uint frame);

	/** The obvious destructor */
	virtual ~QueriedServer() {}

	/**
	 * Does an attempt to reach the queried server. This attempt can be skipped
	 * by the fact that we did not wait enough since the previous attempt.
	 * When we should stop attempting, false has to be returned, when it should
	 * continue with trying, true has to be returned.
	 * @param server the server we are querying for
	 */
	virtual void DoAttempt(class UDPServer *server) {}

	/**
	 * Sends the PACKET_UDP_CLIENT_FIND_SERVER packet to this queried server,
	 * using the given socket.
	 * @param socket socket to send packet on
	 */
	void SendFindGameServerPacket(NetworkUDPSocketHandler *socket);

	/**
	 * Gets the server address of this queried server
	 * @return the server address of this queried server
	 */
	NetworkAddress *GetServerAddress() { return &this->server_address; }

	/**
	 * Get the session key of the server.
	 * @return the session key of the server.
	 */
	virtual uint64 GetSessionKey() const { return 0; }
};

/** Comparator for the struct sockaddr_in's of the QueriedServerMap */
struct SockAddrInComparator
{
	/**
	 * Compare two sockaddr_in's on IP address and port
	 */
	bool operator()(NetworkAddress *s1, NetworkAddress *s2) const
	{
		return *s1 < *s2;
	}
};

/** Definition of the QueriedServerMap, which maps an socket address to a queried server */
typedef std::map<NetworkAddress*, QueriedServer*, SockAddrInComparator> QueriedServerMap;

class UDPServer : public Server {
private:
	QueriedServerMap queried_servers; ///< List of servers we have queried and are awaiting replies for

protected:
	NetworkUDPSocketHandler *query_socket; ///< Address to do queries to game servers on
	uint frame;                            ///< The current 'frame'/time in the server

	/**
	 * Function that is called approximatelly every second and is used
	 * to check every server in the queried_servers list whether they
	 * need to be requeries or not.
	 */
	virtual void CheckServers();

	/** Read packets from all the sockets */
	virtual void ReceivePackets();

	/** Function used to tell that a server has gone online/offline */
	virtual void ServerStateChange() {}

	virtual void RealRun();
public:
	/**
	 * Creates a new server with the given query socket handler
	 * @param sql          the SQL backend to read/write persistent data to
	 */
	UDPServer(SQL *sql);

	/** The obvious destructor */
	virtual ~UDPServer();

	/**
	 * Get the queried server given its address
	 * @param client_addr the address of the querying server
	 * @return the queried server, or NULL when we are not querying that server
	 */
	QueriedServer *GetQueriedServer(NetworkAddress *client_addr);

	/**
	 * Adds a server to the to-be queried servers and returns the previous
	 * QueriedServer for the same address.
	 * @param qs the new queried server
	 * @return the old queried server (or NULL) when there was none
	 */
	QueriedServer *AddQueriedServer(QueriedServer *qs);

	/**
	 * Removes the given queried server from the to-be-queried list
	 * @param qs the queried server to remove
	 * @return the removes queried server
	 */
	QueriedServer *RemoveQueriedServer(QueriedServer *qs);

	/**
	 * Gets the current frame we are in; this is the number of seconds
	 * since start up
	 */
	uint GetFrame() { return this->frame; }

	/**
	 * Gets the socket(handler) use for querying
	 * @return the socket handler
	 */
	NetworkUDPSocketHandler *GetQuerySocket() { return this->query_socket; }
};

#endif /* UDP_SERVER_H */
