/*
 * ui_storage.c - User interface: Storage management
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdlib.h>

#include "hal.h"
#include "gfx.h"
#include "text.h"
#include "wi_list.h"
#include "fmt.h"
#include "storage.h"
#include "block.h"
#include "ui.h"

#include "debug.h"

#define	FONT_TOP		mono24

#define	TOP_H			31
#define	TOP_LINE_WIDTH		2
#define	LIST_Y0			(TOP_H + TOP_LINE_WIDTH + 1)


static const struct wi_list_style style = {
	y0:	LIST_Y0,
	y1:	GFX_HEIGHT - 1,
	fg:	{ GFX_WHITE, GFX_WHITE },
	bg:	{ GFX_BLACK, GFX_HEX(0x202020) },
	min_h:	40,
};

static struct wi_list list;
static const struct wi_list_entry *initialize = NULL;


static void initialize_storage(void)
{
	static const uint8_t zero[BLOCK_PAYLOAD_SIZE] = { 0, };
	unsigned total = storage_blocks();
	unsigned i;

	gfx_clear(&da, GFX_BLACK);
	update_display(&da);
	for (i = 0; i != total; i++) {
debug("%u/%u\n", i, total);
		block_write(NULL, bt_empty, zero, i);
	}
}


/* --- Event handling ------------------------------------------------------ */


static void ui_storage_tap(unsigned x, unsigned y)
{
	const struct wi_list_entry *entry;

	entry = wi_list_pick(&list, x, y);
	if (entry && entry == initialize) {
		initialize_storage();
		ui_switch(&ui_storage, NULL);
	}
	return;
}


static void ui_storage_to(unsigned from_x, unsigned from_y,
    unsigned to_x, unsigned to_y, enum ui_swipe swipe)
{
	if (swipe == us_left)
		ui_return();
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_storage_open(void *params)
{
	unsigned total = storage_blocks();
	unsigned error = 0;
	unsigned empty = 0;
	unsigned unallocated = 0;
	unsigned invalid = 0;
	unsigned used = 0;
	unsigned i;
	uint8_t dummy[BLOCK_PAYLOAD_SIZE];
	char tmp[20];
	char *p;

	gfx_rect_xy(&da, 0, TOP_H, GFX_WIDTH, TOP_LINE_WIDTH, GFX_WHITE);
	text_text(&da, GFX_WIDTH / 2, TOP_H / 2, "Storage",
	    &FONT_TOP, GFX_CENTER, GFX_CENTER, GFX_WHITE);

	update_display(&da);

	for (i = 0; i != total; i++)
		switch (block_read(NULL, dummy, i)) {
		case bt_error:
			error++;
			break;
		case bt_unallocated:
			unallocated++;
			break;
		case bt_empty:
			empty++;
			break;
		case bt_corrupt:
		case bt_invalidated:
			invalid++;
			break;
		case bt_single:
		case bt_first:
		case bt_nth ... bt_max:
		case bt_last:
			used++;
			break;
		default:
			abort();
		}
	
	wi_list_begin(&list, &style);

	p = tmp;
	format(add_char, &p, "%u", total);
	wi_list_add(&list, "Total blocks", tmp, NULL);

	p = tmp;
	format(add_char, &p, "%u", used);
	wi_list_add(&list, "Used", tmp, NULL);

	p = tmp;
	format(add_char, &p, "%u", empty);
	wi_list_add(&list, "Empty", tmp, NULL);

	/*
	 * @@@ we can't have lists longer than the screen yet, so we need to
	 * switch to debug() here.
	 */
#if 0
	p = tmp;
	format(add_char, &p, "%u", unallocated);
	wi_list_add(&list, "Unallocated", tmp, NULL);
#else
debug("Unallocated: %u\n", unallocated);
#endif

#if 0
	p = tmp;
	format(add_char, &p, "%u", invalid);
	wi_list_add(&list, "Invalid", tmp, NULL);
#else
debug("Invalid: %u\n", invalid);
#endif

	if (empty + used)
		initialize = NULL;
	else
		initialize = wi_list_add(&list, "Initialize", NULL, NULL);

	wi_list_end(&list);

	set_idle(IDLE_SETUP_S);
}


static void ui_storage_close(void)
{
	wi_list_destroy(&list);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_storage_events = {
	.touch_tap	= ui_storage_tap,
	.touch_to	= ui_storage_to,
};

const struct ui ui_storage = {
	.open = ui_storage_open,
	.close = ui_storage_close,
	.events = &ui_storage_events,
};
