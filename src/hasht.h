#ifndef __HASHT_H__
#define __HASHT_H__

#define EHASHT_EMPTY 1
#define EHASHT_FULL 2
#define EHASHT_NFOUND 3
#define EHASHT_DUP 4

#define HASHT_INIT(_items, maxsize)                                            \
	{                                                                      \
		.items = _items, .max_size = maxsize, .size = 0,               \
	}

struct hasht_entry {
	uint32_t key;
	void *item;
};

struct hasht {
	struct hasht_entry *items;
	size_t max_size;
	size_t size;
};

int hasht_insert(struct hasht *ht, const uint32_t key, void *data);
struct hasht_entry *hasht_search(struct hasht *ht, const uint32_t key);
int hasht_delete(struct hasht *ht, const uint32_t key);
void hasht_clear(struct hasht *ht);
void hasht_init(struct hasht *ht);

#endif /* __HASHT_H__ */
