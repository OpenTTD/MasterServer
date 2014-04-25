/* $Id$ */

/*
 * This file is part of OpenTTD's master server list updater.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "shared/stdafx.h"
#include "shared/debug.h"
#include "shared/string_func.h"
#include "updater.h"

#include "shared/safeguards.h"

/**
 * @file updater/udp.cpp Handler of incoming UDP packets for the updater
 */

void UpdaterNetworkUDPSocketHandler::Receive_SERVER_RESPONSE(Packet *p, NetworkAddress *client_addr)
{
	UpdaterQueriedServer *qs = this->updater->GetQueriedServer(client_addr);

	/* We were NOT waiting for this server.. drop it */
	if (qs == NULL) {
		DEBUG(net, 0, "received an unexpected 'server response' from %s",
				client_addr->GetAddressAsString());
		return;
	}
	DEBUG(net, 3, "received a 'server response' from %s",
			client_addr->GetAddressAsString());

	/* Make certain that everything is zero-ed, as some values have to
	 * be zero for 'older' versions of the game info packet. */
	NetworkGameInfo info;
	memset(&info, 0, sizeof(info));

	this->current_qs = qs;
	this->ReceiveNetworkGameInfo(p, &info);
	this->current_qs = NULL;

	/* Shouldn't happen ofcourse, but still ... */
	if (this->HasClientQuit()) {
		delete this->updater->RemoveQueriedServer(qs);
		return;
	}

	/* Update the internal as well as the persistent state of the game server */
	qs->ReceivedGameInfo();
	this->updater->GetSQLBackend()->UpdateNetworkGameInfo(qs, &info);

	for (GRFConfig *c = info.grfconfig; c != NULL;) {
		GRFConfig *next = c->next;
		delete c;
		c = next;
	}

	/* We might still be waiting for NewGRF replies */
	if (qs->DoneQuerying()) {
		delete this->updater->RemoveQueriedServer(qs);
	} else {
		qs->RequestGRFs(this);
	}
}

void UpdaterNetworkUDPSocketHandler::Receive_SERVER_NEWGRFS(Packet *p, NetworkAddress *client_addr)
{
	UpdaterQueriedServer *qs = this->updater->GetQueriedServer(client_addr);

	/* We were NOT waiting for this server.. drop it */
	if (qs == NULL) {
		DEBUG(net, 0, "received an unexpected 'newgrf response' from %s",
				client_addr->GetAddressAsString());
		return;
	}

	DEBUG(net, 3, "received a 'newgrf response' from %s",
			client_addr->GetAddressAsString());
	uint8 num_grfs = p->Recv_uint8();
	if (num_grfs > NETWORK_MAX_GRF_COUNT) return;

	for (; num_grfs > 0; num_grfs--) {
		char name[NETWORK_GRF_NAME_LENGTH];
		GRFIdentifier grf;

		this->ReceiveGRFIdentifier(p, &grf);
		p->Recv_string(name, sizeof(name));

		/* An empty name is not possible under normal circumstances
		 * but it is not OK, so drop this entry. */
		if (StrEmpty(name)) continue;

		qs->ReceivedGRF(&grf);
		this->updater->MakeGRFKnown(&grf, name);
	}

	/* We might still be waiting for NewGRF replies */
	if (qs->DoneQuerying()) {
		delete this->updater->RemoveQueriedServer(qs);
	}
}

void UpdaterNetworkUDPSocketHandler::HandleIncomingNetworkGameInfoGRFConfig(GRFConfig *config)
{
	/* If we do not know it, get the name of it */
	if (!this->updater->IsGRFKnown(&config->ident)) this->current_qs->AddMissingGRF(&config->ident);
}
