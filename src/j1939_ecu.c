/* SPDX-License-Identifier: Apache-2.0 */

/**
 * J1939: Electronic Control Unit (ECU) holding one or more
 *        Controller Applications (CAs).
 */
#include <endian.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "compiler.h"
#include "j1939.h"

#define CONN_MODE_RTS 0x10u
#define CONN_MODE_CTS 0x11u
#define CONN_MODE_EOM_ACK 0x13u
#define CONN_MODE_BAM 0x20u
#define CONN_MODE_ABORT 0xFFu

static const struct j1939_pgn BAM = J1939_INIT_PGN(0x00u, 0xFEu, 0xEC);
static const struct j1939_pgn TP_CM = J1939_INIT_PGN(0x00u, 0xECu, 0x00u);
static const struct j1939_pgn TP_DT = J1939_INIT_PGN(0x00u, 0xEBu, 0x00u);
/** @brief Address Claimed */
static const struct j1939_pgn AC = J1939_INIT_PGN(0x00u, 0xEE, 0x00u);
/** @brief Request for Address Claimed */
static const struct j1939_pgn RAC = J1939_INIT_PGN(0x00u, 0xEA, 0x00u);

static int send_tp_rts(uint8_t priority, uint8_t src, uint8_t dst,
		       uint16_t size, uint8_t num_packets)
{
	uint8_t data[8] = {
		CONN_MODE_RTS,
		size & 0xFF,
		size >> 8,
		num_packets,
		0xFF,
		TP_CM.pdu_specific,
		TP_CM.pdu_format,
		TP_CM.data_page,
	};

	return j1939_send(&TP_CM, priority, src, dst, data, 8);
}

static int send_tp_dt(const uint8_t src, const uint8_t dst, uint8_t *data,
		      const uint32_t len)
{
	return j1939_send(&TP_DT, J1939_PRIORITY_LOW, src, dst, data, len);
}

int send_tp_bam(const uint8_t priority, const uint8_t src, uint8_t *data,
		const uint16_t len)
{
	static int step = 0;
	int ret;
	static uint16_t size;
	static uint8_t seqno = 0;
	uint8_t frame[8];
	uint8_t num_packets = (len / 7) + (len % 7);
	uint8_t bam[8] = {
		CONN_MODE_BAM,
		len & 0x00FF,
		len >> 8,
		num_packets,
		0xFF,
		BAM.pdu_specific,
		BAM.pdu_format,
		BAM.data_page,
	};

	if (unlikely(len > J1939_MAX_DATA_LEN)) {
		return -EARGS;
	}

	if (step == 0) {
		ret = j1939_send(&TP_CM, priority, src, ADDRESS_GLOBAL, bam, 8);
		if (ret < 0) {
			return ret;
		}
		step++;
		size = len;
		return -ECONTINUE;
	}

	while (size > 0) {
		frame[0] = seqno;

		if (size >= 7) {
			memcpy(&frame[1], data, 7);
			size -= 7;
		} else {
			memcpy(&frame[1], data, size);
			memset(&frame[1 + size], J1930_NA_8, 7 - size);
			size = 0;
		}

		ret = j1939_send(&TP_DT, priority, src, ADDRESS_GLOBAL, frame, 8);
		if (ret < 0) {
			return ret;
		}
		seqno++;
		step++;
		return -ECONTINUE;
	}
	return 0;
}

static int send_abort(const uint8_t src, const uint8_t dst,
		      const uint8_t reason)
{
	uint8_t data[8] = {
		CONN_MODE_ABORT,
		reason,
		0xFF,
		0xFF,
		0xFF,
		TP_CM.pdu_specific,
		TP_CM.pdu_format,
		TP_CM.data_page,
	};
	return j1939_send(&TP_DT, J1939_PRIORITY_LOW, src, dst, data,
			  ARRAY_SIZE(data));
}

static int send_tp_cts(const uint8_t src, const uint8_t dst,
		       const uint8_t num_packets, const uint8_t next_packet)
{
	uint8_t data[8] = {
		CONN_MODE_CTS,
		num_packets,
		next_packet,
		0xFF,
		0xFF,
		TP_CM.pdu_specific,
		TP_CM.pdu_format,
		TP_CM.data_page,
	};
	return j1939_send(&TP_DT, J1939_PRIORITY_LOW, src, dst, data,
			  ARRAY_SIZE(data));
}

static int send_tp_eom_ack(const uint8_t src, const uint8_t dst,
			   const uint16_t size, const uint8_t num_packets)
{
	uint8_t data[8] = {
		CONN_MODE_EOM_ACK,
		size & 0x00FF,
		(size >> 8),
		num_packets,
		0xFF,
		TP_CM.pdu_specific,
		TP_CM.pdu_format,
		TP_CM.data_page,
	};
	return j1939_send(&TP_CM, J1939_PRIORITY_LOW, src, dst, data,
			  ARRAY_SIZE(data));
}

int j1939_tp(struct j1939_pgn *pgn, const uint8_t priority, const uint8_t src,
	     const uint8_t dst, uint8_t *data, const uint16_t len)
{
	int ret;
	uint8_t *block = data;
	uint16_t num_packets, odd_packet;

	if (unlikely(len > J1939_MAX_DATA_LEN)) {
		return -1;
	}

	/* single frame, send directly */
	if (len <= 8) {
		return j1939_send(pgn, priority, src, dst, data, len);
	}

	num_packets = len / 8u;
	odd_packet = len % 8u;

	ret = send_tp_rts(priority, src, dst, len, num_packets);
	if (unlikely(ret < 0)) {
		return ret;
	}

	while (num_packets > 0) {
		ret = send_tp_dt(src, dst, block, 8);
		if (unlikely(ret < 0)) {
			return ret;
		}
		num_packets--;
		block += 8;
	}

	if (odd_packet) {
		ret = send_tp_dt(src, dst, block, odd_packet);
		if (unlikely(ret < 0)) {
			return ret;
		}
	}

	return send_tp_eom_ack(src, dst, 8, num_packets + odd_packet);
}

int j1939_address_claimed(uint8_t src, ecu_name_t name)
{
	const uint8_t dest = 0xFE;
	uint64_t n = htobe64(name.value);
	return j1939_send(&AC, J1939_PRIORITY_HIGH, src, dest, (uint8_t *)&n, 8);
}

int j1939_cannot_claim_address(ecu_name_t name)
{
	uint64_t n = htobe64(name.value);
	return j1939_send(&AC, J1939_PRIORITY_DEFAULT, ADDRESS_NOT_CLAIMED,
			  ADDRESS_GLOBAL, (uint8_t *)&n, 8);
}

int j1939_address_claim(const uint8_t src, ecu_name_t name)
{
	int ret;

	/* Send Request for Address Claimed */
	ret = j1939_send(&RAC, J1939_PRIORITY_DEFAULT, src, ADDRESS_GLOBAL,
			 (uint8_t *)&AC, 3);
	if (unlikely(ret < 0)) {
		return ret;
	}

	uint64_t n = htobe64(name.value);
	return j1939_send(&AC, J1939_PRIORITY_DEFAULT, src, ADDRESS_GLOBAL,
			  (uint8_t *)&n, 8);
}
