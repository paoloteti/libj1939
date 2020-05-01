#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include <stdbool.h>
#include "config.h"

typedef int atomic_t;
typedef atomic_t atomic_val_t;

#define ATOMIC_INIT(_v) (_v)

#define ATOMIC_BITS (sizeof(atomic_val_t) * 8)
#define ATOMIC_MASK(bit) (1 << ((uint32_t)(bit) & (ATOMIC_BITS - 1)))
#define ATOMIC_ELEM(addr, bit) ((addr) + ((bit) / ATOMIC_BITS))

#ifdef HAVE_STDATOMIC_H
static inline atomic_val_t atomic_get(const atomic_t *target)
{
	return __atomic_load_n(target, __ATOMIC_SEQ_CST);
}

static inline atomic_val_t atomic_or(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_or(target, value, __ATOMIC_SEQ_CST);
}

static inline atomic_val_t atomic_and(atomic_t *target, atomic_val_t value)
{
	return __atomic_fetch_and(target, value, __ATOMIC_SEQ_CST);
}

static inline void atomic_set(atomic_t *target, atomic_val_t x)
{
	__atomic_store_n(target, x, __ATOMIC_SEQ_CST);
}

static inline bool atomic_test_and_set_bit(atomic_t *target, int bit)
{
	atomic_val_t mask = ATOMIC_MASK(bit);
	atomic_val_t old;

	old = atomic_or(ATOMIC_ELEM(target, bit), mask);

	return (old & mask) != 0;
}

static inline void atomic_clear_bit(atomic_t *target, int bit)
{
	atomic_val_t mask = ATOMIC_MASK(bit);

	(void)atomic_and(ATOMIC_ELEM(target, bit), ~mask);
}

#else
extern atomic_val_t atomic_get(const atomic_t *target);
extern atomic_val_t atomic_or(atomic_t *target, atomic_val_t value);
extern atomic_val_t atomic_and(atomic_t *target, atomic_val_t value);
extern void atomic_set(atomic_t *target, atomic_val_t x);
extern bool atomic_test_and_set_bit(atomic_t *target, int bit);
extern void atomic_clear_bit(atomic_t *target, int bit);
#endif

#endif /* __ATOMIC_H__ */
