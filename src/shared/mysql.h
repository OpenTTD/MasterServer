/* $Id$ */

#ifndef MYSQL_H
#define MYSQL_H

#include "shared/sql.h"
#include "shared/mysql_data.h"

/**
 * @file mysql.h Class definition for MySQL SQL backend
 */

/** MySQL backend */
class MySQL : public SQL {
protected:
	/**
	 * Rewrites and MD5 checksum into a hexadecimal string
	 * @param grf  the GRF to get the MD5 checksum of
	 * @param dest the buffer to write to
	 * @pre sizeof(dest) >= sizeof(grf->md5sum) * 2 + 1
	 */
	void MD5sumToString(const GRFIdentifier *grf, char *dest);

	void MakeServerOnline(const char *ip, uint16 port);
	void MakeServerOffline(const char *ip, uint16 port);
	void UpdateNetworkGameInfo(const char *ip, uint16 port, const NetworkGameInfo *info);
public:
	/** Creates the connection to the SQL database */
	MySQL(const char *host, const char *user, const char *passwd, const char *db, unsigned int port);

	/** Frees all connections to the SQL database */
	~MySQL();

	uint GetActiveServers(ServerAddress result[], int length);
	uint GetRequeryServers(ServerAddress result[], int length, uint interval);
	void ResetRequeryIntervals();

	void AddGRF(const GRFIdentifier *grf);
	void SetGRFName(const GRFIdentifier *grf, const char *name);

	/**
	 * Adds quotes and such about the to be quoted string to prevent SQL injections
	 * @param dest     the string to write to
	 * @param dest_len the length of the destination string
	 * @param to_quote the string to quote
	 */
	void Quote(char *dest, size_t dest_len, const char *to_quote);
};

#endif /* MYSQL_H */
