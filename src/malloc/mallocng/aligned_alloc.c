#include <stdlib.h>
#include <errno.h>
#include "meta.h"

void *aligned_alloc(size_t align, size_t len)
{
	/*printf("ENTERING ALIGNED_ALLOC\n");*/
	printf("aligned_alloc(%lu, %lu) = ", align, len);
	if ((align & -align) != align) {
		errno = EINVAL;
		return 0;
	}

	if (len > SIZE_MAX - align || align >= (1ULL<<31)*UNIT) {
		errno = ENOMEM;
		return 0;
	}

	if (DISABLE_ALIGNED_ALLOC) {
		errno = ENOMEM;
		return 0;
	}

	if (align <= UNIT) align = UNIT;

	unsigned char *p = malloc(len + align - UNIT);
	if (!p)
		return 0;

	unsigned char *untagged = (const unsigned char *)((uint64_t)p & ~MTE_TAG_MASK);
	struct meta *g = get_meta(p);
	int idx = get_slot_index(untagged);
	size_t stride = get_stride(g);
	unsigned char *start = g->mem->storage + stride*idx;
	/*printf("START: %p\n", start);*/
	unsigned char *end = g->mem->storage + stride*(idx+1) - IB;
	size_t adj = -(uintptr_t)p & (align-1);

	if (!adj) {
		set_size(untagged, end, len);
		printf("%p\n", p);
		return p;
	}
	p += adj;
	untagged += adj;
	uint32_t offset = (size_t)(p-g->mem->storage)/UNIT;
	if (offset <= 0xffff) {
		*(uint16_t *)(untagged-2) = offset;
		untagged[-4] = 0;
	} else {
		// use a 32-bit offset if 16-bit doesn't fit. for this,
		// 16-bit field must be zero, [-4] byte nonzero.
		*(uint16_t *)(untagged-2) = 0;
		*(uint32_t *)(untagged-8) = offset;
		untagged[-4] = 1;
	}
	untagged[-3] = idx;
	set_size(untagged, end, len);
	// store offset to aligned enframing. this facilitates cycling
	// offset and also iteration of heap for debugging/measurement.
	// for extreme overalignment it won't fit but these are classless
	// allocations anyway.
	*(uint16_t *)(start - 2) = (size_t)(p-start)/UNIT;
	start[-3] = 7<<5;
	/*printf("EXITIING ALIGNED_ALLOC\n");*/
	printf("%p\n", p);
	return p;
}
