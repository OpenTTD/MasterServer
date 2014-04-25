/* $Id$ */

/*
 * This file is part of OpenTTD's master server/updater and content service.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef STRING_H
#define STRING_H

#include "core/bitmath_func.hpp"

/**
 * Appends characters from one string to another.
 *
 * Appends the source string to the destination string with respect of the
 * terminating null-character and and the last pointer to the last element
 * in the destination buffer. If the last pointer is set to NULL no
 * boundary check is performed.
 *
 * @note usage: strecat(dst, src, lastof(dst));
 * @note lastof() applies only to fixed size arrays
 *
 * @param dst The buffer containing the target string
 * @param src The buffer containing the string to append
 * @param last The pointer to the last element of the destination buffer
 * @return The pointer to the terminating null-character in the destination buffer
 */
char *strecat(char *dst, const char *src, const char *last);

/**
 * Copies characters from one buffer to another.
 *
 * Copies the source string to the destination buffer with respect of the
 * terminating null-character and the last pointer to the last element in
 * the destination buffer. If the last pointer is set to NULL no boundary
 * check is performed.
 *
 * @note usage: strecpy(dst, src, lastof(dst));
 * @note lastof() applies only to fixed size arrays
 *
 * @param dst The destination buffer
 * @param src The buffer containing the string to copy
 * @param last The pointer to the last element of the destination buffer
 * @return The pointer to the terminating null-character in the destination buffer
 */
char *strecpy(char *dst, const char *src, const char *last);

/**
 * Scans the string for valid characters and if it finds invalid ones,
 * replaces them with a question mark '?' (if not ignored)
 * @param str the string to validate
 * @param last the last valid character of str
 * @param allow_newlines whether newlines should be allowed or ignored
 * @param ignore whether to ignore or replace with a question mark
 */
void str_validate(char *str, const char *last, bool allow_newlines = false, bool ignore = false);

int CDECL seprintf(char *str, const char *last, const char *format, ...);
int CDECL vseprintf(char *str, const char *last, const char *format, va_list ap);

/** Convert the given string to lowercase, only works with ASCII! */
void strtolower(char *str);

/**
 * Check if a string buffer is empty.
 *
 * @param s The pointer to the firste element of the buffer
 * @return true if the buffer starts with the terminating null-character or
 *         if the given pointer points to NULL else return false
 */
static inline bool StrEmpty(const char *s)
{
	return s == NULL || s[0] == '\0';
}

/**
 * Get the length of a string, within a limited buffer.
 *
 * @param str The pointer to the firste element of the buffer
 * @param maxlen The maximum size of the buffer
 * @return The length of the string
 */
static inline size_t ttd_strnlen(const char *str, size_t maxlen)
{
	const char *t;
	for (t = str; (size_t)(t - str) < maxlen && *t != '\0'; t++) {}
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
static inline int8 Utf8CharLen(WChar c)
{
	if (c < 0x80)       return 1;
	if (c < 0x800)      return 2;
	if (c < 0x10000)    return 3;
	if (c < 0x110000)   return 4;

	/* Invalid valid, we encode as a '?' */
	return 1;
}


/**
 * Return the length of an UTF-8 encoded value based on a single char. This
 * char should be the first byte of the UTF-8 encoding. If not, or encoding
 * is invalid, return value is 0
 * @param c char to query length of
 * @return requested size
 */
static inline int8 Utf8EncodedCharLen(char c)
{
	if (GB(c, 3, 5) == 0x1E) return 4;
	if (GB(c, 4, 4) == 0x0E) return 3;
	if (GB(c, 5, 3) == 0x06) return 2;
	if (GB(c, 7, 1) == 0x00) return 1;

	/* Invalid UTF8 start encoding */
	return 0;
}


/* Check if the given character is part of a UTF8 sequence */
static inline bool IsUtf8Part(char c)
{
	return GB(c, 6, 2) == 2;
}

/**
 * Retrieve the previous UNICODE character in an UTF-8 encoded string.
 * @param s char pointer pointing to (the first char of) the next character
 * @return a pointer in 's' to the previous UNICODE character's first byte
 * @note The function should not be used to determine the length of the previous
 * encoded char because it might be an invalid/corrupt start-sequence
 */
static inline char *Utf8PrevChar(const char *s)
{
	const char *ret = s;
	while (IsUtf8Part(*--ret)) {}
	return (char*)ret;
}

static inline bool IsPrintable(WChar c)
{
	if (c < 0x20)   return false;
	if (c < 0xE000) return true;
	if (c < 0xE200) return false;
	return true;
}


#endif /* STRING_H */
