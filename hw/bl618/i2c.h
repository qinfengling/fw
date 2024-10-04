/*
 * i2c.h - Driver for BL618 I2C
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef I2C_H
#define	I2C_H

#include <stdbool.h>


unsigned i2c_write(unsigned i2c, unsigned addr, unsigned reg,
    const void *data, unsigned len);
bool i2c_read(unsigned i2c, unsigned addr, unsigned reg,
    void *data, unsigned len);

void i2c_init(unsigned i2c, unsigned sda, unsigned scl, unsigned kHz);

#endif /* !I2C_H */
