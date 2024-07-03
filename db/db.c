/*
 * db.c - Account database
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hal.h"
#include "debug.h"
#include "util.h"
#include "alloc.h"
#include "rnd.h"
#include "span.h"
#include "storage.h"
#include "block.h"
#include "db.h"


const enum field_type order2ft[] = {
    ft_end, ft_id, ft_prev, ft_user, ft_email, ft_pw, ft_pw2,
    ft_hotp_secret, ft_hotp_counter, ft_totp_secret, ft_comment };
uint8_t ft2order[ARRAY_ENTRIES(order2ft)];
const unsigned field_types = sizeof(ft2order);

static PSRAM uint8_t payload_buf[BLOCK_PAYLOAD_SIZE];


/* --- Get an erased block, erasing if needed ------------------------------ */


static int get_erased_block(struct db *db)
{
	unsigned erase_size = storage_erase_size();
	int n;

	while (1) {
		n = span_pull_one(&db->erased);
		if (n >= 0)
			break;
		n = span_pull_erase_block(&db->empty, erase_size);
		if (n >= 0) {
			db->stats.empty -= erase_size;
			if (storage_erase_blocks(n, erase_size)) {
				db->stats.erased += erase_size;
				continue;
			}
			db->stats.error += erase_size;
		}
		n = span_pull_erase_block(&db->deleted, erase_size);
		if (n >= 0) {
			db->stats.deleted -= erase_size;
			if (storage_erase_blocks(n, erase_size)) {
				db->stats.erased += erase_size;
				continue;
			}
			db->stats.error += erase_size;
		}
		return -1;
	}
	return n;
}


/* --- Helper functions ---------------------------------------------------- */


static void free_field(struct db_field *f)
{
	free(f->data);
	free(f);
}


static void free_fields(struct db_entry *de)
{
	while (de->fields) {
		struct db_field *this = de->fields;

		de->fields = this->next;
		free_field(this);
	}
}


static void free_entry(struct db_entry *de)
{
	free(de->name);
	free_fields(de);
	free(de);
}


/* --- Database entries ---------------------------------------------------- */


/*
 * Sorting: "prev" establishes a partial order [1]. The idea is that entries
 * are sorted alphabetically, but that this sort order can be overridden by
 * setting "prev".
 *
 * The algorithm below does this for any new entry that is added, where the new
 * entry may have a pointer (prev) to the previous entry. However, this is only
 * valid under the following conditions:
 * - the entry referenced by "prev" already exists when the new entry is added,
 *   and
 * - no existing entries reference the new entry as their "prev" entry.
 *
 * If these conditions are not met, we could aim for an alphabetic order, and
 * then perform a stable [2] topological sort [1] on the resulting list.
 * "Stable" means that entries on which the partial order imposes no ordering,
 * either directly or via other entries, are left in their original order with
 * respect to each other.
 *
 * Some practical considerations:
 * https://stackoverflow.com/q/11230881/11496135
 *
 * Note: the conditions of this algorithm are met if adding a new entry for
 * which no "prev" references exist in the database. However, already deleting
 * an old entry and creating a new one with the same name would violate this
 * condition. (I.e., when deleting entries, we don't try to maintain
 * referential integrity [3] by also removing all "prev" entries pointing to
 * it, which can cause the corresponding ordering relations to "come back"
 * later.)
 *
 * Also, given that entries need to be copied when making changes, including
 * automatic ones, like incrementing the HOTP counter, and thus generally move
 * around, we can easily end up with a database where entries are stored in a
 * sequence where the current algorithm fails to produce the expected results.
 *
 * [1] https://en.wikipedia.org/wiki/Topological_sorting#Relation_to_partial_orders
 * [2] https://en.wikipedia.org/wiki/Sorting_algorithm#Stability
 * [3] https://en.wikipedia.org/wiki/Referential_integrity
 */

/*
 * Update:
 * new_entry only sorts alphabetically. We now have db_tsort for proper
 * topological sorting.
 */

static struct db_entry *new_entry(struct db *db, const char *name,
    const char *prev)
{
	struct db_entry *de, **anchor;
//	struct db_entry *e = NULL;

	de = alloc_type(struct db_entry);
	memset(de, 0, sizeof(*de));
	de->db = db;
//	if (prev)
//		for (e = db->entries; e; e = e->next)
//			if (!strcmp(e->name, prev))
//				break;
//	if (e) {
//		anchor = &e->next;
//	} else {
		for (anchor = &db->entries; *anchor; anchor = &(*anchor)->next)
			if (strcmp((*anchor)->name, name) > 0)
				break;
//	}
	de->next = *anchor;
	*anchor = de;
	return de;
}


unsigned db_tsort(struct db *db)
{
	struct tmp {
		struct db_entry *in;
		const struct db_entry *prev;
		struct db_entry *out;
	} *tmp, *t, *t2;
	struct db_entry *e, *e2;
	unsigned n = 0;
	unsigned i;

	for (e = db->entries; e; e = e->next)
		n++;
	if (!n)
		return 0;

	/* allocate temporary variables */
	tmp = t = alloc_type_n(struct tmp, n);
	for (e = db->entries; e; e = e->next) {
		struct db_field *f;

		t->in = e;
		t->prev = NULL;
		for (f = e->fields; f; f = f->next)
			if (f->type == ft_prev)
				break;
		if (f)
			for (e2 = db->entries; e2; e2 = e2->next)
				if (f->len == strlen(e2->name) &&
				    !memcmp(f->data, e2->name, f->len)) {
					t->prev = e2;
					break;
				}
			/* ignore unmatched references */
		t++;
	}

	/*
	 * Based on Kahn's algorithm
	 * https://en.wikipedia.org/wiki/Topological_sorting#Kahn's_algorithm
	 */
	i = 0;
	while (1) {
		bool more = 0;
		bool progress = 0;

		for (t = tmp; t != tmp + n; t++) {
			if (!t->in)
				continue;
			if (t->prev) {
				more = 1;
			} else {
				progress = 1;
				tmp[i].out = t->in;
				i++;
				for (t2 = tmp; t2 != tmp + n; t2++)
					if (t2->prev == t->in)
						t2->prev = NULL;
				t->in = NULL;
			}
		}
		if (!more)
			break;

		/* break cycles */
		if (!progress)
			for (t = tmp; t != tmp + n; t++)
				if (t->in && t->prev) {
					t->prev = NULL;
					break;
				}
	}
	assert(i == n);

	/* apply new order */
	db->entries = tmp[0].out;
	for (t = tmp; t != tmp + n; t++)
		t->out->next = t + 1 == tmp + n ? NULL : t[1].out;

	free(tmp);

	return n;
}


static bool write_entry(const struct db_entry *de)
{
	const void *end = payload_buf + sizeof(payload_buf);
	uint8_t *p = payload_buf;
	const struct db_field *f;

	memset(payload_buf, 0, sizeof(payload_buf));
	for (f = de->fields; f; f = f->next) {
		if ((const void *) p + f->len + 2 > end) {
			debug("write_entry: %p + %u + 2 > %p\n",
			    p, f->len, end);
			return 0;
		}
		*p++ = f->type;
		*p++ = f->len;
		memcpy(p, f->data, f->len);
		p += f->len;
	}
	return block_write(de->db->c, ct_data, de->seq, payload_buf, de->block);
}


static void add_field(struct db_entry *de, enum field_type type,
    const void *data, unsigned len)
{
	struct db_field **anchor;
	struct db_field *f;

	for (anchor = &de->fields; *anchor; anchor = &(*anchor)->next)
		if ((*anchor)->type > type)
			break;
	f = alloc_type(struct db_field);
	f->type = type;
	f->len = len;
	f->data = alloc_size(len);
	memcpy(f->data, data, len);
	f->next = *anchor;
	*anchor = f;
}


struct db_entry *db_new_entry(struct db *db, const char *name)
{
	struct db_entry *de;
	int new;

	new = get_erased_block(db);
	if (new < 0)
		return NULL;
	de = new_entry(db, name, NULL);
	de->name = stralloc(name);
	de->block = new;
	rnd_bytes(&de->seq, sizeof(de->seq));
	add_field(de, ft_id, name, strlen(name));
	if (write_entry(de))
		return de;
	// @@@ complain
	if (block_delete(new)) {
		span_add(&db->deleted, new, 1);
		db->stats.deleted++;
	}
	return NULL;
}


struct db_entry *db_dummy_entry(struct db *db, const char *name,
    const char *prev)
{
	struct db_entry *de;

	de = new_entry(db, name, NULL);
	de->name = stralloc(name);
	de->block = -1;
	if (prev)
		add_field(de, ft_prev, prev, strlen(prev));
	return de;
}


static bool update_entry(struct db_entry *de, unsigned new)
{
	struct db *db = de->db;
	unsigned old = de->block;

	old = de->block;
	de->block = new;
	de->seq++;

	if (!write_entry(de)) {
		// @@@ complain more if block_delete fails ?
		if (block_delete(new)) {
			span_add(&db->deleted, new, 1);
			db->stats.deleted++;
		}
		return 0;
	}
	// @@@ complain if block_delete fails ?
	if (block_delete(old)) {
		span_add(&db->deleted, old, 1);
		db->stats.deleted++;
	}
	return 1;
}


bool db_change_field(struct db_entry *de, enum field_type type,
    const void *data, unsigned size)
{
	struct db *db = de->db;
	struct db_field **anchor;
	struct db_field *f;
	int new = -1;

	if (!de->defer) {
		new = get_erased_block(db);
		if (new < 0)
			return 0;
	}

	for (anchor = &de->fields; *anchor; anchor = &(*anchor)->next)
		if ((*anchor)->type >= type)
			break;
	if (*anchor && (*anchor)->type == type) {
		f = *anchor;
		free(f->data);
	} else {
		f = alloc_type(struct db_field);
		f->type = type;
		f->next = *anchor;
		*anchor = f;
	}
	f->len = size;
	f->data = alloc_size(size);
	memcpy(f->data, data, size);

	if (type == ft_id) {
		free(de->name);
		de->name = alloc_size(size + 1);
		memcpy(de->name, data, size);
		de->name[size] = 0;
	}

	return de->defer || update_entry(de, new);
}


bool db_delete_field(struct db_entry *de, struct db_field *f)
{
	struct db *db = de->db;
	struct db_field **anchor;
	int new = -1;

	if (!de->defer) {
		new = get_erased_block(db);
		if (new < 0)
			return 0;
	}

	for (anchor = &de->fields; *anchor != f; anchor = &(*anchor)->next);
	*anchor = f->next;
	free_field(f);

	return de->defer || update_entry(de, new);
}


bool db_entry_defer_update(struct db_entry *de, bool defer)
{
	if (!defer) {
		int new;

		new = get_erased_block(de->db);
		if (new < 0)
			return 0;
		if (!update_entry(de, new))
			return 0;
	}
		
	de->defer = defer;
	return 1;
}


bool db_delete_entry(struct db_entry *de)
{
	struct db *db = de->db;
	struct db_entry **anchor;

	for (anchor = &db->entries; *anchor != de;
	    anchor = &(*anchor)->next);
	*anchor = de->next;

	if (!block_delete(de->block))
		return 0;
	span_add(&db->deleted, de->block, 1);
	free_entry(de);
	db->stats.deleted++;
	return 1;
}


struct db_field *db_field_find(const struct db_entry *de, enum field_type type)
{
	struct db_field *f;

	for (f = de->fields; f; f = f->next)
		if (f->type == type)
			return f;
	return NULL;
}


bool db_iterate(struct db *db, bool (*fn)(void *user, struct db_entry *de),
    void *user)
{
	struct db_entry *de, *next;

	for (de = db->entries; de; de = next) {
		next = de->next;
		if (!fn(user, de))
			return 0;
	}
	return 1;
}


/* --- Open/close the account database ------------------------------------- */


void db_stats(const struct db *db, struct db_stats *s)
{
	*s = db->stats;
}


static const void *tlv_item(const void **p, const void *end,
    enum field_type *type, unsigned *len)
{
	const uint8_t *q = *p;

	if (*p + 2 > end)
		return NULL;
	if (*q == ft_end)
		return NULL;
	*type = *q++;
	*len = *q++;
	*p = q + *len;
	if (*p > end)
		return NULL;
	return q;
}


static char *alloc_string(const char *s, unsigned len)
{
	char *tmp;

	tmp = alloc_size(len + 1);
	memcpy(tmp, s, len);
	tmp[len] = 0;
	return tmp;
}


static bool process_payload(struct db *db, unsigned block, uint16_t seq,
    const void *payload, unsigned size)
{
	const void *end = payload + size;
	struct db_entry *de;
	const void *p, *q;
	enum field_type type;
	char *name;
	unsigned len;

	p = payload;
	q = tlv_item(&p, end, &type, &len);
	if (!q || type != ft_id)
		return 0;
	name = alloc_string(q, len);
	for (de = db->entries; de; de = de->next)
		if (!strcmp(de->name, name))
			break;
	if (de) {
		free(name);
		/*
		 * @@@ We lose blocks here. Should either have an "obsolete"
		 * type or consider it as "empty".
		 */
		if (((seq + 0x10000 - de->seq) & 0xffff) >= 0x8000)
			return 1;
		free_fields(de);
	} else {
		char *prev = NULL;

		while (1) {
			q = tlv_item(&p, end, &type, &len);
			if (!q)
				break;
			if (type == ft_prev) {
				prev = alloc_string(q, len);
				break;
			}
		}
		de = new_entry(db, name, prev);
		de->name = name;
		free(prev);
	}
	de->block = block;
	de->seq = seq;
	p = payload;
	while (p != end) {
		q = tlv_item(&p, end, &type, &len);
		if (!q)
			break;
		add_field(de, type, q, len);
	}
	return 1;
}


void db_open_empty(struct db *db, const struct dbcrypt *c)
{
	memset(db, 0, sizeof(*db));
	db->c = c;
	db->stats.total = storage_blocks();
	db->entries = NULL;
}


bool db_open_progress(struct db *db, const struct dbcrypt *c,
    void (*progress)(void *user, unsigned i, unsigned n), void *user)
{
	unsigned i;
	uint16_t seq;

	db_open_empty(db, c);
	for (i = 0; i != db->stats.total; i++) {
		if (progress)
			progress(user, i, db->stats.total);
		switch (block_read(c, &seq, payload_buf, i)) {
		case bt_error:
			db->stats.error++;
			break;
		case bt_deleted:
			span_add(&db->deleted, i, 1);
			db->stats.deleted++;
			break;
		case bt_erased:
			span_add(&db->erased, i, 1);
                        db->stats.erased++;
                        break;
		case bt_invalid:
			db->stats.invalid++;
			break;
		case bt_empty:
			span_add(&db->empty, i, 1);
			db->stats.empty++;
			break;
		case bt_data:
			if (process_payload(db, i, seq, payload_buf,
			    BLOCK_PAYLOAD_SIZE))
				db->stats.data++;
			else
				db->stats.invalid++;
			break;
		default:
			abort();
		}
	}
	if (progress)
		progress(user, i, i);
	memset(payload_buf, 0, sizeof(payload_buf));
	return 1;
}


bool db_open(struct db *db, const struct dbcrypt *c)
{
	return db_open_progress(db, c, NULL, NULL);
}


void db_close(struct db *db)
{
	while (db->entries) {
		struct db_entry *this = db->entries;

		db->entries = this->next;
		free_entry(this);
	}
	span_free_all(db->erased);
	span_free_all(db->deleted);
	span_free_all(db->empty);
}


void db_init(void)
{
	unsigned i;

	for (i = 0; i != sizeof(ft2order); i++)
		ft2order[order2ft[i]] = i;
}
