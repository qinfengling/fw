/*
 * flash-storage.c - Flash-based storage on M0P
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "bflb_flash.h"

#include "../../sys/debug.h"
#include "../../db/storage.h"


/*
 * 8 MB Flash (M0P). We reserve the first half for the SDK and use the second
 * half for data storage. Of the storage area, we use - at least for now - two
 * partitions of 1 MB each.
 */

#define	FLASH_STORAGE_BASE	(4 * 1024 * 1024)
#define	FLASH_STORAGE_SIZE	(2 * 1024 * 1024)

/*
 * @@@ the XT25F64FFSIG Flash in M0P has 4 kB sectors, so each "block" erase
 * erases four blocks. The Flash memories we consider for the real device also
 * have a minimum erase size of 4 kB.
 */
#define	ERASE_SIZE		4


unsigned storage_blocks(void)
{
	return FLASH_STORAGE_SIZE / STORAGE_BLOCK_SIZE;
}


unsigned storage_erase_size(void)
{
	return ERASE_SIZE;
}


bool storage_read_block(void *buf, unsigned n)
{
	uint32_t addr = FLASH_STORAGE_BASE + n * STORAGE_BLOCK_SIZE;
	int ret;

	assert(n < FLASH_STORAGE_SIZE / STORAGE_BLOCK_SIZE);
	ret = bflb_flash_read(addr, buf, STORAGE_BLOCK_SIZE);
//debug("read (%d 0x%08lx) %d\n", n, (unsigned long) addr, ret);
	return !ret;
}


bool storage_write_block(const void *buf, unsigned n)
{
	uint32_t addr = FLASH_STORAGE_BASE + n * STORAGE_BLOCK_SIZE;

/* @@@ we probably need to disable interrupts while erasing / writing Flash */
	assert(n < FLASH_STORAGE_SIZE / STORAGE_BLOCK_SIZE);
	return !bflb_flash_write(addr, (void *) buf, STORAGE_BLOCK_SIZE);
}


bool storage_erase_blocks(unsigned n, unsigned n_blocks)
{
	uint32_t addr = FLASH_STORAGE_BASE + n * STORAGE_BLOCK_SIZE;

	assert(n < FLASH_STORAGE_SIZE / STORAGE_BLOCK_SIZE);
	assert(!(n % ERASE_SIZE));
	assert(!(n_blocks % ERASE_SIZE));
	return !bflb_flash_erase(addr, n_blocks * STORAGE_BLOCK_SIZE);
}

