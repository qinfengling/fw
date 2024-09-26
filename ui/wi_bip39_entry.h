/*
 * wi_bip39_entry.h - User interface: BIP-0039 word entry
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef WI_BIP39_ENTRY_H
#define WI_BIP39_ENTRY_H

#include <stdbool.h>
#include <stdint.h>

#include "ui_entry.h"


struct wi_bip39_entry_ctx {
	struct ui_entry_input *input;
	const struct ui_entry_style *style;
	bool cancel;
	unsigned word;
	unsigned input_max_height;
};


extern struct ui_entry_ops wi_bip39_entry_ops;


/*
 * wi_bip39_entry_setup MUST be called by the caller of ui_entry, before
 * ui_entry calls wi_bip39_entry_init.
 */

void wi_bip39_entry_setup(struct wi_bip39_entry_ctx *c, bool cancel,
    unsigned word);

#endif /* !WI_BIP39_ENTRY_H */