#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "pgn_pool.h"
#include "pgn.h"
#include "config.h"
#include "hasht.h"

#if !defined(PGN_POOL_SIZE)
#error "PGN_POOL_SIZE not defined"
#endif

static struct hasht_entry entries[PGN_POOL_SIZE];
static struct hasht pgn_pool = HASHT_INIT(entries, PGN_POOL_SIZE);

static inline uint32_t make_key(uint32_t pgn, uint8_t code)
{
	return pgn | (code << 24);
}

void pgn_pool_init(void)
{
	hasht_init(&pgn_pool);
}

int pgn_register(const uint32_t pgn, const uint8_t code,
		 const pgn_callback_t cb)
{
	return hasht_insert(&pgn_pool, make_key(pgn, code), cb);
}

int pgn_deregister(const uint32_t pgn, const uint8_t code)
{
	return hasht_delete(&pgn_pool, make_key(pgn, code));
}

void pgn_deregister_all(void)
{
	hasht_clear(&pgn_pool);
}

int pgn_pool_receive(void)
{
	struct hasht_entry *entry;
	j1939_pgn_t pgn;
	uint8_t code, src, priority, dest;
	uint32_t len;
	uint8_t data[8];
	int ret;

	ret = j1939_receive(&pgn, &priority, &src, &dest, data, &len);
	if (ret > 0) {
		code = (pgn == TP_CM) ? data[0] : 0;
		entry = hasht_search(&pgn_pool, make_key(pgn, code));
		if (entry && entry->item) {
			pgn_callback_t cb = (pgn_callback_t)entry->item;
			return (*cb)(pgn, priority, src, dest, data, len);
		}
	}
	return ret;
}