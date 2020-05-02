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
#include <pthread.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "j1939.h"

extern int pgn_pool_receive(void);
extern int connect_canbus(const char *can_ifname);
extern void disconnect_canbus(void);
extern uint32_t j1939_get_time(void);

static int stop = 0;

static void *pgn_rx(void *x)
{
	while (!stop) {
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
	printf("[%02x %02x]: ", src, dest);
	dump_payload(data, len);
	return 0;
}

static void error_handler(j1939_pgn_t pgn, uint8_t priority, uint8_t src,
			  uint8_t dest, int err)
{
	printf("[%02x %02x] ERROR: %d\n", src, dest, err);
#ifdef STOP_ON_ERROR
	stop = 1;
#endif
}

int main(void)
{
	pthread_t tid;

	if (connect_canbus("vcan0") < 0) {
		perror("Opening CANbus vcan0");
		return 1;
	}

	j1939_setup(rcv_tp_dt, error_handler);

	pthread_create(&tid, NULL, pgn_rx, NULL);

	pthread_join(tid, NULL);

	disconnect_canbus();
	return 0;
}
