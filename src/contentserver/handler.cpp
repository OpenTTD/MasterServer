/* $Id$ */

/*
 * This file is part of OpenTTD's content service.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "shared/stdafx.h"
#include "shared/debug.h"
#include "shared/network/core/core.h"
#include "contentserver.h"

/**
 * @file contentserver/handler.cpp Handler of queries to the content server
 */

time_t GetTime()
{
	struct timespec ts;
	int ret = clock_gettime(CLOCK_MONOTONIC, &ts);
	assert(ret == 0);
	return ts.tv_sec;
}


ContentServer::ContentServer(SQL *sql, NetworkAddressList &addresses) : Server(sql), first(NULL)
{
	for (NetworkAddress *address = addresses.Begin(); address != addresses.End(); address++) {
		address->Listen(SOCK_STREAM, &this->listen_sockets);
	}

	if (this->listen_sockets.Length() == 0) error("Could not bind.");
}

ContentServer::~ContentServer()
{
	for (SocketList::iterator s = listen_sockets.Begin(); s != listen_sockets.End(); s++) {
		closesocket(s->second);
	}

	/* Normally ~ServerNetworkContentSocketHandler updates first, but this is slightly more efficient. */
	while (this->first != NULL) {
		ServerNetworkContentSocketHandler *cur = this->first;
		this->first = this->first->next;

		cur->cs = NULL;
		delete cur;
	}
}

void ContentServer::AcceptClients(SOCKET listen_socket)
{
	for (;;) {
		struct sockaddr_storage sin;
		socklen_t sin_len = sizeof(sin);
		SOCKET s = accept(listen_socket, (struct sockaddr*)&sin, &sin_len);
		if (s == INVALID_SOCKET) return;

		if (!SetNonBlocking(s) || !SetNoDelay(s)) {
			closesocket(s);
			return;
		}

		new ServerNetworkContentSocketHandler(this, s, NetworkAddress(sin, sin_len));
	}
}

void ContentServer::RealRun()
{
	while (!this->stop_server) {
		fd_set read_fd, write_fd;
		struct timeval tv;

		FD_ZERO(&read_fd);
		FD_ZERO(&write_fd);

		for (ServerNetworkContentSocketHandler *cs = this->first; cs != NULL; cs = cs->next) {
			if (cs->HasQueue() || cs->HasSendQueue()) FD_SET(cs->sock, &write_fd);
			FD_SET(cs->sock, &read_fd);
		}

		/* take care of listener port */
		for (SocketList::iterator s = listen_sockets.Begin(); s != listen_sockets.End(); s++) {
			FD_SET(s->second, &read_fd);
		}

		/* Wait for a second */
		tv.tv_sec = 1;
		tv.tv_usec = 0;
#if !defined(__MORPHOS__) && !defined(__AMIGA__)
		int ret = select(FD_SETSIZE, &read_fd, &write_fd, NULL, &tv);
#else
		int ret = WaitSelect(FD_SETSIZE, &read_fd, &write_fd, NULL, &tv, NULL);
#endif
		if (ret < 0) {
			/* Try again later. */
			static int warned = false;
			if (!warned) {
				DEBUG(misc, 0, "select returned an error condition: %i", GET_LAST_ERROR());
				warned = true;
			}

			CSleep(1);
			continue;
		}

		/* accept clients.. */
		/* take care of listener port */
		for (SocketList::iterator s = listen_sockets.Begin(); s != listen_sockets.End(); s++) {
			if (FD_ISSET(s->second, &read_fd)) {
				this->AcceptClients(s->second);
			}
		}

		time_t time = GetTime() - IDLE_SOCKET_TIMEOUT;

		/* read stuff from clients/write to them */
		for (ServerNetworkContentSocketHandler *cs = this->first; cs != NULL;) {
			SendPacketsState sps = cs->HasQueue() ? SPS_NONE_SENT : SPS_ALL_SENT;
			if (FD_ISSET(cs->sock, &write_fd)) {
				cs->writable = true;

				while ((sps = cs->SendPackets()) == SPS_ALL_SENT && cs->HasQueue()) {
					cs->SendQueue();
				}
				cs->last_activity = GetTime();
			}

			if (sps == SPS_ALL_SENT && FD_ISSET(cs->sock, &read_fd)) {
				/* Only receive packets when our outgoing packet queue is empty. This
				 * way we prevent internal memory overflows when people start
				 * bombarding the server with enormous requests. */
				cs->ReceivePackets();
			} else if (cs->last_activity < time) {
				DEBUG(misc, 1, "Killing idle connection");
				cs->Close();
			}

			/* Check whether we should delete the socket */
			ServerNetworkContentSocketHandler *cur_cs = cs;
			cs = cs->next;
			if (cur_cs->HasClientQuit()) delete cur_cs;
		}

		CSleep(1);
	}
}

ServerNetworkContentSocketHandler::ServerNetworkContentSocketHandler(ContentServer *cs, SOCKET s, const NetworkAddress &sin) : NetworkContentSocketHandler(s, sin), cs(cs)
{
	this->next = cs->first;
	cs->first = this;

	this->contentQueue       = NULL;
	this->contentFile        = NULL;
	this->contentFileId      = 0;
	this->contentQueueIter   = 0;
	this->contentQueueLength = 0;

	this->last_activity = GetTime();
}

ServerNetworkContentSocketHandler::~ServerNetworkContentSocketHandler()
{
	if (this->cs != NULL) {
		ServerNetworkContentSocketHandler **prev = &this->cs->first;
		while (*prev != this) {
			assert(*prev != NULL);
			prev = &(*prev)->next;
		}

		*prev = this->next;
	}

	if (this->contentFile != NULL) fclose(this->contentFile);
	this->contentFile = NULL;

	delete [] this->contentQueue;
}
