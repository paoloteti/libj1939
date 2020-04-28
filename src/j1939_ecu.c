/* SPDX-License-Identifier: Apache-2.0 */

/**
 * J1939: Electronic Control Unit (ECU) holding one or more
 *        Controller Applications (CAs).
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "compat.h"
#include "compiler.h"
#include "j1939.h"
#include "pgn.h"
#include "pgn_pool.h"
#include "j1939_time.h"
#include <stdio.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define DLC_MAX 8u /*<! CANbus max DLC value */
#define DEFRAG_DLC_MAX (DLC_MAX - 1u)

#define REASON_NONE 0x00u
#define REASON_BUSY 0x01u /*<! Node is busy */
#define REASON_NO_RESOURCE 0x02u /*<! Lacking the necessary resources */
#define REASON_TIMEOUT 0x03u /*<! A timeout occurred */
#define REASON_CTS_WHILE_DT 0x04u /*<! CTS received when during transfer */
#define REASON_INCOMPLETE 0x05u

#define CONN_MODE_RTS 0x10u
#define CONN_MODE_CTS 0x11u
#define CONN_MODE_EOM_ACK 0x13u
#define CONN_MODE_BAM 0x20u
#define CONN_MODE_ABORT 0xFFu

static int cts_done;
static uint8_t cts_num_packets;
static uint8_t cts_next_packet;
static int eom_ack;
static uint16_t eom_ack_size;
static uint8_t eom_ack_num_packets;
static uint8_t tp_num_packets;
static uint16_t tp_tot_size;
static pgn_callback_t user_rcv_tp_callback;
static pgn_error_cb_t user_error_cb;

__weak void j1939_task_yield(void);

static inline uint8_t num_packet_from_size(uint8_t size)
{
	uint8_t min = size / DEFRAG_DLC_MAX;
	return (size % DEFRAG_DLC_MAX) == 0 ? min : min + 1;
}

static int send_tp_rts(uint8_t priority, uint8_t src, uint8_t dst,
		       uint16_t size, uint8_t num_packets)
{
	uint8_t data[DLC_MAX] = {
		CONN_MODE_RTS,
		size & 0xFF,
		size >> 8,
		num_packets,
		0xFF,
		PGN_SPECIFIC(TP_CM),
		PGN_FORMAT(TP_CM),
		PGN_DATA_PAGE(TP_CM),
	};

	return j1939_send(TP_CM, priority, src, dst, data, ARRAY_SIZE(data));
}

static int defrag_send(uint16_t size, const uint8_t priority, const uint8_t src,
		       const uint8_t dest, uint8_t *data)
{
	int ret;
	uint8_t seqno = 0;
	uint8_t frame[DLC_MAX];

	while (size > 0) {
		frame[0] = seqno;
		seqno++;

		if (size >= DEFRAG_DLC_MAX) {
			memcpy(&frame[1], data, DEFRAG_DLC_MAX);
			size -= DEFRAG_DLC_MAX;
			data += DEFRAG_DLC_MAX;
		} else {
			memcpy(&frame[1], data, size);
			memset(&frame[1 + size], J1930_NA_8,
			       DEFRAG_DLC_MAX - size);
			size = 0;
		}

		ret = j1939_send(TP_DT, priority, src, dest, frame,
				 ARRAY_SIZE(frame));
		if (ret < 0) {
			return ret;
		}

		uint32_t now = j1939_get_time();
		while (!elapsed(now, SEND_PERIOD)) {
#if defined(TP_TASK_YIELD)
			j1939_task_yield();
#endif
		}
	}
	return 0;
}

int send_tp_bam(const uint8_t priority, const uint8_t src, uint8_t *data,
		const uint16_t len)
{
	int ret;
	uint8_t num_packets = num_packet_from_size(len);
	uint8_t bam[DLC_MAX] = {
		CONN_MODE_BAM,
		len & 0x00FF,
		len >> 8,
		num_packets,
		0xFF,
		PGN_SPECIFIC(BAM),
		PGN_FORMAT(BAM),
		PGN_DATA_PAGE(BAM),
	};

	if (unlikely(len > J1939_MAX_DATA_LEN)) {
		return -EARGS;
	}

	ret = j1939_send(TP_CM, priority, src, ADDRESS_GLOBAL, bam, DLC_MAX);
	if (ret < 0) {
		return ret;
	}

	return defrag_send(len, priority, src, ADDRESS_GLOBAL, data);
}

static int send_abort(const uint8_t src, const uint8_t dst,
		      const uint8_t reason)
{
	uint8_t data[DLC_MAX] = {
		CONN_MODE_ABORT,
		reason,
		0xFF,
		0xFF,
		0xFF,
		PGN_SPECIFIC(TP_CM),
		PGN_FORMAT(TP_CM),
		PGN_DATA_PAGE(TP_CM),
	};
	return j1939_send(TP_DT, J1939_PRIORITY_LOW, src, dst, data,
			  ARRAY_SIZE(data));
}

static int tp_cts_received(j1939_pgn_t pgn, uint8_t priority, uint8_t src,
			   uint8_t dest, uint8_t *data, uint8_t len)
{
	cts_num_packets = data[1];
	cts_next_packet = data[2];
	cts_done = 1;
	return 0;
}

static uint8_t wait_tp_cts(const j1939_pgn_t pgn, const uint8_t src)
{
	cts_done = 0;

	uint32_t now = j1939_get_time();
	while (!elapsed(now, T3) && !cts_done) {
#if defined(TP_TASK_YIELD)
		j1939_task_yield();
#endif
	}
	return cts_done ? REASON_NONE : REASON_TIMEOUT;
}

static int tp_eom_ack_received(j1939_pgn_t pgn, uint8_t priority, uint8_t src,
			       uint8_t dest, uint8_t *data, uint8_t len)
{
	eom_ack_size = (data[1] << 8) | (data[2]);
	eom_ack_num_packets = data[3];
	eom_ack = 1;
	return 0;
}

static int wait_tp_eom_ack(const j1939_pgn_t pgn, const uint8_t src,
			   const uint16_t size, const uint8_t npackets)
{
	eom_ack = 0;

	uint32_t now = j1939_get_time();
	while (!elapsed(now, T3) && !eom_ack) {
#if defined(TP_TASK_YIELD)
		j1939_task_yield();
#endif
	}

	if (!eom_ack) {
		return -ETIMEOUT;
	}
	if (eom_ack_size == size && eom_ack_num_packets == npackets) {
		return -EINCOMPLETE;
	}
	return 0;
}

static int send_tp_eom_ack(const uint8_t src, const uint8_t dst,
			   const uint16_t size, const uint8_t num_packets)
{
	uint8_t data[DLC_MAX] = {
		CONN_MODE_EOM_ACK,
		size & 0x00FF,
		(size >> 8),
		num_packets,
		0xFF,
		PGN_SPECIFIC(TP_CM),
		PGN_FORMAT(TP_CM),
		PGN_DATA_PAGE(TP_CM),
	};
	return j1939_send(TP_CM, J1939_PRIORITY_LOW, src, dst, data,
			  ARRAY_SIZE(data));
}

int j1939_tp(j1939_pgn_t pgn, const uint8_t priority, const uint8_t src,
	     const uint8_t dst, uint8_t *data, const uint16_t len)
{
	int ret;
	uint8_t num_packets, reason;
	uint16_t size;

	if (unlikely(len > J1939_MAX_DATA_LEN)) {
		return -J1939_EWRONG_DATA_LEN;
	}

	/* single frame, send directly */
	if (len <= DLC_MAX) {
		return j1939_send(pgn, priority, src, dst, data, len);
	}

	num_packets = num_packet_from_size(len);

	/* Send Request To Send (RTS) */
	ret = send_tp_rts(priority, src, dst, len, num_packets);
	if (unlikely(ret < 0)) {
		return ret;
	}

	while (num_packets > 0) {
		/* Wait Clear To Send (CTS) */
		reason = wait_tp_cts(TP_CM, dst);
		if (unlikely(reason != REASON_NONE)) {
			return send_abort(src, dst, reason);
		}

		num_packets -= cts_num_packets;
		size = MIN(cts_num_packets * DEFRAG_DLC_MAX, len);
		ret = defrag_send(size, J1939_PRIORITY_LOW, src, dst, data);
		if (unlikely(ret < 0)) {
			return ret;
		}
	}
	return send_tp_eom_ack(src, dst, len, num_packets);
}

int j1939_address_claimed(uint8_t src, ecu_name_t name)
{
	const uint8_t dest = 0xFE;
	uint64_t n = htobe64(name.value);
	return j1939_send(AC, J1939_PRIORITY_HIGH, src, dest, (uint8_t *)&n,
			  DLC_MAX);
}

int j1939_cannot_claim_address(ecu_name_t name)
{
	uint64_t n = htobe64(name.value);
	return j1939_send(AC, J1939_PRIORITY_DEFAULT, ADDRESS_NOT_CLAIMED,
			  ADDRESS_GLOBAL, (uint8_t *)&n, 8);
}

int j1939_address_claim(const uint8_t src, ecu_name_t name)
{
	int ret;
	uint32_t ac = AC;

	/* Send Request for Address Claimed */
	ret = j1939_send(RAC, J1939_PRIORITY_DEFAULT, src, ADDRESS_GLOBAL,
			 (uint8_t *)&ac, 3);
	if (unlikely(ret < 0)) {
		return ret;
	}

	uint64_t n = htobe64(name.value);
	return j1939_send(AC, J1939_PRIORITY_DEFAULT, src, ADDRESS_GLOBAL,
			  (uint8_t *)&n, DLC_MAX);
}

int j1939_send_tp_cts(const uint8_t src, const uint8_t dst,
		      const uint8_t num_packets, const uint8_t next_packet)
{
	uint8_t data[DLC_MAX] = {
		CONN_MODE_CTS,
		num_packets,
		next_packet,
		0xFF,
		0xFF,
		PGN_SPECIFIC(TP_CM),
		PGN_FORMAT(TP_CM),
		PGN_DATA_PAGE(TP_CM),
	};
	return j1939_send(TP_CM, J1939_PRIORITY_LOW, src, dst, data,
			  ARRAY_SIZE(data));
}

static int pgn_abort(j1939_pgn_t pgn, uint8_t priority, uint8_t src,
		     uint8_t dest, uint8_t *data, uint8_t len)
{
	if (user_error_cb) {
		user_error_cb(pgn, priority, src, dest, data[0]);
	}
	return 0;
}

static int request_to_send(j1939_pgn_t pgn, uint8_t priority, uint8_t src,
			   uint8_t dest, uint8_t *data, uint8_t len)
{
	tp_tot_size = htobe16((data[1] << 8) | data[2]);
	tp_num_packets = num_packet_from_size(tp_tot_size);
	return j1939_send_tp_cts(src, dest, tp_num_packets, 0);
}

static int _rcv_tp(j1939_pgn_t pgn, uint8_t priority, uint8_t src, uint8_t dest,
		   uint8_t *data, uint8_t len)
{
	int ret = 0;

	if (tp_num_packets == 0) {
		ret = wait_tp_eom_ack(pgn, src, tp_tot_size, tp_num_packets);
	}
	tp_num_packets--;

	if (user_rcv_tp_callback) {
		user_rcv_tp_callback(pgn, priority, src, dest, data, len);
	}

	if (ret < 0 && user_error_cb) {
		user_error_cb(pgn, priority, src, dest, ret);
	}
	return ret;
}

int j1939_setup(pgn_callback_t rcv_tp, pgn_error_cb_t err_cb)
{
	user_rcv_tp_callback = rcv_tp;
	user_error_cb = err_cb;

	pgn_register(TP_CM, CONN_MODE_CTS, tp_cts_received);
	pgn_register(TP_CM, CONN_MODE_ABORT, pgn_abort);
	pgn_register(TP_CM, CONN_MODE_RTS, request_to_send);
	pgn_register(TP_CM, CONN_MODE_EOM_ACK, tp_eom_ack_received);
	pgn_register(TP_DT, 0, _rcv_tp);
	return 0;
}

int j1939_dispose(void)
{
	pgn_deregister_all();
	return 0;
}

__weak void j1939_task_yield(void)
{
}
