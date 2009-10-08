/* $Id$ */

/*
 * This file is part of OpenTTD's master server list updater.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UPDATER_H
#define UPDATER_H

#include <set>
#include "shared/udp_server.h"

/**
 * @file updater/updater.h Configuration and classes used by the updater
 */

/**
 * Some configuration constants
 */
enum {
	UPDATER_QUERY_TIMEOUT              =       5, ///< How many frames it takes for a server to time out
	UPDATER_QUERY_ATTEMPTS             =       3, ///< How many times do we try to query?

	UPDATER_SERVER_REQUERY_INTERVAL    =  5 * 60, ///< How often do we requery servers in seconds
	UPDATER_SERVER_UNADVERTISE_TIMEOUT = 20 * 60, ///< How long it takes before marking a server not advertising; advertising interval (~15 minutes) + 5 minutes (server clock delays etc)
	UPDATER_REQUERY_INTERVAL           =       5, ///< How often do we check whether to requery servers
	UPDATER_UNADVERTISE_INTERVAL       =  1 * 60, ///< How often do we check for unadvertised servers
	UPDATER_MAX_REQUERIED_SERVERS      =      32, ///< How many servers do we (maximally) requery in one interval
};

/** List/set of GRFIdentifiers */
typedef std::set<const GRFIdentifier*, GRFComparator> GRFList;

/**
 * An UpdaterQueriedServer is a server for which we are getting the current
 * state of the game from and/or the names of the NewGRFs used.
 */
class UpdaterQueriedServer : public QueriedServer {
private:
	bool received_game_info; ///< Whether we have received the 'NetworkGameInfo'
	GRFList missing_grfs;    ///< GRFs we are missing the name of
public:
	/**
	 * Creates a new UpdaterQueriedServer for the server identifier by
	 * the given address and port
	 * @param address the address of the server
	 * @param frame time of last try
	 */
	UpdaterQueriedServer(NetworkAddress address, uint frame);

	/** The obvious destructor */
	~UpdaterQueriedServer();

	/**
	 * Checks whether it is time to retry, does that if needed
	 * @param server the server to send all queries and such to
	 */
	void DoAttempt(UDPServer *server);

	/**
	 * Makes and sends the request for the NewGRFs we are missing the
	 * name of.
	 * @param socket the socket to send the packet on
	 */
	void RequestGRFs(NetworkUDPSocketHandler *socket);

	/**
	 * Whether we have received both the NetworkGameInfo as the names of the GRFs
	 */
	bool DoneQuerying();

	/** Tell that we received the NetworkGameInfo */
	void ReceivedGameInfo();

	/**
	 * Marks the given GRF as being not missing anymore (no need to search for it)
	 * @param grf the GRF that has been found
	 */
	void ReceivedGRF(const GRFIdentifier *grf);

	/**
	 * Adds a GRF to the list of GRFs to get the name of
	 * @param grf the GRF to find the name of
	 */
	void AddMissingGRF(const GRFIdentifier *grf);
};

/**
 * The updater periodically checks all game servers to check whether they
 * are still online and update their game information for the website.
 */
class Updater : public UDPServer {
protected:
	GRFList known_grfs; ///< The NewGRFs we have the name of
public:
	/**
	 * Create a new Updater given an SQL connection and host
	 * @param sql       the SQL server to use for persistent data storage
	 * @param addresses the host to bind on
	 */
	Updater(SQL *sql, NetworkAddressList *addresses);

	/** The obvious destructor */
	~Updater();

	/**
	 * Checks for the retries _and_ checks whether other servers
	 * should get their information updated too
	 */
	void CheckServers();

	/**
	 * Whether we know the name of the particular GRF or not
	 * @param grf the GRF ID
	 * @return true if and only if we know the name of the GRF
	 */
	bool IsGRFKnown(const GRFIdentifier *grf);

	/**
	 * Make the given GRF known under the given name
	 * @param grf  the grf to be made known
	 * @param name the name of the GRF
	 */
	void MakeGRFKnown(const GRFIdentifier *grf, const char *name);

	UpdaterQueriedServer *GetQueriedServer(NetworkAddress *client_addr) { return (UpdaterQueriedServer*)UDPServer::GetQueriedServer(client_addr); }
};

/** Handler for the query socket of the updater */
class UpdaterNetworkUDPSocketHandler : public NetworkUDPSocketHandler {
protected:
	UpdaterQueriedServer *current_qs; ///< The query server currently receiving data for
	Updater *updater;                 ///< The updater associated with this socket
	DECLARE_UDP_RECEIVE_COMMAND(PACKET_UDP_SERVER_RESPONSE); ///< Handle a PACKET_UDP_SERVER_RESPONSE packet
	DECLARE_UDP_RECEIVE_COMMAND(PACKET_UDP_SERVER_NEWGRFS); ///< Handle a PACKET_UDP_SERVER_NEWGRFS packet

	/**
	 * Callback function use to check existance and such of a GRF
	 * @param config the GRF to handle
	 */
	void HandleIncomingNetworkGameInfoGRFConfig(GRFConfig *config);
public:
	/**
	 * Create a new updater socket handler for a given updater
	 * @param updater the updater this socket is related to
	 * @param addresses the host to bind on
	 */
	UpdaterNetworkUDPSocketHandler(Updater *updater, NetworkAddressList *addresses) :
		NetworkUDPSocketHandler(addresses),
		updater(updater)
	{}

	/** The obvious destructor */
	virtual ~UpdaterNetworkUDPSocketHandler() {}
};

#endif /* UPDATER_H */
