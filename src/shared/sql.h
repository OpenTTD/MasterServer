/* $Id$ */

#ifndef SQL_H
#define SQL_H

#include "shared/network/core/game.h"

/**
 * @file sql.h Interface for persistent storage of data into a SQL server
 */

/* Forward declare the QueriedServer, as we use it here and server.h uses SQL */
class QueriedServer;

/** Simple structure to hold the IP and port of a server in */
struct ServerAddress {
	uint32 ip;   ///< The IP part of the address
	uint16 port; ///< The port part of the address
};

/**
 * Abstract 'interface' for all SQL clients
 */
class SQL {
protected:
	/** Same as public MakeServerOnline but ip, port instead of QueriedServer */
	virtual void MakeServerOnline(const char *ip, uint16 port) = 0;
	/** Same as public MakeServerOffline but ip, port instead of QueriedServer */
	virtual void MakeServerOffline(const char *ip, uint16 port) = 0;
	/** Same as public UpdateNetworkGameInfo but ip, port instead of QueriedServer */
	virtual void UpdateNetworkGameInfo(const char *ip, uint16 port, const NetworkGameInfo *info) = 0;

	/**
	 * Adds a specific IP + port to the given index a ServerAddress list
	 * @param result table to place the IP and port in
	 * @param index  index into the result table
	 * @param ip     the IP address to add
	 * @param port   the port to add
	 */
	void AddServerAddress(ServerAddress result[], int index, const char *ip, uint16 port);
public:
	/** The obvious destructor */
	virtual ~SQL() {}

	/**
	 * Updates the necessary data structures to tell a server has come online
	 * @param qs the queried server to make online
	 */
	void MakeServerOnline(QueriedServer *qs);

	/**
	 * Updates the necessary data structures to tell a server has gone offline
	 * @param qs the queried server to make offline
	 */
	void MakeServerOffline(QueriedServer *qs);

	/**
	 * Updates the game 'state' in the database
	 * @param qs   the queried server to update the game 'state' of
	 * @param info the game state
	 * @warning none of the strings are escaped in any way; must be done if needed
	 */
	void UpdateNetworkGameInfo(QueriedServer *qs, NetworkGameInfo *info);

	/**
	 * Adds an unknown GRF to the database (we expect to get the name
	 * of the GRF soon, but there is no guarantee)
	 * @param grf the unknown GRF to add
	 */
	virtual void AddGRF(const GRFIdentifier *grf) = 0;

	/**
	 * Sets the name of an alrealy known GRF
	 * @param grf  the GRF to set the name of
	 * @param name the name of the given GRF
	 * @warning name is not escaped in any way; must be done if needed
	 */
	virtual void SetGRFName(const GRFIdentifier *grf, const char *name) = 0;

	/**
	 * Fills result with up-to length active servers.
	 * @param result table to place the active servers into
	 * @param length the length of the result table
	 * @return the number of active servers added to the list
	 */
	virtual uint GetActiveServers(ServerAddress result[], int length) = 0;

	/**
	 * Fills result with up-to length to be requeried servers servers.
	 * Also resets the to-be-requeried timer, so we will get those servers
	 * again after timeout seconds.
	 * @param result   table to place the to be requeried servers into
	 * @param length   the length of the result table
	 * @param interval the number of seconds till we have to requery the
	 *                 returned servers again.
	 * @return the number of serves to requery servers added to the list
	 */
	virtual uint GetRequeryServers(ServerAddress result[], int length, uint interval) = 0;

	/**
	 * Resets the requery timers/intervals of all servers, so they will
	 * (gradually) be requeried in the event we have restarted.
	 */
	virtual void ResetRequeryIntervals() = 0;
};

#endif /* SQL_H */
