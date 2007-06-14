/* $Id$ */

#ifndef SERVER_H
#define SERVER_H

#include <map>
#include <vector>

#include "shared/network/core/os_abstraction.h"
#include "shared/network/core/udp.h"
#include "shared/sql.h"

/**
 * @file server.h Shared server (master server/updater) related functionality
 */

/* Forward declaration as Server uses QueriedServer and vice-versa */
class Server;

/**
 * A queried server is a server that is already queried and is awaiting
 * reply. It has a DoAttempt that is called every frame, which can be
 * used to either retry or remove the queried server from the list of
 * waiting queried servers
 */
class QueriedServer {
protected:
	struct sockaddr_in server_address; ///< Address of the running game server
	uint attempts;                     ///< Number of attempts trying to reach the server
	uint frame;                        ///< Last frame we did an attempt
public:
	/**
	 * Creates a new queried server with the given address and port
	 * @param address the (sin_addr.addr) address of the server
	 * @param port    the port of the server
	 */
	QueriedServer(uint32 address, uint16 port);

	/** The obvious destructor */
	virtual ~QueriedServer() {}

	/**
	 * Does an attempt to reach the queried server. This attempt can be skipped
	 * by the fact that we did not wait enough since the previous attempt.
	 * When we should stop attempting, false has to be returned, when it should
	 * continue with trying, true has to be returned.
	 * @param server the server we are querying for
	 */
	virtual void DoAttempt(Server *server) {}

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
	struct sockaddr_in *GetServerAddress() { return &this->server_address; }
};

/** Comparator for the struct sockaddr_in's of the QueriedServerMap */
struct SockAddrInComparator
{
	/**
	 * Compare two sockaddr_in's on IP address and port
	 */
	bool operator()(const struct sockaddr_in* s1, const struct sockaddr_in* s2) const
	{
		if (s1->sin_addr.s_addr == s2->sin_addr.s_addr) return s1->sin_port < s2->sin_port;
		return s1->sin_addr.s_addr < s2->sin_addr.s_addr;
	}
};

/** Definition of the QueriedServerMap, which maps an socket address to a queried server */
typedef std::map<const struct sockaddr_in*, QueriedServer*, SockAddrInComparator> QueriedServerMap;

class Server {
private:
	QueriedServerMap queried_servers; ///< List of servers we have queried and are awaiting replies for

protected:
	SQL *sql;                              ///< SQL backend to read/write persistent data to
	uint frame;                            ///< The current 'frame'/time in the server
	bool stop_server;                      ///< Whether to stop or not
	NetworkUDPSocketHandler *query_socket; ///< Address to do queries to game servers on

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
public:
	/**
	 * Creates a new server with the given query socket handler
	 * @param sql          the SQL backend to read/write persistent data to
	 * @param host         the host to bind the socket to
	 * @param query_socket socket handler for the socket we will listen on
	 */
	Server(SQL *sql, const char *host, NetworkUDPSocketHandler *query_socket);

	/** The obvious destructor */
	virtual ~Server();

	/**
	 * Get the queried server given its address
	 * @param client_addr the address of the querying server
	 * @return the queried server, or NULL when we are not querying that server
	 */
	QueriedServer *GetQueriedServer(const struct sockaddr_in *client_addr);

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
	 * Runs the application
	 * @param logfile          the file to send logs to when forked
	 * @param application_name the name of the application
	 * @param fork             whether to fork the application or not
	 */
	void Run(const char *logfile, const char *application_name, bool fork);

	/**
	 * Signal the server to stop at the first possible moment
	 */
	void Stop() { this->stop_server = true; }

	/**
	 * Gets the current frame we are in; this is the number of seconds
	 * since start up
	 */
	uint GetFrame() { return this->frame; }

	/**
	 * Returns the SQL backend we are currently using
	 * @return the SQL backend
	 */
	SQL *GetSQLBackend() { return this->sql; }

	/**
	 * Gets the socket(handler) use for querying
	 * @return the socket handler
	 */
	NetworkUDPSocketHandler *GetQuerySocket() { return this->query_socket; }
};

/**
 * Runs the application
 * @param argc             number of arguments coming from the console
 * @param argv             arguments coming from the console
 * @param hostname         variable to fill with the console supplied hostname
 * @param hostname_length  length of the hostname array
 * @param fork             variable to tell whether the arguments imply forking
 * @param application_name the name of the application
 */
void ParseCommandArguments(int argc, char *argv[], char *hostname, size_t hostname_length, bool *fork, const char *application_name);

#endif /* SERVER_H */
