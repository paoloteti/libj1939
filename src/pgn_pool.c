#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "pgn_pool.h"
#include "pgn.h"
#include "config.h"

struct pgn_ht_item {
	uint32_t pgn;
	pgn_callback_t callback;
};

static size_t size;

#if !defined(PGN_POOL_SIZE)
#error "PGN_POOL_SIZE not defined"
#endif

static struct pgn_ht_item pgn_pool[PGN_POOL_SIZE];

static inline uint32_t next_hash(const uint32_t hash)
{
	return (hash + 1u) % PGN_POOL_SIZE;
}

static inline uint32_t hash_code(const uint32_t key)
{
	return key % PGN_POOL_SIZE;
}

static int pgn_ht_delete(const uint32_t key)
{
	int nitems = PGN_POOL_SIZE;
	uint32_t hash = hash_code(key);

	if (size > 0) {
		while (nitems-- != 0) {
			if (pgn_pool[hash].pgn == key) {
				pgn_pool[hash].callback = NULL;
				pgn_pool[hash].pgn = 0;
				size--;
				return ERR_NONE;
			}
			hash = next_hash(hash);
		}
	}
	return -ERR_PGN_UNKNOWN;
}

static struct pgn_ht_item *pgn_ht_search(const uint32_t key)
{
	uint32_t hash = hash_code(key);
	int nitems = PGN_POOL_SIZE;
	while (nitems-- != 0) {
		if (pgn_pool[hash].pgn == key) {
			return &pgn_pool[hash];
		}
		hash = next_hash(hash);
	}
	return NULL;
}

static int pgn_ht_insert(const uint32_t key, pgn_callback_t cb)
{
	uint32_t hash = hash_code(key);

	if (size == PGN_POOL_SIZE) {
		return -ERR_TOO_MANY_PGN;
	}

	if (pgn_ht_search(key) == NULL) {
		while (pgn_pool[hash].callback != NULL) {
			hash = next_hash(hash);
		}
		pgn_pool[hash].pgn = key;
		pgn_pool[hash].callback = cb;
		size++;
		return ERR_NONE;
	}
	return -ERR_DUPLICATE_PGN;
}

static inline uint32_t make_key(uint32_t pgn, uint8_t code)
{
	return pgn | (code << 24);
}

int pgn_register(const uint32_t pgn, const uint8_t code,
		 const pgn_callback_t cb)
{
	return pgn_ht_insert(make_key(pgn, code), cb);
}

int pgn_deregister(const uint32_t pgn, const uint8_t code)
{
	return pgn_ht_delete(make_key(pgn, code));
}

void pgn_deregister_all(void)
{
	size = 0;
}


int pgn_pool_receive(void)
{
	struct pgn_ht_item *entry;
	j1939_pgn_t pgn;
	uint8_t code, src, priority, dest;
	uint32_t len;
	uint8_t data[8];
	int ret;

	ret = j1939_receive(&pgn, &priority, &src, &dest, data, &len);
	if (ret > 0) {
		code = (pgn == TP_CM) ? data[0] : 0;
		entry = pgn_ht_search(make_key(pgn, code));
		if (entry && entry->callback) {
			return (*entry->callback)(pgn, priority, src, dest, data, len);
		}
	}
	return ret;
}