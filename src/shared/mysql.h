/* $Id$ */

/*
 * This file is part of OpenTTD's master server/updater and content service.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

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
	 * @param md5sum the MD5 checksum
	 * @param dest   the buffer to write to
	 */
	void MD5sumToString(const uint8 md5sum[16], char *dest);

	void MakeServerOnline(const char *ip, uint16 port, bool ipv6, uint64 session_key);
	void MakeServerOffline(const char *ip, uint16 port);
	void UpdateNetworkGameInfo(const char *ip, uint16 port, const NetworkGameInfo *info);
public:
	/** Creates the connection to the SQL database */
	MySQL(const char *host, const char *user, const char *passwd, const char *db, unsigned int port);

	/** Frees all connections to the SQL database */
	~MySQL();

	uint GetActiveServers(NetworkAddress result[], int length, bool ipv6);
	uint GetRequeryServers(NetworkAddress result[], int length, uint interval);
	void ResetRequeryIntervals();

	void AddGRF(const GRFIdentifier *grf);
	void SetGRFName(const GRFIdentifier *grf, const char *name);

	bool FillContentDetails(ContentInfo info[], int length, ContentKey key, bool extra_data);
	uint FindContentDetails(ContentInfo info[], int length, ContentType type, uint32 version);
	void IncrementDownloadCount(ContentID id);

	/**
	 * Adds quotes and such about the to be quoted string to prevent SQL injections
	 * @param dest     the string to write to
	 * @param dest_len the length of the destination string
	 * @param to_quote the string to quote
	 */
	void Quote(char *dest, size_t dest_len, const char *to_quote);
};

#endif /* MYSQL_H */
