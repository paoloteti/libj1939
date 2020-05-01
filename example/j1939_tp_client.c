/* SPDX-License-Identifier: Apache-2.0 */

/*
 * NOTE: This is just an example.
 *
 * Linux has its own J1939 kernel module, so there is no need to use
 * this library.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <bits/time.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "j1939.h"

#define DEST 0x80u

extern int pgn_pool_receive(void);
extern void j1939_task_yield(void);
extern int connect_canbus(const char *can_ifname);
extern void disconnect_canbus(void);
extern uint32_t j1939_get_time(void);

static j1939_pgn_t PGN = J1939_INIT_PGN(0x0, 0xFE, 0xF6);

static void *pgn_rx(void *x)
{
	while (1) {
		pgn_pool_receive();
	}
	return NULL;
}

static void dump_payload(uint8_t *data, const uint8_t len)
{
	for (uint8_t i = 0; i < len; i++) {
		printf("%02x ", data[i]);
	}
	printf("\n");
}

static int rcv_tp_dt(j1939_pgn_t pgn, uint8_t priority, uint8_t src,
		     uint8_t dest, uint8_t *data, uint8_t len)
{
	dump_payload(data, len);
	return 0;
}

static void error_handler(j1939_pgn_t pgn, uint8_t priority, uint8_t src,
			  uint8_t dest, int err)
{
	printf("[%02x %02x] ERROR: %d\n", src, dest, err);
}

static void *sender(void *x)
{
	uint8_t sess = 0;
	uint8_t data[32];
	int ret, ntimes = 32;
	uint8_t src = *((uint8_t *)x);

	do {
		memset(data, sess, sizeof(data));
		sess++;
	retry:
		ret = j1939_tp(PGN, 6, src, DEST, data, sizeof(data));
		if (ret < 0) {
			if (ret == -J1939_EBUSY) {
				usleep(500000);
				goto retry;
			}
			printf("J1939 TP returns with code %d\n", ret);
		}

		/* Add src to have different periods */
		usleep(1000000 + src);
	} while (ntimes-- && ret >= 0);
	pthread_exit(NULL);
}

int main(void)
{
	uint8_t src1 = 0x10;
	uint8_t src2 = 0x20;
	uint8_t src3 = 0x30;
	pthread_t tid, s1, s2, s3;

	if (connect_canbus("vcan0") < 0) {
		perror("Opening CANbus vcan0");
		return 1;
	}

	j1939_setup(rcv_tp_dt, error_handler);

	pthread_create(&tid, NULL, pgn_rx, NULL);
	pthread_detach(tid);

	pthread_create(&s1, NULL, sender, &src1);
	pthread_create(&s2, NULL, sender, &src2);
	pthread_create(&s3, NULL, sender, &src3);

	pthread_join(s1, NULL);
	pthread_join(s2, NULL);
	pthread_join(s3, NULL);

	disconnect_canbus();
	return 0;
}
