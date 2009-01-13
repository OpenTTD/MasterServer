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

void CSleep(int milliseconds)
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

Server::Server(SQL *sql)
{
	this->sql          = sql;
	this->stop_server  = false;

	/* Launch network for a given OS */
	if (!NetworkCoreInitialize()) {
		error("Could not initialize the network");
	}

	assert(_server == NULL);
	_server = this;
}

Server::~Server()
{
	/* Clean stuff up */
	NetworkCoreShutdown();

	_server = NULL;
}

extern const char *_revision; ///< SVN revision of this application

void Server::Run(const char *logfile, const char *application_name, bool fork)
{
#ifdef UNIX
	if (fork) ForkServer(logfile, application_name);

	/* Signal handlers */
	signal(SIGTERM, SignalHandler);
	signal(SIGINT,  SignalHandler);
	signal(SIGQUIT, SignalHandler);
#endif

	/* It is nice to know what revision we're running (shows up in the logs) */
	DEBUG(misc, 0, "Starting %s %s", application_name, _revision);

	this->RealRun();

	DEBUG(misc, 0, "Closing down sockets and database connection");

	if (_log_file_fd != NULL) fclose(_log_file_fd);
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
