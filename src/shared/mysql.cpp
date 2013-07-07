/* $Id$ */

/*
 * This file is part of OpenTTD's master server/updater and content service.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"
#include "debug.h"
#include "mysql.h"
#include "core/alloc_func.hpp"
#include "core/bitmath_func.hpp"
#include "core/math_func.hpp"
#include "string_func.h"
#include "date_func.h"
#include <mysql/mysql.h>
#include <string.h>

/**
 * @file mysql.cpp Implementation of the MySQL backend
 */

/** The database we are connected to */
static MYSQL *_mysql = NULL;

enum {
	MAX_SQL_LEN = 1024 ///< Maximum length of a SQL query we can perform
};

/**
 * Perform a query on the MySQL database
 * @param sql the query to perform
 * @return the result of the query
 */
MYSQL_RES *MySQLQuery(const char *sql)
{
	DEBUG(sql, 6, "Executing: %s", sql);

	int result = mysql_query(_mysql, sql);
	if (result) {
		DEBUG(sql, 0, "SQL Error: %s executing %s", mysql_error(_mysql), sql);
		return NULL;
	}

	MYSQL_RES *mysql_res = mysql_store_result(_mysql);

	return mysql_res;
}


MySQL::MySQL(const char *host, const char *user, const char *passwd, const char *db, unsigned int port)
{
	_mysql = mysql_init(NULL);
	if (_mysql == NULL) error("Unable to create mysql object");

	if (!mysql_real_connect(_mysql, host, user, passwd, db, port, NULL, 0)) {
		error("Cannot connect to MySQL: %s", mysql_error(_mysql));
	}

	bool truep = true;
	if (mysql_options(_mysql, MYSQL_OPT_RECONNECT, (const char *)&truep) != 0) {
		error("Cannot enable MySQL reconnection: %s", mysql_error(_mysql));
	}

	if (mysql_set_character_set(_mysql, "utf8")) {
		error("Cannot change character set to utf8: %s", mysql_error(_mysql));
	}

	DEBUG(sql, 1, "Connected to MySQL");
}

MySQL::~MySQL()
{
	mysql_close(_mysql);
	mysql_library_end();

	_mysql = NULL;
}

void MySQL::Quote(char *dest, size_t dest_len, const char *to_quote)
{
	/* Make sure the destination string is large enough to perform quoting
	 * and not the source string, as that goes not work. */
	if (dest == NULL || to_quote == NULL || dest == to_quote || dest_len < (2 * strlen(to_quote) + 1)) {
		if (dest != NULL) dest[0] = '\0';
		return;
	}

	mysql_real_escape_string(_mysql, dest, to_quote, strlen(to_quote));
}

void MySQL::MD5sumToString(const uint8 md5sum[16], char *dest)
{
	static const char *digits = "0123456789ABCDEF";

	for (uint j = 0; j < 16; j++) {
		dest[j * 2    ] = digits[md5sum[j] / 16];
		dest[j * 2 + 1] = digits[md5sum[j] % 16];
	}
	dest[32] = '\0';
}

void MySQL::MakeServerOnline(const char *ip, uint16 port, bool ipv6, uint64 session_key)
{
	char sql[MAX_SQL_LEN];

	/* Do NOT reset the last_queried when making a server go online,
	 * as the server regularly sends an advertisement to the server.
	 * Resetting the last_queried here makes the updater update them
	 * which is pointless. New servers, and old servers that have really
	 * come online (last_queried = 0000...) are first in the queue. */
	snprintf(sql, sizeof(sql), "CALL MakeOnline('%d', '%s', '%d', '%lld')", ipv6, ip, port, session_key);
	MYSQL_RES *res = MySQLQuery(sql);
	if (res != NULL) mysql_free_result(res);
}

void MySQL::MakeServerOffline(const char *ip, uint16 port)
{
	char sql[MAX_SQL_LEN];

	/* Set the 'last_queried' date to a low value, so the next time
	 * the server is marked online, it will be handled with priority
	 * by the updater. */
	snprintf(sql, sizeof(sql), "CALL MakeOffline('%s', '%d')", ip, port);
	MYSQL_RES *res = MySQLQuery(sql);
	if (res != NULL) mysql_free_result(res);
}

void MySQL::UpdateNetworkGameInfo(const char *ip, uint16 port, const NetworkGameInfo *info)
{
	char sql[MAX_SQL_LEN];

	/*
	 * Convert some of the variables in the NetworkGameInfo struct to strings
	 * that the database understands and are SQL-injection safe.
	 */

	/*
	 * The dates are 14 because 13 is the maximum length
	 * (7 for year, 2 for month, 2 for day and 2 separators)
	 */
	char game_date[14];
	char start_date[14];
	DateToString(info->game_date,  game_date,  sizeof(game_date));
	DateToString(info->start_date, start_date, sizeof(start_date));

	/* Count number of GRFS */
	uint num_grfs = 0;
	for (GRFConfig *c = info->grfconfig; c != NULL; c = c->next) num_grfs++;

	/* Do some SQL-injection protection; make strings large enough for Quote */
	char safe_server_name[sizeof(info->server_name) * 2];
	char safe_server_revision[sizeof(info->server_revision) * 2];
	char safe_map_name[sizeof(info->map_name) * 2];
	this->Quote(safe_server_name,     sizeof(safe_server_name),     info->server_name);
	this->Quote(safe_server_revision, sizeof(safe_server_revision), info->server_revision);
	this->Quote(safe_map_name,        sizeof(safe_map_name),        info->map_name);

	/* Do the actual update of the information */
	snprintf(sql, sizeof(sql), "SELECT UpdateGameInfo ('%s', '%d'," \
						"'%d', '%s', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', " \
						"'%d', '%s', '%s', '%s', '%d', '%d', '%d', '%d', '%d')", ip, port,
						info->game_info_version, safe_server_name, safe_server_revision,
						info->server_lang, info->use_password, info->clients_max,
						info->clients_on, info->spectators_max, info->spectators_on,
						info->companies_max, info->companies_on, game_date,
						start_date, safe_map_name, info->map_width, info->map_height,
						info->map_set, info->dedicated, num_grfs);

	MYSQL_RES *res = MySQLQuery(sql);
	if (res == NULL) return;

	if (mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return;
	}

	/*
	 * The server_id is 'just' an index in the DB and the size of the index
	 * should never exceed 15 characters.
	 */
	char server_id[16];
	snprintf(server_id, sizeof(server_id), "%s", mysql_fetch_row(res)[0]);
	mysql_free_result(res);
	if (strcmp(server_id, "0") == 0) return;

	/*
	 * TODO: is it possible to rewrite the next few lines, so we only need to
	 * refresh NewGRFs if the game server was restarted instead for every time
	 * we query the game server?
	 */

	/* Remove all GRFs, so we can add them later on */
	snprintf(sql, sizeof(sql), "DELETE FROM servers_newgrfs WHERE server_id='%s'", server_id);
	res = MySQLQuery(sql);
	if (res != NULL) mysql_free_result(res);

	/* Now add the new GRFs */
	for (GRFConfig *c = info->grfconfig; c != NULL; c = c->next) {
		char md5sum[sizeof(c->ident.md5sum) * 2 + 1];
		this->MD5sumToString(c->ident.md5sum, md5sum);

		snprintf(sql, sizeof(sql), "INSERT INTO servers_newgrfs SET server_id='%s', "
				"grfid='%u', md5sum='%s'", server_id, BSWAP32(c->ident.grfid), md5sum);
		res = MySQLQuery(sql);

		if (res != NULL) mysql_free_result(res);
	}
}

uint MySQL::GetActiveServers(NetworkAddress result[], int length, bool ipv6)
{
	char sql[MAX_SQL_LEN];

	/* Select the online servers from database */
	snprintf(sql, sizeof(sql), "SELECT ip, port FROM servers_ips WHERE online='1' AND ipv6='%d' GROUP BY server_id LIMIT 0,%d", ipv6, length);
	MYSQL_RES *res = MySQLQuery(sql);
	if (res == NULL) return 0;

	/* The amount of advertised servers in the database */
	uint count = mysql_num_rows(res);

	for (uint i = 0; i < count; i++) {
		MYSQL_ROW row = mysql_fetch_row(res);
		result[i] = NetworkAddress(row[0], atoi(row[1]));
	}

	mysql_free_result(res);
	return count;
}

uint MySQL::GetRequeryServers(NetworkAddress result[], int length, uint interval)
{
	char sql[MAX_SQL_LEN];

	/* Select the online servers from database */
	snprintf(sql, sizeof(sql), "SELECT ip, port FROM servers_ips "   \
						"WHERE online='1' AND last_queried < DATE_SUB(NOW(), " \
						"INTERVAL %d SECOND) ORDER BY last_queried LIMIT 0,%d",
						interval, length);
	MYSQL_RES *res = MySQLQuery(sql);
	if (res == NULL) return 0;

	/* The amount of advertised servers in the database */
	uint count = mysql_num_rows(res);

	for (uint i = 0; i < count; i++) {
		MYSQL_ROW row = mysql_fetch_row(res);
		result[i] = NetworkAddress(row[0], atoi(row[1]));

		snprintf(sql, sizeof(sql),
				"UPDATE servers_ips SET last_queried=NOW() WHERE ip='%s' AND port='%s'", row[0], row[1]);
		MYSQL_RES *res2 = MySQLQuery(sql);
		mysql_free_result(res2);
	}

	mysql_free_result(res);
	return count;
}

void MySQL::RemoveUnadvertised(uint interval)
{
	char sql[MAX_SQL_LEN];

	/* Select the online servers from database */
	snprintf(sql, sizeof(sql), "UPDATE servers_ips SET online='0' WHERE "   \
						"online='1' AND last_advertised < DATE_SUB(NOW(), INTERVAL %d SECOND)",
						interval);

	/* We don't really care about the result, just execute it! */
	MYSQL_RES *res = MySQLQuery(sql);
	if (res != NULL) mysql_free_result(res);
}

void MySQL::ResetRequeryIntervals()
{
	MYSQL_RES *res = MySQLQuery("UPDATE servers_ips SET last_queried='0000-00-00 00:00:00'");
	mysql_free_result(res);
}

void MySQL::AddGRF(const GRFIdentifier *grf)
{
	char sql[MAX_SQL_LEN];
	char md5sum[sizeof(grf->md5sum) * 2 + 1];

	this->MD5sumToString(grf->md5sum, md5sum);

	snprintf(sql, sizeof(sql), "INSERT IGNORE INTO newgrfs SET name='Not yet known',"
			"grfid='%u', md5sum='%s', unknown='1'", BSWAP32(grf->grfid), md5sum);
	MYSQL_RES *res = MySQLQuery(sql);

	if (res != NULL) mysql_free_result(res);
}

void MySQL::SetGRFName(const GRFIdentifier *grf, const char *name)
{
	char sql[MAX_SQL_LEN];
	char md5sum[sizeof(grf->md5sum) * 2 + 1];
	char safe_name[NETWORK_GRF_NAME_LENGTH * 2];

	this->MD5sumToString(grf->md5sum, md5sum);
	this->Quote(safe_name, sizeof(safe_name), name);

	snprintf(sql, sizeof(sql), "UPDATE newgrfs SET name='%s', unknown='0' WHERE grfid='%u' AND md5sum='%s' AND unknown='1'",
			safe_name, BSWAP32(grf->grfid), md5sum);
	MYSQL_RES *res = MySQLQuery(sql);

	if (res != NULL) mysql_free_result(res);
}

bool MySQL::FillContentDetails(ContentInfo info[], int length, ContentKey key, bool extra_data)
{
	for (int i = 0; i < length; i++) {
		char sql[MAX_SQL_LEN];
		char *p = sql;

		p += snprintf(sql, sizeof(sql), "SELECT id, name, filename, filesize, " \
				"type_id, version, url, description, uniqueid, uniquemd5 " \
				"FROM bananas_file WHERE active = 1 AND ");

		switch (key) {
			case CK_ID:
				snprintf(p, lastof(sql) - p, "id = %i", info[i].id);
				break;

			case CK_UNIQUEID:
				snprintf(p, lastof(sql) - p, "uniqueid = %u AND type_id = %i",
								info[i].unique_id, info[i].type);
				break;

			case CK_UNIQUEID_MD5:
				char md5sum[sizeof(info[i].md5sum) * 2 + 1];
				this->MD5sumToString(info[i].md5sum, md5sum);
				snprintf(p, lastof(sql) - p, "uniqueid = %u AND uniquemd5 = '%s' AND type_id = %i",
								info[i].unique_id, md5sum, info[i].type);
				break;

			default:
				return false;
		}
		MYSQL_RES *res = MySQLQuery(sql);
		if (res == NULL) return false;

		if (mysql_num_rows(res) == 0) {
			mysql_free_result(res);
			continue;
		}

		MYSQL_ROW row = mysql_fetch_row(res);
		info[i].id = (ContentID)atoi(row[0]);
		ttd_strlcpy(info[i].filename, row[2], sizeof(info[i].filename));
		info[i].filesize = atoi(row[3]);
		info[i].type = (ContentType)atoi(row[4]);

		if (extra_data) {
			ttd_strlcpy(info[i].name, row[1], sizeof(info[i].name));
			ttd_strlcpy(info[i].version, row[5], sizeof(info[i].version));
			ttd_strlcpy(info[i].url, row[6], sizeof(info[i].url));
			ttd_strlcpy(info[i].description, row[7], sizeof(info[i].description));
		}

		if (extra_data && key != CK_UNIQUEID_MD5) {
			info[i].unique_id = strtoll(row[8], NULL, 10);
			for (uint j = 0; j < 32; j++) {
				int k;
				char c = row[9][j];
				if (c <= '9') {
					k = c - '0';
				} else if (c <= 'F') {
					k = c - 'A' + 10;
				} else {
					k = c - 'a' + 10;
				}

				if (j % 2 == 0) {
					info[i].md5sum[j / 2] = k << 4;
				} else {
					info[i].md5sum[j / 2] |= k;
				}
			}
		}
		mysql_free_result(res);

		if (!extra_data) continue;

		/* Now get the tags */
		snprintf(sql, sizeof(sql), "SELECT tag.name FROM bananas_tag AS tag JOIN " \
				"bananas_file_tags AS file ON tag.id = file.tag_id WHERE " \
				"file.file_id = %u", info[i].id);
		res = MySQLQuery(sql);
		if (res == NULL) return false;

		uint rows = min(mysql_num_rows(res), 255);
		if (rows != 0) {
			info[i].tag_count = rows;
			info[i].tags = MallocT<char[32]>(rows);
			for (uint j = 0; j < rows; j++) {
				MYSQL_ROW row = mysql_fetch_row(res);
				ttd_strlcpy(info[i].tags[j], row[0], sizeof(info[i].tags[j]));
			}
		}
		mysql_free_result(res);

		/* And now get the dependencies */
		snprintf(sql, sizeof(sql), "SELECT to_file_id FROM bananas_file_deps " \
				"WHERE from_file_id = %u", info[i].id);
		res = MySQLQuery(sql);
		if (res == NULL) return false;

		rows = min(mysql_num_rows(res), 255);
		if (rows != 0) {
			info[i].dependency_count = rows;
			info[i].dependencies = MallocT<ContentID>(rows);
			for (uint j = 0; j < rows; j++) {
				MYSQL_ROW row = mysql_fetch_row(res);
				info[i].dependencies[j] = (ContentID)atoi(row[0]);
			}
		}
		mysql_free_result(res);
	}

	return true;
}

uint MySQL::FindContentDetails(ContentInfo info[], int length, ContentType type, uint32 version)
{
	char sql[MAX_SQL_LEN];

	snprintf(sql, sizeof(sql), "SELECT id FROM bananas_file " \
			"WHERE active = 1 AND published = 1 AND type_id = %i AND " \
			"minimalVersion <= %i AND " \
			"(maximalVersion = -1 OR maximalVersion >= %i)" \
			"ORDER BY uniqueid DESC LIMIT 0,%d",
			(int)type, version, version, length);

	MYSQL_RES *res = MySQLQuery(sql);
	if (res == NULL) return 0;

	/* The amount of advertised servers in the database */
	uint count = mysql_num_rows(res);

	for (uint i = 0; i < count; i++) {
		MYSQL_ROW row = mysql_fetch_row(res);
		info[i].id = (ContentID)atoi(row[0]);
	}

	mysql_free_result(res);

	return this->FillContentDetails(info, count, CK_ID, true) ? count : 0;
}

void MySQL::IncrementDownloadCount(ContentID id)
{
	char sql[MAX_SQL_LEN];

	snprintf(sql, sizeof(sql), "UPDATE bananas_file SET downloads = downloads + 1 WHERE id = %u", id);
	MYSQL_RES *res = MySQLQuery(sql);
	if (res != NULL) mysql_free_result(res);

	snprintf(sql, sizeof(sql), "INSERT DELAYED INTO bananas_download SET file_id = %u, date = NOW()", id);
	res = MySQLQuery(sql);
	if (res != NULL) mysql_free_result(res);
}
