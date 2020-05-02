/* SPDX-License-Identifier: Apache-2.0 */

/**
 * J1939: Electronic Control Unit (ECU) holding one or more
 *        Controller Applications (CAs).
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "atomic.h"
#include "compat.h"
#include "compiler.h"
#include "j1939.h"
#include "pgn.h"
#include "pgn_pool.h"
#include "j1939_time.h"
#include "session.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define DLC_MAX 8u /*<! CANbus max DLC value */
#define DEFRAG_DLC_MAX (DLC_MAX - 1u)

#define REASON_NONE 		0x00u /*<! No Errors */
#define REASON_BUSY 		0x01u /*<! Node is busy */
#define REASON_NO_RESOURCE 	0x02u /*<! Lacking the necessary resources */
#define REASON_TIMEOUT 		0x03u /*<! A timeout occurred */
#define REASON_CTS_WHILE_DT 	0x04u /*<! CTS received when during transfer */
#define REASON_INCOMPLETE 	0x05u /*<! Incomplete transfer */

#define CONN_MODE_RTS 		0x10u
#define CONN_MODE_CTS 		0x11u
#define CONN_MODE_EOM_ACK 	0x13u
#define CONN_MODE_BAM 		0x20u
#define CONN_MODE_ABORT 	0xFFu

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
		return -J1939_EARGS;
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
	struct j1939_session *sess = j1939_session_search_addr(dest, src);
	if (sess == NULL) {
		return -1;
	}

	sess->cts_num_packets = data[1];
	sess->cts_next_packet = data[2];
	atomic_set(&sess->cts_done, 1);
	return 1;
}

static uint8_t wait_tp_cts(const j1939_pgn_t pgn, const uint8_t src,
			   const uint8_t dst)
{
	uint8_t ret;
	struct j1939_session *sess = j1939_session_search_addr(src, dst);
	if (sess == NULL) {
		return REASON_NO_RESOURCE;
	}

	sess->timeout = j1939_get_time();
	while (!elapsed(sess->timeout, T3) && !atomic_get(&sess->cts_done)) {
#if defined(TP_TASK_YIELD)
		j1939_task_yield();
#endif
	}

	ret = atomic_get(&sess->cts_done) ? REASON_NONE : REASON_TIMEOUT;
	atomic_set(&sess->cts_done, 0);
	return ret;
}

static int tp_eom_ack_received(j1939_pgn_t pgn, uint8_t priority, uint8_t src,
			       uint8_t dest, uint8_t *data, uint8_t len)
{
	int ret = 0;
	struct j1939_session *sess = j1939_session_search_addr(src, dest);
	if (sess == NULL) {
		ret = -J1939_ENO_RESOURCE;
		goto err;
	}

	if (elapsed(sess->timeout, T3)) {
		ret = -J1939_ETIMEOUT;
		goto err;
	}

	atomic_set(&sess->eom_ack, 1);
	uint8_t eom_ack_size = (data[1] << 8) | (data[2]);
	uint8_t eom_ack_num_packets = data[3];

	if (sess->eom_ack_size != eom_ack_size ||
	    sess->eom_ack_num_packets != eom_ack_num_packets) {
		ret = -J1939_EINCOMPLETE;
		goto err;
	}

	j1939_session_close(src, dest);
	return 0;

err:
	if (user_error_cb) {
		user_error_cb(pgn, priority, src, dest, ret);
	}
	return ret;
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
	bool initiated = false;
	int ret;
	struct j1939_session *sess;
	uint8_t num_packets, reason;
	uint16_t size;

	if (unlikely(len > J1939_MAX_DATA_LEN)) {
		return -J1939_EWRONG_DATA_LEN;
	}

	/* single frame, send directly */
	if (len <= DLC_MAX) {
		return j1939_send(pgn, priority, src, dst, data, len);
	}

	sess = j1939_session_open(src, dst);
	if (sess == NULL) {
		return -J1939_ENO_RESOURCE;
	}

	num_packets = num_packet_from_size(len);

	sess->eom_ack_num_packets = num_packets;
	sess->eom_ack_size = len;

	/* Send Request To Send (RTS) */
	ret = send_tp_rts(priority, src, dst, len, num_packets);
	if (unlikely(ret < 0)) {
		goto out;
	}

	while (num_packets > 0) {
		/* Wait Clear To Send (CTS) */
		reason = wait_tp_cts(TP_CM, src, dst);
		if (unlikely(reason != REASON_NONE)) {
			if (initiated) {
				ret = send_abort(src, dst, reason);
			} else {
				ret = -J1939_EBUSY;
			}
			goto out;
		} else {
			initiated = true;
		}

		num_packets -= sess->cts_num_packets;
		size = MIN(sess->cts_num_packets * DEFRAG_DLC_MAX, len);
		ret = defrag_send(size, J1939_PRIORITY_LOW, src, dst, data);
		if (unlikely(ret < 0)) {
			goto out;
		}
	}
	ret = send_tp_eom_ack(src, dst, len, num_packets);

out:
	j1939_session_close(src, dst);
	return ret;
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
		user_error_cb(pgn, priority, src, dest, data[1]);
	}
	return 0;
}

static int request_to_send(j1939_pgn_t pgn, uint8_t priority, uint8_t src,
			   uint8_t dest, uint8_t *data, uint8_t len)
{
	struct j1939_session *sess = j1939_session_open(src, dest);
	if (sess == NULL) {
		return -1;
	}

	sess->tp_tot_size = htobe16((data[1] << 8) | data[2]);
	sess->tp_num_packets = num_packet_from_size(sess->tp_tot_size);
	return j1939_send_tp_cts(dest, src, sess->tp_num_packets, 0);
}

static int _rcv_tp(j1939_pgn_t pgn, uint8_t priority, uint8_t src, uint8_t dest,
		   uint8_t *data, uint8_t len)
{
	struct j1939_session *sess = j1939_session_search_addr(src, dest);

	if (sess == NULL) {
		return -1;
	}

	if (sess->tp_num_packets == 1) {
		/* Next packet expected to be EOM ACK */
		atomic_set(&sess->eom_ack, 0);
		sess->timeout = j1939_get_time();
	}
	sess->tp_num_packets--;

	if (user_rcv_tp_callback) {
		user_rcv_tp_callback(pgn, priority, src, dest, data, len);
	}

	return 0;
}

int j1939_setup(pgn_callback_t rcv_tp, pgn_error_cb_t err_cb)
{
	user_rcv_tp_callback = rcv_tp;
	user_error_cb = err_cb;

	pgn_pool_init();
	pgn_register(TP_CM, CONN_MODE_CTS, tp_cts_received);
	pgn_register(TP_CM, CONN_MODE_ABORT, pgn_abort);
	pgn_register(TP_CM, CONN_MODE_RTS, request_to_send);
	pgn_register(TP_CM, CONN_MODE_EOM_ACK, tp_eom_ack_received);
	pgn_register(TP_DT, 0, _rcv_tp);

	j1939_session_init();
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
