/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __J1939_H__
#define __J1939_H__

#define ADDRESS_GLOBAL 0xFFu
#define ADDRESS_NULL 0xEFu

#define J1939_PRIORITY_HIGH 0x0u
#define J1939_PRIORITY_LOW 0x7u

#define REASON_BUSY 0x01u /*<! Node is busy */
#define REASON_NO_RESOURCE 0x02u /*<! Lacking the necessary resources */
#define REASON_TIMEOUT 0x03u /*<! A timeout occurred */
#define REASON_CTS_WHILE_DT 0x04u /*<! CTS received when during transfer */

/** @brief indicates that the parameter is "not available" */
#define J1930_NOT_AVAILABLE_8 0xFFu
#define J1930_NA_8 J1930_NOT_AVAILABLE_8
#define J1930_NOT_AVAILABLE_16 0xFF00u
#define J1930_NA_16 J1930_NOT_AVAILABLE_16

/** @brief indicates that the parameter is "not valid" or "in error" */
#define J1930_NOT_VALID_8 0xFEu
#define J1930_NV_8 J J1930_NOT_VALID_8
#define J1930_NOT_VALID_16 0xFE00u
#define J1930_NV_16 J1930_NOT_VALID_16

/** @brief raw parameter values must not exceed the following values */
#define J1930_MAX_8 0xFAu
#define J1930_MAX_16 0xFAFFu

extern int j1939_cansend(uint32_t id, uint8_t *data, uint8_t len);
extern int j1939_canrcv(uint32_t *id, uint8_t *data);

struct j1939_pgn {
	uint8_t data_page;
	uint8_t pdu_format;
	uint8_t pdu_specific;
};

/** @brief Timeouts ([msec]) according SAE J1939/21 */
enum j1939_timeouts {
	/* Response Time */
	Tr = 200,
	/* Holding Time */
	Th = 500,
	T1 = 750,
	T2 = 1250,
	T3 = 1250,
	T4 = 1050,
	/* timeout for multi packet broadcast messages 50..200ms */
	Tb = 50,
};

/** @brief Initialize Parameter Group Number using 3 bytes */
#define J1939_INIT_PGN(_dp, _format, _specific)                                \
	{                                                                      \
		.data_page = _dp & 0x01u, .pdu_format = _format,               \
		.pdu_specific = _specific,                                     \
	}

bool static inline j1939_valid_priority(const uint8_t p)
{
	return p <= J1939_PRIORITY_LOW;
}

/** @brief Check if PDU format < 240 (peer-to-peer) */
static inline bool j1939_pdu_is_p2p(const struct j1939_pgn *pgn)
{
	return pgn->pdu_format < 240u;
}

/** @brief Check if PDU format >= 240 (broadcast) */
static inline bool j1939_pdu_is_broadcast(const struct j1939_pgn *pgn)
{
	return !j1939_pdu_is_p2p(pgn);
}

static inline void j1939_pgn_from_id(struct j1939_pgn *pgn, const uint32_t id)
{
	pgn->pdu_format = (id >> 16) & 0x000000FFu;
	pgn->pdu_specific = (id >> 8) & 0x000000FFu;
	pgn->data_page = (id >> 24) & 0x00000001u;
}

static inline uint32_t j1939_pgn_to_id(const struct j1939_pgn *pgn)
{
	return ((uint32_t)pgn->data_page << 16) |
	       ((uint32_t)pgn->pdu_format << 8) | (uint32_t)pgn->pdu_specific;
}

int j1939_send(const struct j1939_pgn *pgn, const uint8_t priority,
	       const uint8_t src, const uint8_t dst, uint8_t *data,
	       const uint32_t len);

int j1939_receive(struct j1939_pgn *pgn, uint8_t *priority, uint8_t *src,
		  uint8_t *dst, uint8_t *data, uint32_t *len);

int j1939_tp(struct j1939_pgn *pgn, const uint8_t priority, const uint8_t src,
	     const uint8_t dst, uint8_t *data, const uint16_t len);

#endif /* __J1939_H__ */
