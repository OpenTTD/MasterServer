/* $Id$ */

#include "stdafx.h"
#include "debug.h"
#include "mysql.h"
#include "core/bitmath_func.hpp"
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
		DEBUG(sql, 0, "SQL Error: %s", sql);
		return NULL;
	}

	MYSQL_RES *mysql_res = mysql_store_result(_mysql);

	return mysql_res;
}


MySQL::MySQL(const char *host, const char *user, const char *passwd, const char *db, unsigned int port)
{
	_mysql = mysql_init(NULL);
	if (_mysql == NULL) error("Unable to create mysql object");

	if ((!mysql_real_connect(_mysql, host, user, passwd, db, port, NULL, 0))) {
		error("Cannot connect to MySQL: %s", mysql_error(_mysql));
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

void MySQL::MD5sumToString(const GRFIdentifier *grf, char *dest)
{
	static const char *digits = "0123456789ABCDEF";

	for (uint j = 0; j < sizeof(grf->md5sum); j++) {
		dest[j * 2    ] = digits[grf->md5sum[j] / 16];
		dest[j * 2 + 1] = digits[grf->md5sum[j] % 16];
	}
	dest[sizeof(grf->md5sum) * 2] = '\0';
}

void MySQL::MakeServerOnline(const char *ip, uint16 port)
{
	char sql[MAX_SQL_LEN];

	/* Do NOT reset the last_queried when making a server go online,
	 * as the server regularly sends an advertisement to the server.
	 * Resetting the last_queried here makes the updater update them
	 * which is pointless. New servers, and old servers that have really
	 * come online (last_queried = 0000...) are first in the queue. */
	snprintf(sql, sizeof(sql), "INSERT INTO servers SET ip='%s', port='%d', last_queried='0000-00-00 00:00:00', created=NOW(), last_online=NOW(), online='1' ON DUPLICATE KEY UPDATE online='1', last_online=NOW()", ip, port);
	MYSQL_RES *res = MySQLQuery(sql);
	if (res != NULL) mysql_free_result(res);
}

void MySQL::MakeServerOffline(const char *ip, uint16 port)
{
	char sql[MAX_SQL_LEN];

	/* Set the 'last_queried' date to a low value, so the next time
	 * the server is marked online, it will be handled with priority
	 * by the updater. */
	snprintf(sql, sizeof(sql), "UPDATE servers SET online='0', last_queried='0000-00-00 00:00:00' WHERE ip='%s' AND port='%d'", ip, port);
	MYSQL_RES *res = MySQLQuery(sql);
	if (res != NULL) mysql_free_result(res);
}

void MySQL::UpdateNetworkGameInfo(const char *ip, uint16 port, const NetworkGameInfo *info)
{
	char sql[MAX_SQL_LEN];

	/* Acquire the server_id for this server */
	snprintf(sql, sizeof(sql), "SELECT id FROM servers WHERE ip='%s' AND port='%d'", ip, port);
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
	snprintf(sql, sizeof(sql), "UPDATE servers SET last_queried=NOW(), "    \
						"online='1', last_online=NOW(), "                                 \
						"info_version='%d', name='%s', revision='%s', server_lang='%d', " \
						"use_password='%d', clients_max='%d', clients_on='%d', "          \
						"spectators_max='%d', spectators_on='%d', companies_max='%d', "   \
						"companies_on='%d', game_date='%s', start_date='%s', "            \
						"map_name='%s', map_width='%d', map_height='%d', map_set='%d', "  \
						"dedicated='%d', num_grfs='%d' WHERE id='%s'",
						info->game_info_version, safe_server_name, safe_server_revision,
						info->server_lang, info->use_password, info->clients_max,
						info->clients_on, info->spectators_max, info->spectators_on,
						info->companies_max, info->companies_on, game_date,
						start_date, safe_map_name, info->map_width, info->map_height,
						info->map_set, info->dedicated, num_grfs, server_id);

	res = MySQLQuery(sql);
	if (res != NULL) mysql_free_result(res);

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
		char md5sum[sizeof(c->md5sum) * 2 + 1];
		this->MD5sumToString(c, md5sum);

		snprintf(sql, sizeof(sql), "INSERT INTO servers_newgrfs SET server_id='%s', "
				"grfid='%u', md5sum='%s'", server_id, BSWAP32(c->grfid), md5sum);
		res = MySQLQuery(sql);

		if (res != NULL) mysql_free_result(res);
	}
}

uint MySQL::GetActiveServers(ServerAddress result[], int length)
{
	char sql[MAX_SQL_LEN];

	/* Select the online servers from database */
	snprintf(sql, sizeof(sql), "SELECT ip, port FROM servers WHERE online='1' ORDER BY revision DESC LIMIT 0,%d", length);
	MYSQL_RES *res = MySQLQuery(sql);
	if (res == NULL) return 0;

	/* The amount of advertised servers in the database */
	uint count = mysql_num_rows(res);

	for (uint i = 0; i < count; i++) {
		MYSQL_ROW row = mysql_fetch_row(res);
		this->AddServerAddress(result, i, row[0], atoi(row[1]));
	}

	mysql_free_result(res);
	return count;
}

uint MySQL::GetRequeryServers(ServerAddress result[], int length, uint interval)
{
	char sql[MAX_SQL_LEN];

	/* Select the online servers from database */
	snprintf(sql, sizeof(sql), "SELECT ip, port, id FROM servers "   \
						"WHERE online='1' AND last_queried < DATE_SUB(NOW(), " \
						"INTERVAL %d SECOND) ORDER BY last_queried LIMIT 0,%d",
						interval, length);
	MYSQL_RES *res = MySQLQuery(sql);
	if (res == NULL) return 0;

	/* The amount of advertised servers in the database */
	uint count = mysql_num_rows(res);

	for (uint i = 0; i < count; i++) {
		MYSQL_ROW row = mysql_fetch_row(res);
		this->AddServerAddress(result, i, row[0], atoi(row[1]));

		snprintf(sql, sizeof(sql),
				"UPDATE servers SET last_queried=NOW() WHERE id='%s'", row[2]);
		MYSQL_RES *res2 = MySQLQuery(sql);
		mysql_free_result(res2);
	}

	mysql_free_result(res);
	return count;
}

void MySQL::ResetRequeryIntervals()
{
	MYSQL_RES *res = MySQLQuery("UPDATE servers SET last_queried='0000-00-00 00:00:00'");
	mysql_free_result(res);
}

void MySQL::AddGRF(const GRFIdentifier *grf)
{
	char sql[MAX_SQL_LEN];
	char md5sum[sizeof(grf->md5sum) * 2 + 1];

	this->MD5sumToString(grf, md5sum);

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

	this->MD5sumToString(grf, md5sum);
	this->Quote(safe_name, sizeof(safe_name), name);

	snprintf(sql, sizeof(sql), "UPDATE newgrfs SET name='%s', unknown='0' WHERE grfid='%u' AND md5sum='%s' AND unknown='1'",
			safe_name, BSWAP32(grf->grfid), md5sum);
	MYSQL_RES *res = MySQLQuery(sql);

	if (res != NULL) mysql_free_result(res);
}
