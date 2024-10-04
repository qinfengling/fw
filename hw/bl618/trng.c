/*
 * trng.h - Driver for BL618 True Random Number Generator
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ The BL618 TRNG stretches the entropy using successive hashing of the
 * random value. There are two ways to prevent/reduce such stretching:
 * 1) By forcing re-seeding. The code below tries to do this, but we don't know
 *    if it actually works.
 * 2) By reducing the number of random numbers produced before automatically
 *    re-seeding. We set this limit to "1".
 */

#include <stdint.h>
#include <sys/types.h>

#include "hal.h"
#include "mmio.h"
#include "trng.h"


#define	TRNG_BASE	(mmio_m0_base + 0x4200)

#define	TRNG_CTRL_0		(*(volatile uint32_t *) TRNG_BASE)
#define	TRNG_MASK_CTRL_MANUAL_EN	(1 << 15)
#define	TRNG_MASK_CTRL_MANUAL_RESEED	(1 << 14)
#define	TRNG_MASK_CTRL_MANUAL_FUN_SEL	(1 << 13)
#define	TRNG_MASK_CTRL_HT_ERROR		(1 << 4)
#define	TRNG_MASK_CTRL_INT_CLR		(1 << 9)
#define	TRNG_MASK_CTRL_DOUT_CLR		(1 << 3)
#define	TRNG_MASK_CTRL_EN		(1 << 2)
#define	TRNG_MASK_CTRL_TRIG		(1 << 1)
#define	TRNG_MASK_CTRL_BUSY		(1 << 0)
#define	TRNG_DOUT(n)		(*(volatile uint32_t *) \
				    (TRNG_BASE + 8 + 4 * (n)))
#define	TRNG_RESEED_N_LSB	(*(volatile uint32_t *) (TRNG_BASE + 0x2c))
#define	TRNG_RESEED_N_MSB	(*(volatile uint32_t *) (TRNG_BASE + 0x30))


static void wait_idle(void)
{
	/* based on bouffalo_sdk, drivers/lhal/src/bflb_sec_trng.c */
	asm volatile ("nop");
	asm volatile ("nop");
	asm volatile ("nop");
	asm volatile ("nop");
	while (TRNG_CTRL_0 & TRNG_MASK_CTRL_BUSY);
}


void trng_read(void *buf, size_t size)
{
	/* @@@ does this work ? */
	TRNG_RESEED_N_LSB = 1;
	TRNG_RESEED_N_MSB = 0;

	/* @@@ does this work ? */
	TRNG_CTRL_0 &= ~TRNG_MASK_CTRL_EN;
	TRNG_CTRL_0 |= TRNG_MASK_CTRL_MANUAL_EN | TRNG_MASK_CTRL_MANUAL_RESEED;
	TRNG_CTRL_0 &= ~TRNG_MASK_CTRL_MANUAL_EN;
	TRNG_CTRL_0 &= ~TRNG_MASK_CTRL_MANUAL_RESEED;
	wait_idle();

	TRNG_CTRL_0 |= TRNG_MASK_CTRL_EN;
	wait_idle();

	while (size) {
		unsigned i = 0;
		uint32_t tmp;

		TRNG_CTRL_0 |= TRNG_MASK_CTRL_TRIG;
		wait_idle();
		TRNG_CTRL_0 &= ~TRNG_MASK_CTRL_TRIG;

		for (i = 0; i != 8 && size >= 4; i++) {
			*(uint32_t *) buf = TRNG_DOUT(i);
			buf += 4;
			size -= 4;
		}
		if (!size)
			break;
		if (i == 8)
			continue;
		tmp = TRNG_DOUT(i);
		while (size) {
			*(uint8_t *) buf++ = tmp;
			tmp >>= 8;
			size--;
		}
	}
	TRNG_CTRL_0 |= TRNG_MASK_CTRL_DOUT_CLR;
	TRNG_CTRL_0 &= ~TRNG_MASK_CTRL_DOUT_CLR;
	TRNG_CTRL_0 &= ~TRNG_MASK_CTRL_EN;
}
