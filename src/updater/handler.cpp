/* $Id$ */

#include "shared/stdafx.h"
#include "shared/debug.h"
#include "updater.h"

/**
 * @file updater/handler.cpp Handler of retries and requerying game servers
 */

///*** Checking for expiration of retries of servers ***///

UpdaterQueriedServer::UpdaterQueriedServer(NetworkAddress address, uint frame) : QueriedServer(address, frame)
{
	this->received_game_info = false;
}

UpdaterQueriedServer::~UpdaterQueriedServer()
{
	/* Free all GRFs we allocated */
	GRFList::iterator iter = this->missing_grfs.begin();
	for (; iter != this->missing_grfs.end(); iter++) {
		delete *iter;
	}
}

void UpdaterQueriedServer::DoAttempt(UDPServer *server)
{
	if (this->frame + UPDATER_QUERY_TIMEOUT > server->GetFrame()) return;

	/* The server did not respond in time, retry */
	this->attempts++;

	if (this->attempts > UPDATER_QUERY_ATTEMPTS) {
		/* We tried too many times already, make the server offline */

		if (!this->received_game_info) {
			DEBUG(net, 4, "[retry] too many query retries; marking %s as offline",
					this->server_address.GetAddressAsString());

			server->GetSQLBackend()->MakeServerOffline(this);
		}
		server->RemoveQueriedServer(this);
		delete this;
		return;
	}

	if (!this->received_game_info) {
		DEBUG(net, 4, "[retry] querying %s", this->server_address.GetAddressAsString());

		/* Resend game info query */
		this->SendFindGameServerPacket(server->GetQuerySocket());
	} else {
		DEBUG(net, 4, "[retry] querying grf information on %s", this->server_address.GetAddressAsString());

		/* Request the GRFs */
		this->RequestGRFs(server->GetQuerySocket());
	}

	this->frame = server->GetFrame();
}

void UpdaterQueriedServer::RequestGRFs(NetworkUDPSocketHandler *socket)
{
	if (this->missing_grfs.empty()) return;

	Packet packet(PACKET_UDP_CLIENT_GET_NEWGRFS);
	packet.Send_uint8(this->missing_grfs.size());

	GRFList::iterator iter = this->missing_grfs.begin();
	for (; iter != this->missing_grfs.end(); iter++) {
		socket->Send_GRFIdentifier(&packet, *iter);
	}

	socket->SendPacket(&packet, &this->server_address);
}

bool UpdaterQueriedServer::DoneQuerying()
{
	return this->received_game_info && this->missing_grfs.size() == 0;
}

void UpdaterQueriedServer::ReceivedGameInfo()
{
	this->received_game_info = true;
}

void UpdaterQueriedServer::ReceivedGRF(const GRFIdentifier *grf)
{
	GRFList::iterator iter = this->missing_grfs.find(grf);
	if (iter == this->missing_grfs.end()) return;

	delete *iter;
	this->missing_grfs.erase(iter);
}

void UpdaterQueriedServer::AddMissingGRF(const GRFIdentifier *grf)
{
	if (this->missing_grfs.find(grf) != this->missing_grfs.end()) return;

	this->missing_grfs.insert(new GRFIdentifier(grf));
}


Updater::Updater(SQL *sql, NetworkAddressList *addresses) : UDPServer(sql)
{
	/* We reset the requery intervals, so all servers that are marked
	 * on-line get an initial sweep on startup of the application. */
	this->sql->ResetRequeryIntervals();

	this->query_socket = new UpdaterNetworkUDPSocketHandler(this, addresses);
	if (!this->query_socket->Listen()) error("Could not bind query socket\n");
}

Updater::~Updater()
{
	/* Free all GRFs we allocated */
	GRFList::iterator iter = this->known_grfs.begin();
	for (; iter != this->known_grfs.end(); iter++) {
		delete *iter;
	}
}

bool Updater::IsGRFKnown(const GRFIdentifier *grf)
{
	bool known = this->known_grfs.find(grf) != this->known_grfs.end();

	/*
	 * If we did not know the GRF, it is a nice moment to add a stub for this
	 * GRF to the database, so when the NetworkGameInfo is added, the GRFs
	 * 'key' actually exists in the GRF table.
	 */
	if (!known) this->sql->AddGRF(grf);

	return known;
}

void Updater::MakeGRFKnown(const GRFIdentifier *grf, const char *name)
{
	if (this->IsGRFKnown(grf)) return;

	DEBUG(misc, 4, "[newgrf] found name for %08X: %s", grf->grfid, name);
	this->known_grfs.insert(new GRFIdentifier(grf));

	this->sql->SetGRFName(grf, name);
}

void Updater::CheckServers()
{
	/* First handle the requeries of sent packets */
	UDPServer::CheckServers();

	if (this->GetFrame() % UPDATER_REQUERY_INTERVAL != 0) return;

	/*
	 * Every UPDATER_REQUERY_INTERVAL seconds, we requery some
	 * servers (with a maximum of UPDATER_MAX_REQUERIED_SERVERS
	 * servers per requery interval) that need to be requeried.
	 * These servers are either new or they have not been queried
	 * for.UPDATER_SERVER_REQUERY_INTERVAL seconds.
	 */
	NetworkAddress requery[UPDATER_MAX_REQUERIED_SERVERS];
	uint count = this->sql->GetRequeryServers(requery, UPDATER_MAX_REQUERIED_SERVERS, UPDATER_SERVER_REQUERY_INTERVAL);

	for (uint i = 0; i < count; i++) {
		QueriedServer *qs = new UpdaterQueriedServer(requery[i], this->GetFrame());

		DEBUG(net, 4, "querying %s", qs->GetServerAddress()->GetAddressAsString());

		/* Send the game info query */
		qs->SendFindGameServerPacket(this->GetQuerySocket());
		this->AddQueriedServer(qs);
	}
}
