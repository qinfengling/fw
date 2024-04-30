/*
 * lib/fmt.c - Number formatting
 *
 * Written 2013-2015, 2024 by Werner Almesberger
 * Copyright 2013-2015, 2024 Werner Almesberger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This work is also licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "fmt.h"


void add_char(void *user, char c)
{
	char **p = user;

	*(*p)++ = c;
	**p = 0;
}


static uint8_t do_print_number(char *s, unsigned long long v, uint8_t len,
    uint8_t base, const char *alphabet)
{
	uint8_t digits = 0;
	unsigned long long tmp;
	uint8_t res = len;

	if (!v) {
		digits = 1;
	} else {
		for (tmp = v; tmp; tmp /= base)
			digits++;
	}
	if (digits > len)
		res = digits;
	while (len > digits) {
		*s++ = '0';
		len--;
	}
	s += digits;
	*s = 0;
	while (digits--) {
		*--s = alphabet[v % base];
		v /= base;
	}
	return res;
}


uint8_t print_number(char *s, unsigned long long v, uint8_t len, uint8_t base)
{
	return do_print_number(s, v, len, base, "0123456789abcdef");
}


static void string(void (*out)(void *user, char c), void *user, const char *s)
{
	while (*s)
		out(user, *s++);
}


#define	GET_xINT(signed)						\
	({								\
		signed long long int _tmp;				\
									\
		switch (longer) {					\
		case 0:							\
			_tmp = sn = va_arg(ap, signed int);		\
			break;						\
		case 1:							\
			_tmp = sn = va_arg(ap, signed long int);	\
			break;						\
		default:						\
			_tmp = va_arg(ap, signed long long int);	\
			break;						\
		}							\
		_tmp;							\
	})


#define	GET_UINT GET_xINT(unsigned)

#define	GET_INT GET_xINT()


bool vformat(void (*out)(void *user, char c), void *user,
    const char *fmt, va_list ap)

{
	static bool nl = 1;
	char buf[20]; /* @@@ ugly */
	bool percent = 0;
	uint8_t pad = 0; /* pacify gcc */
	unsigned longer;
	const char *s;
	char ch;
	void *p;
	unsigned long long n;
	int long long sn;

	while (*fmt) {
		if (percent) {
			switch (*fmt) {
			case 'c':
				ch = va_arg(ap, int);
				out(user, ch);
				nl = ch == '\n';
				break;
			case 's':
				s = va_arg(ap, const char *);
				string(out, user, s);
				if (*s)
					nl = strchr(s, 0)[-1] == '\n';
				break;
			case 'd':
				sn = GET_INT;
				if (sn < 0) {
					out(user, '-');
					n = -sn;
				} else {
					n = sn;
				}
				goto decimal;
			case 'l':
				longer++;
				fmt++;
				continue;
			case 'u':
				n = GET_UINT;
decimal:
				print_number(buf, n, pad, 10);
				string(out, user, buf);
				nl = 0;
				break;
			case 'x':
				n = GET_UINT;
				print_number(buf, n, pad, 16);
				string(out, user, buf);
				nl = 0;
				break;
			case 'X':
				n = GET_UINT;
				do_print_number(buf, n, pad, 16,
				    "0123456789ABCDEF");
				string(out, user, buf);
				nl = 0;
				break;
			case 'p':
				p = va_arg(ap, void *);
				print_number(buf, (unsigned long) p, 8, 16);
				string(out, user, buf);
				nl = 0;
				break;
			case '0'...'9':
				pad = pad * 10 + *fmt - '0';
				fmt++;
				continue;
			case '%':
				out(user, '%');
				nl = 0;
				break;
			default:
				string(out, user, "%?");
				nl = 0;
				break;
			}
			percent = 0;
		} else {
			switch (*fmt) {
			case '%':
				percent = 1;
				pad = 0;
				longer = 0;
				nl = 0;
				break;
			default:
				out(user, *fmt);
				nl = *fmt == '\n';
			}
		}
		fmt++;
	}
	return nl;
}


bool format(void (*out)(void *user, char c), void *user, const char *fmt, ...)
{
	va_list ap;
	bool res;

	va_start(ap, fmt);
	res = vformat(out, user, fmt, ap);
	va_end(ap);
	return res;
}
