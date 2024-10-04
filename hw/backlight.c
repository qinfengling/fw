/*
 * backlight.c - Backlight driver
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ should be a PWM driver
 */

#include <stdbool.h>

#include "board.h"
#include "gpio.h"
#include "backlight.h"


static unsigned backlight_pin;


void backlight_on(bool on)
{
	gpio_out(backlight_pin, (on == 1) ? LCD_BL_ON : LCD_BL_OFF);
}


void backlight_init(unsigned pin)
{
	backlight_pin = pin;
	gpio_cfg_out(pin, 0, 0);
}
