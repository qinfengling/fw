/*
 * physmem.h - Physical memory allocator
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef PHYSMEM_H
#define	PHYSMEM_H

#include <sys/types.h>


void **calloc_phys_vec(size_t n, size_t size);
unsigned long *xlat_virt(void *const *v, size_t n);

#endif /* !PHYSMEM_H */
