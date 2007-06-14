/* $Id$ */

#include "stdafx.h"
#include "server.h"
#include "debug.h"

/**
 * @file server.cpp Shared server (master server/updater) related functionality
 */

#ifdef __AMIGA__
/* usleep() implementation */
#include <devices/timer.h>
#include <dos/dos.h>
static struct Device      *TimerBase    = NULL;
static struct MsgPort     *TimerPort    = NULL;
static struct timerequest *TimerRequest = NULL;
#endif /* __AMIGA__ */

static FILE *_log_file_fd = NULL; ///< File descriptor for the logfile
static Server *_server = NULL;    ///< The current server

#ifdef UNIX
#include <sys/signal.h>

/**
 * Handler for POSIX signals. Every incoming signal will be used
 * to terminate the application.
 * @param sig the incoming signal
 */
static void SignalHandler(int sig)
{
	_server->Stop();
	signal(sig, SignalHandler);
	DEBUG(misc, 0, "Stopping the server");
}

/**
 * Helper for forking the application
 * @param logfile          the file to send logs to when forked
 * @param application_name the name of the application
 */
static void ForkServer(const char *logfile, const char *application_name)
{
	/* Fork the program */
	pid_t pid = fork();
	switch (pid) {
		case -1:
			error("Unable to fork");

		case 0:
			/* We are the child */

			/* Open the log-file to log all stuff too */
			_log_file_fd = fopen(logfile, "a");
			if (_log_file_fd == NULL) error("Unable to open logfile");

			/* Redirect stdout and stderr to log-file */
			if (dup2(fileno(_log_file_fd), fileno(stdout)) == -1) error("Re-routing stdout");
			if (dup2(fileno(_log_file_fd), fileno(stderr)) == -1) error("Re-routing stderr");
			break;

		default:
			/* We are the parent */
			DEBUG(misc, 0, "Forked to background with pid %d\n", pid);
			exit(0);
	}
}
#endif

/**
 * Multi os compatible sleep function
 * @param milliseconds to sleep
 */
static void CSleep(int milliseconds)
{
#ifdef WIN32
	Sleep(milliseconds);
#endif /* WIN32 */
#ifdef UNIX
#if !defined(__BEOS__) && !defined(__AMIGAOS__)
	usleep(milliseconds * 1000);
#endif /* !__BEOS__ && !__AMIGAOS */
#ifdef __BEOS__
	snooze(milliseconds * 1000);
#endif /* __BEOS__ */
#if defined(__AMIGAOS__) && !defined(__MORPHOS__)
{
	ULONG signals;
	ULONG TimerSigBit = 1 << TimerPort->mp_SigBit;

	/* send IORequest */
	TimerRequest->tr_node.io_Command = TR_ADDREQUEST;
	TimerRequest->tr_time.tv_secs    = (milliseconds * 1000) / 1000000;
	TimerRequest->tr_time.tv_micro   = (milliseconds * 1000) % 1000000;
	SendIO((struct IORequest *)TimerRequest);

	if (!((signals = Wait(TimerSigBit | SIGBREAKF_CTRL_C)) & TimerSigBit) ) {
		AbortIO((struct IORequest *)TimerRequest);
	}
	WaitIO((struct IORequest *)TimerRequest);
}
#endif /* __AMIGAOS__ && !__MORPHOS__ */
#endif /* UNIX */
}

QueriedServer::	QueriedServer(uint32 address, uint16 port)
{
	this->server_address.sin_family      = AF_INET;
	this->server_address.sin_addr.s_addr = address;
	this->server_address.sin_port        = port;

	this->attempts = 0;
	this->frame    = _server->GetFrame();
}

void QueriedServer::SendFindGameServerPacket(NetworkUDPSocketHandler *socket)
{
	/* Send game info query */
	Packet packet(PACKET_UDP_CLIENT_FIND_SERVER);
	socket->SendPacket(&packet, &this->server_address);
}

Server::Server(SQL *sql, const char *host, NetworkUDPSocketHandler *query_socket)
{
	this->sql          = sql;
	this->query_socket = query_socket;
	this->frame        = 0;
	this->stop_server  = false;

	/* Launch network for a given OS */
	if (!NetworkCoreInitialize()) {
		error("Could not initialize the network");
	}

	if (!this->query_socket->Listen(inet_addr(host), 0, false)) {
		error("Could not bind to %s:0\n", host);
	}

	assert(_server == NULL);
	_server = this;
}

Server::~Server()
{
	/* Clean stuff up */
	delete this->query_socket;

	NetworkCoreShutdown();
	_server = NULL;
}

void Server::ReceivePackets()
{
	this->query_socket->ReceivePackets();
}

void Server::CheckServers()
{
	QueriedServerMap::iterator iter = this->queried_servers.begin();

	for (; iter != this->queried_servers.end();) {
		QueriedServer *srv = iter->second;
		iter++;
		srv->DoAttempt(this);
	}
}

extern const char *_revision; ///< SVN revision of this application

void Server::Run(const char *logfile, const char *application_name, bool fork)
{
	byte small_frame = 0;
	/* It is nice to know what revision we're running (shows up in the logs) */

#ifdef UNIX
	if (fork) ForkServer(logfile, application_name);

	/* Signal handlers */
	signal(SIGTERM, SignalHandler);
	signal(SIGINT,  SignalHandler);
	signal(SIGQUIT, SignalHandler);
#endif

	DEBUG(misc, 0, "Starting %s %s", application_name, _revision);

	while (!this->stop_server) {
		small_frame++;
		/* We want _frame to be in (approximatelly) seconds (not 0.1 seconds) */
		if (small_frame % 10 == 0) {
			this->frame++;
			small_frame = 0;

			/* Check if we have servers that are expired */
			this->CheckServers();
		}

		/* Check if we have any data on the socket */
		this->ReceivePackets();
		CSleep(100);
	}

	DEBUG(misc, 0, "Closing down sockets and database connection");

	if (_log_file_fd != NULL) fclose(_log_file_fd);
}

QueriedServer *Server::GetQueriedServer(const struct sockaddr_in *addr)
{
	QueriedServerMap::iterator iter = this->queried_servers.find(addr);
	if (iter == this->queried_servers.end()) return NULL;
	return iter->second;
}

QueriedServer *Server::AddQueriedServer(QueriedServer *qs)
{
	QueriedServer *ret = this->RemoveQueriedServer(qs);

	this->queried_servers[qs->GetServerAddress()] = qs;
	return ret;
}

QueriedServer *Server::RemoveQueriedServer(QueriedServer *qs)
{
	QueriedServerMap::iterator iter = this->queried_servers.find(qs->GetServerAddress());
	if (iter == this->queried_servers.end()) return NULL;

	QueriedServer *ret = iter->second;
	this->queried_servers.erase(iter);

	return ret;
}

void ParseCommandArguments(int argc, char *argv[], char *hostname, size_t hostname_length, bool *fork, const char *application_name)
{
	/* Default bind to all addresses */
	strcpy(hostname, "0.0.0.0");

	/* Check params */
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch(argv[i][1]) {
#ifdef UNIX
				case 'D': *fork = true; break;
#endif
				case 'd':
					if (i + 1 >= argc) {
						fprintf(stderr, "Missing debug level\n");
						exit(-1);
					}
					i++;
					SetDebugString(argv[i]);
					break;

				default:
					printf("\n OpenTTD %s - %s\n\n", application_name, _revision);
					printf("Usage: %s [-h] [-d level] [-D] [<bind address>]\n\n", argv[0]);
					printf(" -d [fac]=lvl[,...] set debug levels\n");
#ifdef UNIX
					printf(" -D                 fork application to background\n");
#endif
					printf(" -h                 shows this help\n");
					printf("\n");
					exit(0);
			}
		} else {
			strncpy(hostname, argv[i], hostname_length);
		}
	}
}
