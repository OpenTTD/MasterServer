/* $Id$ */

#ifndef STRING_H
#define STRING_H

#include "macros.h"

/*
 * dst: destination buffer
 * src: string to copy/concatenate
 * size: size of the destination buffer
 * usage: ttd_strlcpy(dst, src, lengthof(dst));
 */
void ttd_strlcat(char *dst, const char *src, size_t size);
void ttd_strlcpy(char *dst, const char *src, size_t size);

/*
 * dst: destination buffer
 * src: string to copy
 * last: pointer to the last element in the dst array
 *       if NULL no boundary check is performed
 * returns a pointer to the terminating \0 in the destination buffer
 * usage: strecpy(dst, src, lastof(dst));
 */
char* strecat(char* dst, const char* src, const char* last);
char* strecpy(char* dst, const char* src, const char* last);

char* CDECL str_fmt(const char* str, ...);

/** Scans the string for valid characters and if it finds invalid ones,
 * replaces them with a question mark '?' */
void str_validate(char *str);

/** Convert the given string to lowercase, only works with ASCII! */
void strtolower(char *str);

static inline bool StrEmpty(const char* s) { return s[0] == '\0'; }

/** Get the length of a string, within a limited buffer */
static inline int ttd_strnlen(const char *str, int maxlen)
{
	const char *t;
	for (t = str; *t != '\0' && t - str < maxlen; t++) {}
	return t - str;
}


typedef uint32 WChar;

size_t Utf8Decode(WChar *c, const char *s);
size_t Utf8Encode(char *buf, WChar c);


static inline WChar Utf8Consume(const char **s)
{
	WChar c;
	*s += Utf8Decode(&c, *s);
	return c;
}


/** Return the length of a UTF-8 encoded character.
 * @param c Unicode character.
 * @return Length of UTF-8 encoding for character.
 */
static inline size_t Utf8CharLen(WChar c)
{
	if (c < 0x80)       return 1;
	if (c < 0x800)      return 2;
	if (c < 0x10000)    return 3;
	if (c < 0x110000)   return 4;

	/* Invalid valid, we encode as a '?' */
	return 1;
}


/* Check if the given character is part of a UTF8 sequence */
static inline bool IsUtf8Part(char c)
{
	return GB(c, 6, 2) == 2;
}


static inline bool IsPrintable(WChar c)
{
	if (c < 0x20)   return false;
	if (c < 0xE000) return true;
	if (c < 0xE200) return false;
	return true;
}


#endif /* STRING_H */
