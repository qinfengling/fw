/*
 * trng.h - Driver for BL618 True Random Number Generator
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef TRNG_H
#define	TRNG_H

#include <sys/types.h>


void trng_read(void *buf, size_t size);

#endif /* !TRNG_H */
