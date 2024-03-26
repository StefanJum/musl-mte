#include <stdint.h>
#define ARM64_FEAT_MTE		1
#define ARM64_FEAT_MTE2		2
#define ARM64_FEAT_MTE3		3

#define MTE_TAG_GRANULE		16
#define MTE_TAG_MASK		(0xFULL << 56)

#define MTE_TCF_IGNORE		0UL
#define MTE_TCF_SYNC		1UL
#define MTE_TCF_ASYNC		2UL
#define MTE_TCF_ASYMMETRIC	3UL

static inline uint64_t mte_load_tag(uint64_t addr)
{
	uint64_t tag;

	__asm__ __volatile__ ("ldg	%x0, [%x1]\n"
			      : "=&r"(tag) : "r"(addr));

	return tag;
}

static inline void mte_store_tag(uint64_t addr)
{
	__asm__ __volatile__ ("stg	%0, [%0]"
			      : : "r"(addr) : "memory");
}

static inline void mte_store_zero_tag(uint64_t addr)
{
	__asm__ __volatile__ ("stzg	%x0, [%x0]"
			      : : "r"(addr) : "memory");
}

static inline uint64_t mte_get_exclude_mask(uint64_t addr)
{
	uint64_t reg;

	__asm__ __volatile__("gmi	%x0, %x1, xzr\n"
			     : "=r"(reg) : "r" (addr));
	return reg;
}

static inline uint64_t mte_insert_random_tag(uint64_t addr, uint64_t mask)
{
	uint64_t reg;

	__asm__ __volatile__("irg	%x0, %x2, %x1\n"
			     : "=r"(reg) : "r" (mask), "r" (addr));
	return reg;
}
