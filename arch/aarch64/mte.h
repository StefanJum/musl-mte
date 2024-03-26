#include <stdint.h>

#define MTE_TAG_GRANULE		16
#define MTE_TAG_MASK		(0xFULL << 56)

static inline uintptr_t mte_load_tag(uintptr_t addr)
{
	uintptr_t tag;

	__asm__ __volatile__ ("ldg	%x0, [%x1]\n"
			      : "=&r"(tag) : "r"(addr));

	return tag;
}

static inline void mte_store_tag(uintptr_t addr)
{
	__asm__ __volatile__ ("stg	%0, [%0]"
			      : : "r"(addr) : "memory");
}

static inline void mte_store_zero_tag(uintptr_t addr)
{
	__asm__ __volatile__ ("stzg	%x0, [%x0]"
			      : : "r"(addr) : "memory");
}

static inline uintptr_t mte_get_exclude_mask(uintptr_t addr)
{
	uintptr_t reg;

	__asm__ __volatile__("gmi	%x0, %x1, xzr\n"
			     : "=r"(reg) : "r" (addr));
	return reg;
}

static inline uintptr_t mte_insert_random_tag(uintptr_t addr, uintptr_t mask)
{
	uintptr_t reg;

	__asm__ __volatile__("irg	%x0, %x2, %x1\n"
			     : "=r"(reg) : "r" (mask), "r" (addr));
	return reg;
}
