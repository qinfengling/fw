/*
 * ui_entry.c - User interface: Text entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

/*
 * @@@ ui_entry may be used at different places. maybe we need a stack of UI
 * elements ?
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "hal.h"
#include "debug.h"
#include "timer.h"
#include "gfx.h"
#include "ntext.h"
#include "shape.h"
#include "ui.h"


/* --- Input --------------------------------------------------------------- */

#define	INPUT_VALID_BG		gfx_hex(0x002060)
#define	INPUT_INVALID_BG	gfx_hex(0x800000)

#define	INPUT_H			54
#define	INPUT_Y0		5
#define	INPUT_FONT_SIZE		42
#define	INPUT_FONT		mono34
#define INPUT_MAX_X		210

/* --- Keypad -------------------------------------------------------------- */

#define UP_BG			GFX_WHITE
#define DOWN_BG			gfx_hex(0x808080)
#define	DISABLED_BG		gfx_hex(0x606060)
#define	SPECIAL_UP_BG		GFX_YELLOW
#define	SPECIAL_DOWN_BG		gfx_hex(0x808000)
#define	SPECIAL_DISABLED_BG	gfx_hex(0x404030)

#define	BUTTON_LINGER_MS	150

#define	LABEL_MARGIN		2

/*
 * Generally approximate the golden ratio: 1:1.618 or 0.384:0.615, but increase
 * the font size since since we have only one small descender (for Q).
 */

#define	FONT_1_TOP_SIZE		32
#define	FONT_1_BOTTOM_SIZE	22
#define	FONT_1_TOP		mono24
#define	FONT_1_BOTTOM		mono18

#define	FONT_2_SIZE		42
#define	FONT_2			mono34

#define	LABEL_TOP_OFFSET	-11
#define	LABEL_BOTTOM_OFFSET	14

#define	BUTTON_BOTTOM_OFFSET	3

#define	BUTTON_R		12
#define	BUTTON_W		70
#define	BUTTON_H		50
#define	BUTTON_X_GAP		10
#define	BUTTON_Y_GAP		5
#define	BUTTON_X_SPACING	(BUTTON_W + BUTTON_X_GAP)
#define	BUTTON_Y_SPACING	(BUTTON_H + BUTTON_Y_GAP)
#define	BUTTON_X0		(GFX_WIDTH / 2 - BUTTON_X_SPACING)
#define	BUTTON_Y1		(GFX_HEIGHT - BUTTON_H / 2 - \
				    BUTTON_BOTTOM_OFFSET)
#define	BUTTON_Y0		(BUTTON_Y1 - 3 * BUTTON_Y_SPACING)

#define	X_ADJUST(ch)		0
#define	Y_ADJUST(ch)		(use_ntext ? -1 : 1)


/* --- First page ---------------------------------------------------------- */


static const char *first_map[] = {
	"0 /!?",
	"1@\"+#",
	"2ABC",
	"3DEF",
	"4GHI",
	"5JKL",
	"6MNO",
	"7PQRS",
	"8TUV",
	"9WXYZ"
};


/* --- Second pages -------------------------------------------------------- */


static const char *second_maps[] = {
	"0&!?:;/ .,",
	"1'\"`+-*@#$",
	"2ABCabc(&)",
	"3DEFdef<=>",
	"4GHIghi[\\]",
	"5JKLjkl{|}",
	"6MNOmno^_~",
	"7PQRpqrSs",
	"8TUVtuv%",
	"9WXYwxyZz"
};


/* ---  Variables ---------------------------------------------------------- */


char ui_entry_input[MAX_INPUT_LEN + 1] = { 0, };
bool (*ui_entry_validate)(const char *s) = NULL;

static const char *second = NULL;
static struct timer t_button;


/* --- Input validation ---------------------------------------------------- */


static bool valid(void)
{
	return !*ui_entry_input || !*ui_entry_validate ||
            ui_entry_validate(ui_entry_input);
}


/* --- Input field --------------------------------------------------------- */


/* @@@ we should just clear the bounding box */

static void clear_input(void)
{
	gfx_rect_xy(&da, 0, 0, GFX_WIDTH, INPUT_H,
	    valid() ? INPUT_VALID_BG : INPUT_INVALID_BG);
}


static void draw_input(void)
{
	struct gfx_drawable buf;
	gfx_color fb[MAX_INPUT_LEN * INPUT_FONT_SIZE * INPUT_H];
	struct gfx_rect bb;

	if (!*ui_entry_input)
		return;
	if (use_ntext)
		ntext_text_bbox(0, 0, ui_entry_input, &INPUT_FONT,
		    GFX_LEFT, GFX_TOP, &bb);
	else
		gfx_text_bbox(0, 0, ui_entry_input, INPUT_FONT_SIZE,
		    GFX_LEFT, GFX_TOP, &bb);
	assert(bb.w <= MAX_INPUT_LEN * INPUT_FONT_SIZE);
	assert(bb.h <= INPUT_H);
	gfx_da_init(&buf, bb.w, bb.h, fb);
        gfx_clear(&buf, valid() ? INPUT_VALID_BG : INPUT_INVALID_BG);
	if (use_ntext)
	        ntext_text(&buf, 0, 0, ui_entry_input, &INPUT_FONT,
		    GFX_LEFT, GFX_TOP, GFX_WHITE);
	else
	        gfx_text(&buf, 0, 0, ui_entry_input, INPUT_FONT_SIZE,
		    GFX_LEFT, GFX_TOP, GFX_WHITE);
	if ((GFX_WIDTH + bb.w) / 2 < INPUT_MAX_X)
		gfx_copy(&da, (GFX_WIDTH - bb.w) / 2, INPUT_Y0, &buf, 0, 0,
		    bb.w, bb.h, -1);
	else if (bb.w < INPUT_MAX_X)
		gfx_copy(&da, INPUT_MAX_X - bb.w, INPUT_Y0, &buf, 0, 0,
		    bb.w, bb.h, -1);
	else
		gfx_copy(&da, 0, INPUT_Y0, &buf, bb.w - INPUT_MAX_X, 0,
		    INPUT_MAX_X, bb.h, -1);
}


/* --- Common for all buttons ---------------------------------------------- */


static void base(unsigned x, unsigned y, gfx_color bg)
{
	int dx, dy;

	for (dx = -1; dx <= 1; dx += 2)
		for (dy = -1; dy <= 1; dy += 2) {
			gfx_disc(&da,
			    x + dx * (BUTTON_W / 2 - BUTTON_R - (dx == 1)),
			    y + dy * (BUTTON_H / 2 - BUTTON_R - (dy == 1)),
			    BUTTON_R, bg);
		}
	gfx_rect_xy(&da, x - BUTTON_W / 2 + BUTTON_R, y - BUTTON_H / 2,
	    BUTTON_W - 2 * BUTTON_R, BUTTON_R, bg);
	gfx_rect_xy(&da, x - BUTTON_W / 2, y - BUTTON_H / 2 + BUTTON_R,
	    BUTTON_W, BUTTON_H - 2 * BUTTON_R, bg);
	gfx_rect_xy(&da, x - BUTTON_W / 2 + BUTTON_R,
	    y + BUTTON_H / 2 - BUTTON_R,
	    BUTTON_W - 2 * BUTTON_R, BUTTON_R, bg);
}


/* --- Draw buttons (first level) ------------------------------------------ */


static void first_label(unsigned x, unsigned y, const char *s)
{
	char top[] = { *s, 0 };

	if (use_ntext) {
		ntext_text(&da,
		    x + X_ADJUST(*s), y + Y_ADJUST(*s) + LABEL_TOP_OFFSET,
		    top, &FONT_1_TOP, GFX_CENTER, GFX_CENTER, GFX_BLACK);
		ntext_text(&da, x + X_ADJUST('X'),
		    y + Y_ADJUST('A') + LABEL_BOTTOM_OFFSET,
		    s + 1, &FONT_1_BOTTOM, GFX_CENTER, GFX_CENTER,
		    GFX_BLACK);
	} else {
		gfx_text(&da,
		    x + X_ADJUST(*s), y + Y_ADJUST(*s) + LABEL_TOP_OFFSET,
		    top, FONT_1_TOP_SIZE, GFX_CENTER, GFX_CENTER, GFX_BLACK);
		gfx_text(&da, x + X_ADJUST('X'),
		    y + Y_ADJUST('A') + LABEL_BOTTOM_OFFSET,
		    s + 1, FONT_1_BOTTOM_SIZE, GFX_CENTER, GFX_CENTER,
		    GFX_BLACK);
	}
}


static void first_button(unsigned col, unsigned row, gfx_color bg)
{
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
        unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;

	if (row > 0) {
		base(x, y, bg);
		first_label(x, y, first_map[1 + col + (3 - row) * 3]);
	} else if (col == 0) {	// X
		base(x, y, bg);
		if (*ui_entry_input)
			equilateral(x, y, BUTTON_H * 0.7, -1, GFX_BLACK);
		else
			cross(x, y, BUTTON_H * 0.4, 4, GFX_BLACK);
	} else if (col == 1) {	// "0"
		base(x, y, bg);
		first_label(x, y, first_map[0]);
	} else {	// >
		if (*ui_entry_input) {
			base(x, y, valid() ? bg : SPECIAL_DISABLED_BG);
			equilateral(x, y, BUTTON_H * 0.7, 1, GFX_BLACK);
		} else {
			base(x, y, GFX_BLACK);
		}
	}
}


static void draw_first_text(bool enabled)
{
	unsigned row, col;

	for (col = 0; col != 3; col++)
		for (row = 0; row != 4; row++)
			if (row > 0 || col == 1)
				first_button(col, row,
				    enabled ? UP_BG : DISABLED_BG);
}


/* --- Draw buttons (second level) ----------------------------------------- */


static void second_label(unsigned x, unsigned y, char ch)
{
	char s[] = { ch, 0 };

	if (use_ntext)
		ntext_text(&da, x + X_ADJUST(ch), y + Y_ADJUST(ch),
		    s, &FONT_2, GFX_CENTER, GFX_CENTER, GFX_BLACK);
	else
		gfx_text(&da, x + X_ADJUST(ch), y + Y_ADJUST(ch),
		    s, FONT_2_SIZE, GFX_CENTER, GFX_CENTER, GFX_BLACK);
}


static void second_button(const char *map, unsigned col, unsigned row,
    gfx_color bg)
{
	unsigned x = BUTTON_X0 + BUTTON_X_SPACING * col;
        unsigned y = BUTTON_Y1 - BUTTON_Y_SPACING * row;
	size_t len = strlen(map);
	unsigned n = 1 + col + (3 - row) * 3;

	if (row > 0 && n >= len)
		return;
	if (row > 0) {
		base(x, y, bg);
		second_label(x, y, map[n]);
	} else if (col == 0) {	// X
		base(x, y, SPECIAL_UP_BG);
		equilateral(x, y, BUTTON_H * 0.7, -1, GFX_BLACK);
	} else if (col == 1) {	// "0"
		base(x, y, bg);
		second_label(x, y, map[0]);
	} else {	// >
		base(x, y, GFX_BLACK);
	}
}


static void draw_second(const char *map)
{
	const unsigned h = 4 * BUTTON_H + 2 * BUTTON_X_GAP;
	unsigned row, col;

	gfx_rect_xy(&da, 0, GFX_HEIGHT - h - BUTTON_BOTTOM_OFFSET,
	    GFX_WIDTH, h, GFX_BLACK);
	for (col = 0; col != 3; col++)
		for (row = 0; row != 4; row++)
			second_button(map, col, row, UP_BG);
}


/* --- Event handling ------------------------------------------------------ */


static void release_button(void *user)
{
	unsigned n = (uintptr_t) user;

	first_button(n >> 4, n & 15, SPECIAL_UP_BG);
	update_display(&da);
}


static void ui_entry_tap(unsigned x, unsigned y)
{
	unsigned col = x < BUTTON_X0 + BUTTON_X_SPACING / 2 ? 0 :
	    x < BUTTON_X0 + 1.5 * BUTTON_X_SPACING ? 1 : 2;
	unsigned row = 0;
	unsigned n;
	char *end = strchr(ui_entry_input, 0);

	if (y < BUTTON_Y0 - BUTTON_Y_SPACING + BUTTON_R)
		return;
	if (y < BUTTON_Y0)
		row = 3;
	else if (y > BUTTON_Y1)
		row = 0;
	else
		row = 3 - (y - BUTTON_Y0 + BUTTON_Y_SPACING / 2) /
		    BUTTON_Y_SPACING;

	debug("entry_tap X %u Y %u -> col %u row %u\n", x, y, col, row);
	if (col == 0 && row == 0) { // cancel or delete
		if (second) {
			second = NULL;
			draw_first_text(1);
			first_button(0, 0, SPECIAL_UP_BG);
			first_button(2, 0, SPECIAL_UP_BG);
			update_display(&da);
			return;
		}
		if (!*ui_entry_input) {
			ui_switch(&ui_accounts);
			return;
		}
		timer_flush(&t_button);
		first_button(0, 0, SPECIAL_DOWN_BG);
		timer_set(&t_button, BUTTON_LINGER_MS, release_button,
		    (void *) (uintptr_t) (col << 4 | row));
		progress();

		end[-1] = 0;
		clear_input();
		draw_input();
		if (!*ui_entry_input)
			first_button(2, 0, SPECIAL_UP_BG);
		if (end - ui_entry_input == MAX_INPUT_LEN)
			draw_first_text(1);
		update_display(&da);
		return;
	}
	if (col == 2 && row == 0) { // enter
		if (second || !*ui_entry_input)
			return;
		if (valid())
			ui_switch(&ui_accounts);
		return;
	}
	if (end - ui_entry_input == MAX_INPUT_LEN)
		return;
	progress();
	timer_flush(&t_button);
	n = row ? (3 - row) * 3 + col + 1 : 0;
	if (second) {
		if (n >= strlen(second))
			return;
		end[0] = second[n];
		end[1] = 0;
		second = NULL;
		clear_input();
		draw_input();
		first_button(0, 0, SPECIAL_UP_BG);
		first_button(2, 0, SPECIAL_UP_BG);
		draw_first_text(strlen(ui_entry_input) != MAX_INPUT_LEN);
	} else {
		second = second_maps[n];
		draw_second(second);
	}
	update_display(&da);
}


/* --- Open/close ---------------------------------------------------------- */


static void ui_entry_open(void)
{
	assert(strlen(ui_entry_input) <= MAX_INPUT_LEN);

	clear_input();
	draw_input();
	first_button(0, 0, SPECIAL_UP_BG);
	draw_first_text(strlen(ui_entry_input) != MAX_INPUT_LEN);
	first_button(2, 0, SPECIAL_UP_BG);
	timer_init(&t_button);
}


static void ui_entry_close(void)
{
	timer_cancel(&t_button);
}


/* --- Interface ----------------------------------------------------------- */


static const struct ui_events ui_entry_events = {
	.touch_tap	= ui_entry_tap,
};


const struct ui ui_entry = {
	.open = ui_entry_open,
	.close = ui_entry_close,
	.events	= &ui_entry_events,
};