/* SPDX-License-Identifier: Apache-2.0 */

/*
 * J1939 Message Construction
 *
 * J1939 messages are built on top of CAN 2.0b and make specific use of
 * extended frames. Extended frames use a 29-bit identifier instead of the
 * common 11-bit identifier.
 *
 * |======================= CAN IDENTIFIER (29 bit) =======================|
 * | Priority [3] | Parameter Group Number (PGN) [18] | Source Address [8] |
 * |-----------------------------------------------------------------------|
 * | EDP [1]  | DP [1]  | PDU Format [8] | PDU Specific / Destination [8]  |
 * |=======================================================================|
 *
 * The first three bits are the priority field.
 * This field sets the message’s priority on the network and helps ensure
 * messages with higher importance are sent/received before lower priority
 * messages. Zero is the highest priority.
 *
 * Using the Extended Data Page bit (EDP) and the Data Page bit (DP),
 * four different "Data pages" for J1939 messages (Parameter Group) can be
 * selected:
 *
 * EDP | DP |  Description
 * =======================================
 *  0  | 0  |  SAE J1939 Parameter Groups
 *  0  | 1  |  NMEA2000 defined
 *  1  | 0  |  SAE J1939 reserved
 *  1  | 1  |  ISO 15765-3 defined
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "j1939.h"
#include "compiler.h"

int j1939_send(const struct j1939_pgn *pgn, const uint8_t priority,
	       const uint8_t src, const uint8_t dst, uint8_t *data,
	       const uint32_t len)
{
	uint32_t id;

	if (!j1939_valid_priority(priority)) {
		return -1;
	}

	id = ((uint32_t)priority << 26) |
	     ((j1939_pgn_to_id(pgn) & 0x3FFFF) << 8) | (uint32_t)src;

	/* If PGN is peer-to-peer, add destination address to the ID */
	if (j1939_pdu_is_p2p(pgn)) {
		id = (id & 0xFFFF00FFu) | ((uint32_t)dst << 8);
	}

	return j1939_cansend(id, data, len);
}

int j1939_receive(struct j1939_pgn *pgn, uint8_t *priority, uint8_t *src,
		  uint8_t *dst, uint8_t *data, uint32_t *len)
{
	uint32_t id;
	int received;

	if (unlikely(!pgn || !priority || !src || !dst || !data || !len)) {
		return -1;
	}

	received = j1939_canrcv(&id, data);

	if (received >= 0) {
		*len = received;
		*priority = (id & 0x1C000000u) >> 26;
		*src = id & 0x000000FFu;
		j1939_pgn_from_id(pgn, id);

		if (j1939_pdu_is_p2p(pgn)) {
			*dst = id & 0x000000FFu;
		}
	}

	return received;
}
