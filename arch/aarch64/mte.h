#include <stdint.h>

#define MTE_TAG_GRANULE		16
#define MTE_TAG_MASK		(0xFULL << 56)

/**
 * Read the allocated tag for `addr`.
 */
static inline uintptr_t mte_load_tag(uintptr_t addr)
{
	uintptr_t tag;

	__asm__ __volatile__ ("ldg	%0, [%1]\n"
			      : "=&r" (tag) : "r"(addr));

	return tag;
}

/**
 * Store the allocated tag for `addr`.
 * The tag is derived from `addr`.
 */
static inline void mte_store_tag(uintptr_t addr)
{
	__asm__ __volatile__ ("stg	%0, [%0]\n"
			      : : "r"(addr) : "memory");
}

/**
 * Tag `addr` with random tag.
 * If the address is already tagged, make sure the new tag differs.
 */
static inline uintptr_t mte_insert_random_tag(uintptr_t addr)
{
	uintptr_t reg;

	__asm__ __volatile__("gmi	%0, %1, xzr\n"
			     "irg	%1, %1, %0\n"
			     : "=&r"(reg), "+r" (addr));
	return addr;
}
