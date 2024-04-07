/*
 * hal.h - Hardware abstraction layer
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef HAL_H
#define	HAL_H

#include <stdarg.h>
#include <stdbool.h>

#include "gfx.h"


#define GFX_WIDTH	240
#define GFX_HEIGHT	280


void vdebug(const char *fmt, va_list ap);
void t0(void);
void t1(const char *s);

void button_event(bool down);
void touch_down_event(unsigned x, unsigned y);
void touch_move_event(unsigned x, unsigned y);
void touch_up_event(void);

void mdelay(unsigned ms);
void msleep(unsigned ms);

void update_display(struct gfx_drawable *da);
void display_on(bool on);

bool app_init(char *const *args, unsigned n_args);

#endif /* !HAL_H */
