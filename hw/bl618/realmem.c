/*
 * realmem.h - Memory mappings for running on Dummy the real hardware
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "mmio.h"
#include "physmem.h"


/* Dummy mmio.c */


volatile void *mmio_m0_base;


void mmio_init(void)
{
	mmio_m0_base = (volatile void *) 0x20000000;
}


/* Dummy physmem.c */


void **calloc_phys_vec(size_t n, size_t size)
{
	void **vec;
	size_t i;

	vec = malloc(sizeof(void *) * n);
	if (!vec) {
		perror("malloc");
		exit(1);
	}
	for (i = 0; i != n; i++) {
		vec[i] = malloc(size);
		if (!vec[i]) {
			perror("malloc");
			exit(1);
		}
	}
	return vec;
}


unsigned long *xlat_virt(void *const *v, size_t n)
{
	unsigned long *res;
	size_t i;

	res = malloc(n * sizeof(unsigned long));
	if (!res) {
		perror("malloc");
		exit(1);
	}
	for (i = 0; i != n; i++)
		res[i] = (unsigned long) v[i];
	return res;
}
