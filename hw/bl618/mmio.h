/*
 * mmio.h - Memory-mapped register IO (BL618)
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef MMIO_H
#define	MMIO_H

extern volatile void *mmio_m0_base;


void mmio_init(void);

#endif /* !MMIO_H */
