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
	while (this->first != NULL) delete this->first;
}

void ContentServer::AcceptClients(SOCKET listen_socket)
{
	for (;;) {
		struct sockaddr_storage sin;
		socklen_t sin_len = sizeof(sin);
		SOCKET s = accept(listen_socket, (struct sockaddr*)&sin, &sin_len);
		if (s == INVALID_SOCKET) return;

		if (!SetNonBlocking(s) || !SetNoDelay(s)) return;

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
			if (!cs->IsPacketQueueEmpty() || cs->HasQueue()) {
				FD_SET(cs->sock, &write_fd);
			} else {
				FD_SET(cs->sock, &read_fd);
			}
		}

		/* take care of listener port */
		for (SocketList::iterator s = listen_sockets.Begin(); s != listen_sockets.End(); s++) {
			FD_SET(s->second, &read_fd);
		}

		/* Wait for a second */
		tv.tv_sec = 1;
		tv.tv_usec = 0;
#if !defined(__MORPHOS__) && !defined(__AMIGA__)
		select(FD_SETSIZE, &read_fd, &write_fd, NULL, &tv);
#else
		WaitSelect(FD_SETSIZE, &read_fd, &write_fd, NULL, &tv, NULL);
#endif
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
			if (FD_ISSET(cs->sock, &write_fd)) {
				cs->writable = true;
				cs->last_activity = GetTime();

				while (cs->Send_Packets() && cs->IsPacketQueueEmpty() && cs->HasQueue()) {
					cs->SendQueue();
				}
			} else if (FD_ISSET(cs->sock, &read_fd)) {
				/* Only receive packets when our outgoing packet queue is empty. This
				 * way we prevent internal memory overflows when people start
				 * bombarding the server with enormous requests. */
				cs->Recv_Packets();
			} else if (cs->last_activity < time) {
				DEBUG(misc, 0, "Killing idle connection");
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

ServerNetworkContentSocketHandler::ServerNetworkContentSocketHandler(ContentServer *cs, SOCKET s, NetworkAddress sin) : NetworkContentSocketHandler(s, sin), cs(cs)
{
	this->next = cs->first;
	cs->first = this;

	this->contentQueue = NULL;

	this->last_activity = GetTime();
}

ServerNetworkContentSocketHandler::~ServerNetworkContentSocketHandler()
{
	ServerNetworkContentSocketHandler **prev = &cs->first;
	while (*prev != this) {
		assert(*prev != NULL);
		prev = &(*prev)->next;
	}

	*prev = this->next;

	delete [] this->contentQueue;
}
