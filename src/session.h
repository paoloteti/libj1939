
#ifndef __SESSION_H__
#define __SESSION_H__

struct j1939_session {
	int8_t id;
	uint8_t cts_num_packets;
	uint8_t cts_next_packet;
	uint16_t eom_ack_size;
	uint8_t eom_ack_num_packets;
	uint8_t tp_num_packets;
	uint16_t tp_tot_size;
	atomic_t cts_done;
	atomic_t eom_ack;
	uint32_t timeout;
};

void j1939_session_init(void);
uint16_t j1939_session_hash(const uint8_t s, const uint8_t d);
struct j1939_session *j1939_session_open(const uint8_t src, const uint8_t dest);
int j1939_session_close(const uint8_t src, const uint8_t dest);
struct j1939_session *j1939_session_search(const uint16_t id);
struct j1939_session *j1939_session_search_addr(const uint16_t src,
						const uint16_t dst);

#endif /* __SESSION_H__ */