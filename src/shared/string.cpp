/* $Id$ */

#include "stdafx.h"
#include "string_func.h"
#include "core/math_func.hpp"
#include "debug.h"
#include "helpers.hpp"

#include <stdarg.h>
#include <ctype.h> // required for tolower()

enum StringControlCode {
	SCC_SPRITE_START  = 0xE200,
	SCC_SPRITE_END    = SCC_SPRITE_START + 0xFF,
};

/**
 * Safer implementation of vsnprintf; same as vsnprintf except:
 * - last instead of size, i.e. replace sizeof with lastof.
 * - return gives the amount of characters added, not what it would add.
 * @param str    buffer to write to up to last
 * @param last   last character we may write to
 * @param format the formatting (see snprintf)
 * @param ap     the list of arguments for the format
 * @return the number of added characters
 */
static int CDECL vseprintf(char *str, const char *last, const char *format, va_list ap)
{
	if (str >= last) return 0;
	size_t size = last - str;
	return min((int)size, vsnprintf(str, size, format, ap));
}

void ttd_strlcat(char *dst, const char *src, size_t size)
{
	assert(size > 0);
	while (size > 0 && *dst != '\0') {
		size--;
		dst++;
	}

	ttd_strlcpy(dst, src, size);
}


void ttd_strlcpy(char *dst, const char *src, size_t size)
{
	assert(size > 0);
	while (--size > 0 && *src != '\0') {
		*dst++ = *src++;
	}
	*dst = '\0';
}


char *strecat(char *dst, const char *src, const char *last)
{
	assert(dst <= last);
	while (*dst != '\0') {
		if (dst == last) return dst;
		dst++;
	}

	return strecpy(dst, src, last);
}


char *strecpy(char *dst, const char *src, const char *last)
{
	assert(dst <= last);
	while (dst != last && *src != '\0') {
		*dst++ = *src++;
	}
	*dst = '\0';

	if (dst == last && *src != '\0') {
#ifdef STRGEN
		error("String too long for destination buffer");
#else /* STRGEN */
		DEBUG(misc, 0, "String too long for destination buffer");
#endif /* STRGEN */
	}
	return dst;
}

/**
 * Safer implementation of snprintf; same as snprintf except:
 * - last instead of size, i.e. replace sizeof with lastof.
 * - return gives the amount of characters added, not what it would add.
 * @param str    buffer to write to up to last
 * @param last   last character we may write to
 * @param format the formatting (see snprintf)
 * @return the number of added characters
 */
int CDECL seprintf(char *str, const char *last, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	int ret = vseprintf(str, last, format, ap);
	va_end(ap);
	return ret;
}

char *CDECL str_fmt(const char *str, ...)
{
	char buf[4096];
	va_list va;

	va_start(va, str);
	int len = vseprintf(buf, lastof(buf), str, va);
	va_end(va);
	char *p = MallocT<char>(len + 1);
	memcpy(p, buf, len + 1);
	return p;
}


void str_validate(char *str, const char *last, bool allow_newlines, bool ignore)
{
	/* Assume the ABSOLUTE WORST to be in str as it comes from the outside. */

	char *dst = str;
	while (*str != '\0') {
		size_t len = Utf8EncodedCharLen(*str);
		/* If the character is unknown, i.e. encoded length is 0
		 * we assume worst case for the length check.
		 * The length check is needed to prevent Utf8Decode to read
		 * over the terminating '\0' if that happens to be placed
		 * within the encoding of an UTF8 character. */
		if ((len == 0 && str + 4 > last) || str + len > last) break;

		WChar c;
		len = Utf8Decode(&c, str);
		/* It's possible to encode the string termination character
		 * into a multiple bytes. This prevents those termination
		 * characters to be skipped */
		if (c == '\0') break;

		if (IsPrintable(c) && (c < SCC_SPRITE_START || c > SCC_SPRITE_END)) {
			/* Copy the character back. Even if dst is current the same as str
			 * (i.e. no characters have been changed) this is quicker than
			 * moving the pointers ahead by len */
			do {
				*dst++ = *str++;
			} while (--len != 0);
		} else if (allow_newlines && c == '\n') {
			*dst++ = *str++;
		} else {
			if (allow_newlines && c == '\r' && str[1] == '\n') {
				str += len;
				continue;
			}
			/* Replace the undesirable character with a question mark */
			str += len;
			if (!ignore) *dst++ = '?';
		}
	}

	*dst = '\0';
}

/** Convert a given ASCII string to lowercase.
 * NOTE: only support ASCII characters, no UTF8 fancy. As currently
 * the function is only used to lowercase data-filenames if they are
 * not found, this is sufficient. If more, or general functionality is
 * needed, look to r7271 where it was removed because it was broken when
 * using certain locales: eg in Turkish the uppercase 'I' was converted to
 * '?', so just revert to the old functionality */
void strtolower(char *str)
{
	for (; *str != '\0'; str++) *str = tolower(*str);
}

#ifdef WIN32
int CDECL snprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = vsnprintf(str, size, format, ap);
	va_end(ap);
	return ret;
}

#ifdef _MSC_VER
int CDECL vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int ret;
	ret = _vsnprintf(str, size, format, ap);
	if (ret < 0) str[size - 1] = '\0';
	return ret;
}
#endif /* _MSC_VER */

#endif /* WIN32 */


/* UTF-8 handling routines */


/* Decode and consume the next UTF-8 encoded character
 * @param c Buffer to place decoded character.
 * @param s Character stream to retrieve character from.
 * @return Number of characters in the sequence.
 */
size_t Utf8Decode(WChar *c, const char *s)
{
	assert(c != NULL);

	if (!HasBit(s[0], 7)) {
		/* Single byte character: 0xxxxxxx */
		*c = s[0];
		return 1;
	} else if (GB(s[0], 5, 3) == 6) {
		if (IsUtf8Part(s[1])) {
			/* Double byte character: 110xxxxx 10xxxxxx */
			*c = GB(s[0], 0, 5) << 6 | GB(s[1], 0, 6);
			if (*c >= 0x80) return 2;
		}
	} else if (GB(s[0], 4, 4) == 14) {
		if (IsUtf8Part(s[1]) && IsUtf8Part(s[2])) {
			/* Triple byte character: 1110xxxx 10xxxxxx 10xxxxxx */
			*c = GB(s[0], 0, 4) << 12 | GB(s[1], 0, 6) << 6 | GB(s[2], 0, 6);
			if (*c >= 0x800) return 3;
		}
	} else if (GB(s[0], 3, 5) == 30) {
		if (IsUtf8Part(s[1]) && IsUtf8Part(s[2]) && IsUtf8Part(s[3])) {
			/* 4 byte character: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
			*c = GB(s[0], 0, 3) << 18 | GB(s[1], 0, 6) << 12 | GB(s[2], 0, 6) << 6 | GB(s[3], 0, 6);
			if (*c >= 0x10000 && *c <= 0x10FFFF) return 4;
		}
	}

	//DEBUG(misc, 1, "[utf8] invalid UTF-8 sequence");
	*c = '?';
	return 1;
}


/* Encode a unicode character and place it in the buffer
 * @param buf Buffer to place character.
 * @param c   Unicode character to encode.
 * @return Number of characters in the encoded sequence.
 */
size_t Utf8Encode(char *buf, WChar c)
{
	if (c < 0x80) {
		*buf = c;
		return 1;
	} else if (c < 0x800) {
		*buf++ = 0xC0 + GB(c,  6, 5);
		*buf   = 0x80 + GB(c,  0, 6);
		return 2;
	} else if (c < 0x10000) {
		*buf++ = 0xE0 + GB(c, 12, 4);
		*buf++ = 0x80 + GB(c,  6, 6);
		*buf   = 0x80 + GB(c,  0, 6);
		return 3;
	} else if (c < 0x110000) {
		*buf++ = 0xF0 + GB(c, 18, 3);
		*buf++ = 0x80 + GB(c, 12, 6);
		*buf++ = 0x80 + GB(c,  6, 6);
		*buf   = 0x80 + GB(c,  0, 6);
		return 4;
	}

	//DEBUG(misc, 1, "[utf8] can't UTF-8 encode value 0x%X", c);
	*buf = '?';
	return 1;
}
