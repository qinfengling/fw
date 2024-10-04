/*
 * spi.h - TX-only SPI0 driver for BL618
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef SPI_H
#define	SPI_H

void spi_sync(void);

void spi_start(void);
void spi_send(const void *data, unsigned len);
void spi_end(void);

void spi_init(unsigned mosi, unsigned sclk, unsigned ss, unsigned MHz);

#endif /* !SPI_H */
