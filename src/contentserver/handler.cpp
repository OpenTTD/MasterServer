/* $Id$ */

#include "shared/stdafx.h"
#include "shared/debug.h"
#include "shared/network/core/core.h"
#include "shared/network/core/host.h"
#include "contentserver.h"

/**
 * @file contentserver/handler.cpp Handler of queries to the content server
 */

ContentServer::ContentServer(SQL *sql, const char *host, uint16 port) : Server(sql), first(NULL)
{
	this->listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->listen_socket == INVALID_SOCKET) {
		error("Could not bind to %s:%d; cannot open socket\n", host, port);
	}

	/* reuse the socket */
	int reuse = 1;
	/* The (const char*) cast is needed for windows!! */
	if (setsockopt(this->listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == -1) {
		error("Could not bind to %s:%d; setsockopt() failed\n", host, port);
	}

	if (!SetNonBlocking(this->listen_socket)) {
		error("Could not bind to %s:%d; could not make socket non-blocking failed\n", host, port);
	}

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = NetworkResolveHost(host);
	sin.sin_port = htons(port);

	if (bind(this->listen_socket, (struct sockaddr*)&sin, sizeof(sin)) != 0) {
		error("Could not bind to %s:%d; bind() failed\n", host, port);
	}

	if (listen(this->listen_socket, 1) != 0) {
		error("Could not bind to %s:%d; listen() failed\n", host, port);
	}
}

ContentServer::~ContentServer()
{
	closesocket(this->listen_socket);
	while (this->first != NULL) delete this->first;
}

void ContentServer::AcceptClients()
{
	for (;;) {
		struct sockaddr_in sin;
		socklen_t sin_len = sizeof(sin);
		SOCKET s = accept(this->listen_socket, (struct sockaddr*)&sin, &sin_len);
		if (s == INVALID_SOCKET) return;

		if (!SetNonBlocking(s) || !SetNoDelay(s)) return;

		new ServerNetworkContentSocketHandler(this, s, &sin);
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
			FD_SET(cs->sock, &read_fd);
			if (!cs->IsPacketQueueEmpty() || cs->HasQueue()) FD_SET(cs->sock, &write_fd);
		}

		/* take care of listener port */
		FD_SET(this->listen_socket, &read_fd);

		/* Wait for a second */
		tv.tv_sec = 1;
		tv.tv_usec = 0;
#if !defined(__MORPHOS__) && !defined(__AMIGA__)
		select(FD_SETSIZE, &read_fd, &write_fd, NULL, &tv);
#else
		WaitSelect(FD_SETSIZE, &read_fd, &write_fd, NULL, &tv, NULL);
#endif
		/* accept clients.. */
		if (FD_ISSET(this->listen_socket, &read_fd)) {
			this->AcceptClients();
		}

		/* read stuff from clients/write to them */
		for (ServerNetworkContentSocketHandler *cs = this->first; cs != NULL;) {
			if (FD_ISSET(cs->sock, &write_fd)) {
				cs->writable = true;
				cs->Send_Packets();
			}

			if (cs->HasQueue()) {
					/* Fill the queue only when the current queue is empty */
					if (cs->IsPacketQueueEmpty()) cs->SendQueue();
			} else if (FD_ISSET(cs->sock, &read_fd)) {
				/* Only receive packets when our outgoing packet queue is empty. This
				 * way we prevent internal memory overflows when people start
				 * bombarding the server with enormous requests. */
				cs->Recv_Packets();
			}

			/* Check whether we should delete the socket */
			ServerNetworkContentSocketHandler *cur_cs = cs;
			cs = cs->next;
			if (cur_cs->HasClientQuit()) delete cur_cs;
		}
	}
}

ServerNetworkContentSocketHandler::ServerNetworkContentSocketHandler(ContentServer *cs, SOCKET s, const struct sockaddr_in *sin) : NetworkContentSocketHandler(s, sin), cs(cs)
{
	this->next = cs->first;
	cs->first = this;

	this->contentQueue = NULL;
	this->curFile = NULL;
	this->packetsPerTick = 8;
}

ServerNetworkContentSocketHandler::~ServerNetworkContentSocketHandler()
{
	ServerNetworkContentSocketHandler **prev = &cs->first;
	while (*prev != this) {
		assert(*prev != NULL);
		prev = &(*prev)->next;
	}

	*prev = this->next;

	if (this->curFile != NULL) fclose(this->curFile);
	delete [] this->contentQueue;
}
