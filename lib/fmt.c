/*
 * lib/fmt.c - Number formatting
 *
 * Written 2013-2015 by Werner Almesberger
 * Copyright 2013-2015 Werner Almesberger
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
#include <stdint.h>
#include <stdbool.h>

#include "fmt.h"


void add_char(void *user, char c)
{
	char **p = user;

	*(*p)++ = c;
	**p = 0;
}


static uint8_t do_print_number(char *s, uint32_t v, uint8_t len, uint8_t base,
    const char *alphabet)
{
	uint8_t digits = 0;
	uint32_t tmp;
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


uint8_t print_number(char *s, uint32_t v, uint8_t len, uint8_t base)
{
	return do_print_number(s, v, len, base, "0123456789abcdef");
}


static void string(void (*out)(void *user, char c), void *user, const char *s)
{
	while (*s)
		out(user, *s++);
}


void vformat(void (*out)(void *user, char c), void *user,
    const char *fmt, va_list ap)

{
	char buf[20]; /* @@@ ugly */
	bool percent = 0;
	uint8_t pad = 0; /* pacify gcc */
	const char *s;
	char ch;
	void *p;
	unsigned n;
	int sn;

	while (*fmt) {
		if (percent) {
			switch (*fmt) {
			case 'c':
				ch = va_arg(ap, int);
				out(user, ch);
				break;
			case 's':
				s = va_arg(ap, const char *);
				string(out, user, s);
				break;
			case 'd':
				sn = va_arg(ap, int);
				if (sn < 0) {
					out(user, '-');
					n = -sn;
				} else {
					n = sn;
				}
				goto decimal;
			case 'u':
				n = va_arg(ap, unsigned);
decimal:
				print_number(buf, n, pad, 10);
				string(out, user, buf);
				break;
			case 'x':
				n = va_arg(ap, unsigned);
				print_number(buf, n, pad, 16);
				string(out, user, buf);
				break;
			case 'X':
				n = va_arg(ap, unsigned);
				do_print_number(buf, n, pad, 16,
				    "0123456789ABCDEF");
				string(out, user, buf);
				break;
			case 'p':
				p = va_arg(ap, void *);
				print_number(buf, (unsigned long) p, 8, 16);
				string(out, user, buf);
				break;
			case '0'...'9':
				pad = pad * 10 + *fmt - '0';
				fmt++;
				continue;
			case '%':
				out(user, '%');
				break;
			default:
				string(out, user, "%?");
				break;
			}
			percent = 0;
		} else {
			switch (*fmt) {
			case '%':
				percent = 1;
				pad = 0;
				break;
			default:
				out(user, *fmt);
			}
		}
		fmt++;
	}
}


void format(void (*out)(void *user, char c), void *user, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vformat(out, user, fmt, ap);
	va_end(ap);
}
