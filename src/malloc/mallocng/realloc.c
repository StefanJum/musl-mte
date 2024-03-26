#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include "meta.h"

void *realloc(void *p, size_t n)
{
	if (!p) return malloc(n);
	if (size_overflows(n)) return 0;

#ifdef MEMTAG
	size_t required_size = ALIGN_UP(n, 16);
#else
	size_t required_size = n;
#endif
	unsigned char *untagged = untag(p);
	struct meta *g = get_meta(p);
	int idx = get_slot_index(untagged);
	size_t stride = get_stride(g);
	unsigned char *start = g->mem->storage + stride*idx;
	unsigned char *end = start + stride - IB;
	size_t old_size = get_nominal_size(untagged, end);
	size_t avail_size = end-(unsigned char *)untagged;
	void *new;

	// only resize in-place if size class matches
	if (required_size <= avail_size && n<MMAP_THRESHOLD
	    && size_to_class(n)+1 >= g->sizeclass) {

		// zero-tag the free space left
		untag_region(untagged, n, old_size);

		set_size(untagged, end, required_size);

		return tag_region(p, n);
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
