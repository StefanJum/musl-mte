#include <stdint.h>
#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include "meta.h"

void *realloc(void *p, size_t n)
{
	n = ALIGN_UP(n, 16);
	if (!p) return malloc(n);
	if (size_overflows(n)) return 0;

#ifdef MEMTAG
	unsigned char *untagged = (unsigned char *)((uint64_t)p & ~MTE_TAG_MASK);
#else
	unsigned char *untagged = p;
#endif
	struct meta *g = get_meta(p);
	int idx = get_slot_index(untagged);
	size_t stride = get_stride(g);
	unsigned char *start = g->mem->storage + stride*idx;
	unsigned char *end = start + stride - IB;
	size_t old_size = get_nominal_size(untagged, end);
	size_t avail_size = end-(unsigned char *)untagged;
	void *new;

	// only resize in-place if size class matches
	if (n <= avail_size && n<MMAP_THRESHOLD
	    && size_to_class(n)+1 >= g->sizeclass) {

		uint64_t addr;

#ifdef MEMTAG
		for (size_t i = 0; i < old_size; i += 16)
			mte_store_tag((uint64_t)(untagged + i));

		uint64_t mask_mte = mte_get_exclude_mask((uint64_t)p);
		addr = mte_insert_random_tag((uint64_t)p, mask_mte);

		for (size_t i = 0; i < n; i += 16)
			mte_store_tag(addr + i);
#else
		addr = (uint64_t)p;
#endif

		set_size(untagged, end, n);

		return (void *)addr;
	}

	// use mremap if old and new size are both mmap-worthy
	if (g->sizeclass>=48 && n>=MMAP_THRESHOLD) {
		assert(g->sizeclass==63);
		size_t base = (unsigned char *)p-start;
		size_t needed = (n + base + UNIT + IB + 4095) & -4096;
		new = g->maplen*4096UL == needed ? g->mem :
			mremap(g->mem, g->maplen*4096UL, needed, MREMAP_MAYMOVE);
		if (new!=MAP_FAILED) {
			g->mem = new;
			g->maplen = needed/4096;
			p = g->mem->storage + base;
			end = g->mem->storage + (needed - UNIT) - IB;
			*end = 0;
			set_size(p, end, n);
			return p;
		}
	}

	new = malloc(n);
	if (!new) return 0;
	memcpy(new, p, n < old_size ? n : old_size);
	free(p);

	return new;
}
