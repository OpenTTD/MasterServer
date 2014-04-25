/* $Id$ */

/*
 * This file is part of OpenTTD's master server/updater and content service.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "debug.h"
#include "string_func.h"

#include "shared/safeguards.h"
#undef vsnprintf

enum {
	BUFFER_LENGTH = 1024
};

/**
 * Prints a log to a given stream
 * @param stream the stream to write the log to
 * @param header the class of log (net, udp, misc, FATAL) etc
 * @param format format string for the list of arguments
 * @param va     list of arguments to be formatted before printed
 */
static void PrintLog(FILE *stream, const char *header, const char *format, va_list va)
{
	char buf[BUFFER_LENGTH];
	char date[32];
	time_t curtime;

	curtime = time(NULL);
	strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S GMT", gmtime(&curtime));

	vseprintf(buf, lastof(buf), format, va);
	va_end(va);

	fprintf(stream, "[%s]: [%s] %s\n", date, header, buf);
	fflush(stream);
}

int _debug_cache_level = 0;
int _debug_misc_level  = 0;
int _debug_net_level   = 0;
int _debug_sql_level   = 0;


/**
 * Information about a debug level
 */
struct DebugLevel {
	const char *name; ///< Name of the debug level
	int *level;       ///< The current level of the given debug level
};

#define DEBUG_LEVEL(x) { #x, &_debug_##x##_level }
	static const DebugLevel debug_level[] = {
	DEBUG_LEVEL(cache),
	DEBUG_LEVEL(net),
	DEBUG_LEVEL(misc),
	DEBUG_LEVEL(sql),
	};
#undef DEBUG_LEVEL

#if !defined(NO_DEBUG_MESSAGES)

/** Functionized DEBUG macro for compilers that don't support
 * variadic macros (__VA_ARGS__) such as...yes MSVC2003 and lower */
#if defined(NO_VARARG_MACRO)
void CDECL DEBUG(int name, int level, ...)
{
	va_list va;
	const char *s;
	const char *dbg;
	const DebugLevel *dl = &debug_level[name];

	if (level != 0 && *dl->level < level) return;
	dbg = dl->name;
	va_start(va, level);
#else
void CDECL debug(const char *dbg, ...)
{
	const char *s;
	va_list va;
	va_start(va, dbg);
#endif /* NO_VARARG_MACRO */
	s = va_arg(va, const char*);
	PrintLog(stdout, dbg, s, va);
	va_end(va);
}
#endif /* NO_DEBUG_MESSAGES */

void NORETURN error(const char *s, ...)
{
	va_list va;
	va_start(va, s);
	PrintLog(stderr, "FATAL", s, va);
	va_end(va);
	exit(1);
}


void SetDebugString(const char *s)
{
	int v;
	char *end;
	const char *t;

	// global debugging level?
	if (*s >= '0' && *s <= '9') {
		const DebugLevel *i;

		v = strtoul(s, &end, 0);
		s = end;

		for (i = debug_level; i != endof(debug_level); ++i) *i->level = v;
	}

	/* individual levels */
	for (;;) {
		const DebugLevel *i;
		int *p;

		/* skip delimiters */
		while (*s == ' ' || *s == ',' || *s == '\t') s++;
		if (*s == '\0') break;

		t = s;
		while (*s >= 'a' && *s <= 'z') s++;

		/* check debugging levels */
		p = NULL;
		for (i = debug_level; i != endof(debug_level); ++i)
			if (s == t + strlen(i->name) && strncmp(t, i->name, s - t) == 0) {
				p = i->level;
				break;
			}

		if (*s == '=') s++;
		v = strtoul(s, &end, 0);
		s = end;
		if (p != NULL) {
			*p = v;
		} else {
			printf("Unknown debug level '%.*s'", (int)(s - t), t);
			return;
		}
	}
}

/** Print out the current debug-level
 * Just return a string with the values of all the debug categorites
 * @return string with debug-levels
 */
const char *GetDebugString(void)
{
	const DebugLevel *i;
	static char dbgstr[100];
	char dbgval[20];

	memset(dbgstr, 0, sizeof(dbgstr));
	i = debug_level;
	seprintf(dbgstr, lastof(dbgstr), "%s=%d", i->name, *i->level);

	for (i++; i != endof(debug_level); i++) {
		seprintf(dbgval, lastof(dbgval), ", %s=%d", i->name, *i->level);
		strecat(dbgstr, dbgval, lastof(dbgstr));
	}

	return dbgstr;
}
