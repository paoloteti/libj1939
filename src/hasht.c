#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "hasht.h"

static inline uint32_t next_hash(struct hasht *ht, const uint32_t hash)
{
	return (hash + 1u) % ht->max_size;
}

static inline uint32_t hash_code(struct hasht *ht, const uint32_t key)
{
	return key % ht->max_size;
}

void hasht_clear(struct hasht *ht)
{
	ht->size = 0;
}

int hasht_delete(struct hasht *ht, const uint32_t key)
{
	size_t nitems = ht->size;
	uint32_t hash = hash_code(ht, key);

	if (ht->size > 0) {
		while (nitems-- != 0) {
			if (ht->items[hash].key == key) {
				ht->items[hash].item = NULL;
				ht->size--;
				return 0;
			}
			hash = next_hash(ht, hash);
		}
	}
	return -EHASHT_NFOUND;
}

struct hasht_entry *hasht_search(struct hasht *ht, const uint32_t key)
{
	uint32_t hash = hash_code(ht, key);
	size_t nitems = ht->size;
	while (nitems-- != 0) {
		if (ht->items[hash].key == key) {
			return &ht->items[hash];
		}
		hash = next_hash(ht, hash);
	}
	return NULL;
}

int hasht_insert(struct hasht *ht, const uint32_t key, void *data)
{
	uint32_t hash = hash_code(ht, key);

	if (ht->size == ht->max_size) {
		return -EHASHT_FULL;
	}

	if (hasht_search(ht, key) == NULL) {
		while (ht->items[hash].item != NULL) {
			hash = next_hash(ht, hash);
		}
		ht->items[hash].key = key;
		ht->items[hash].item = data;
		ht->size++;
		return 0;
	}
	return -EHASHT_DUP;
}
