/* $Id$ */

#ifndef SQL_H
#define SQL_H

#include "shared/network/core/address.h"
#include "shared/network/core/game.h"
#include "shared/network/core/tcp_content.h"

/**
 * @file sql.h Interface for persistent storage of data into a SQL server
 */

/* Forward declare the QueriedServer, as we use it here and server.h uses SQL */
class QueriedServer;

/**
 * Abstract 'interface' for all SQL clients
 */
class SQL {
protected:
	/** Same as public MakeServerOnline but ip, port instead of QueriedServer */
	virtual void MakeServerOnline(const char *ip, uint16 port, bool ipv6, uint64 session_key) = 0;
	/** Same as public MakeServerOffline but ip, port instead of QueriedServer */
	virtual void MakeServerOffline(const char *ip, uint16 port) = 0;
	/** Same as public UpdateNetworkGameInfo but ip, port instead of QueriedServer */
	virtual void UpdateNetworkGameInfo(const char *ip, uint16 port, const NetworkGameInfo *info) = 0;
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
	 * @param ipv6 whether to get IPv6 or IPv4 addresses
	 * @return the number of active servers added to the list
	 */
	virtual uint GetActiveServers(NetworkAddress result[], int length, bool ipv6) = 0;

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
	virtual uint GetRequeryServers(NetworkAddress result[], int length, uint interval) = 0;

	/**
	 * Resets the requery timers/intervals of all servers, so they will
	 * (gradually) be requeried in the event we have restarted.
	 */
	virtual void ResetRequeryIntervals() = 0;

	/** Key where to search content with */
	enum ContentKey {
		CK_ID,           ///< Search based on the ID
		CK_UNIQUEID,     ///< Search based on the unique ID
		CK_UNIQUEID_MD5  ///< Search based on the unique ID and MD5 checksum
	};

	/**
	 * Fill the content information of the given list of infos.
	 * @param info   table to store the results in (and read the keys from)
	 * @param length the length of the table
	 * @param key    key to search the database with
	 * @param extra_data whether to acquire the complex data, such as MD5 sum or dependencies
	 * @return true if the query was succesfull, false otherwise.
	 */
	virtual bool FillContentDetails(ContentInfo info[], int length, ContentKey key, bool extra_data = true) = 0;

	/**
	 * Fill the content information of the given list of infos.
	 * @param info   table to store the results in.
	 * @param length the length of the table.
	 * @param type   the type to get a listing from.
	 * @return the number of items acquired from the database.
	 */
	virtual uint FindContentDetails(ContentInfo info[], int length, ContentType type, uint32 version) = 0;

	/**
	 * Increment the download count of the given content.
	 * @param id the ID of the content.
	 */
	virtual void IncrementDownloadCount(ContentID id) = 0;
};

#endif /* SQL_H */
