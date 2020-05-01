/* SPDX-License-Identifier: Apache-2.0 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "atomic.h"
#include "hasht.h"
#include "session.h"

#define SESSION_UNDEF (-1)
#if !defined(MAX_J1939_SESSIONS)
#error "MAX_J1939_SESSIONS not defined"
#endif

static struct j1939_session session_dict[MAX_J1939_SESSIONS];
static struct hasht_entry entries[MAX_J1939_SESSIONS];
static struct hasht sessions = HASHT_INIT(entries, MAX_J1939_SESSIONS);

uint16_t j1939_session_hash(const uint8_t s, const uint8_t d)
{
	return (s << 8) | d;
}

void j1939_session_init(void)
{
	hasht_init(&sessions);
	for (size_t i = 0; i < MAX_J1939_SESSIONS; i++) {
		session_dict[i].id = SESSION_UNDEF;
	}
}

static struct j1939_session *assign_session(void)
{
	for (int8_t i = 0; i < MAX_J1939_SESSIONS; i++) {
		if (session_dict[i].id < 0) {
			memset(&session_dict[i], 0,
			       sizeof(struct j1939_session));
			session_dict[i].id = i;
			return &session_dict[i];
		}
	}
	return NULL;
}

struct j1939_session *j1939_session_open(const uint8_t src, const uint8_t dest)
{
	struct j1939_session *sess;
	uint16_t key = j1939_session_hash(src, dest);
	if (j1939_session_search(key) == NULL) {
		sess = assign_session();
		if (sess) {
			hasht_insert(&sessions, key, sess);
			return sess;
		}
	}
	return NULL;
}

struct j1939_session *j1939_session_search_addr(const uint16_t src,
						const uint16_t dst)
{
	uint16_t key = j1939_session_hash(src, dst);
	return j1939_session_search(key);
}

struct j1939_session *j1939_session_search(const uint16_t id)
{
	struct hasht_entry *s;
	s = hasht_search(&sessions, id);
	return s != NULL ? (struct j1939_session *)s->item : NULL;
}

int j1939_session_close(const uint8_t src, const uint8_t dest)
{
	struct j1939_session *sess;
	uint16_t key = j1939_session_hash(src, dest);
	sess = j1939_session_search(key);
	if (sess) {
		sess->id = SESSION_UNDEF;
		return hasht_delete(&sessions, key);
	}
	return -1;
}
