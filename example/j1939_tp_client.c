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
#include <linux/can/j1939.h>

#include "j1939.h"


extern int pgn_pool_receive(void);
extern void j1939_task_yield(void);
extern int connect_canbus(const char *can_ifname);
extern void disconnect_canbus(void);

static int stop;

uint32_t j1939_get_time(void)
{
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	return tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
}

void j1939_task_yield(void)
{
	pthread_yield();
}

void *pgn_rx(void *x)
{
	while (!stop) {
		pgn_pool_receive();
	}
	return NULL;
}

static void dump_payload(uint8_t *data, const uint8_t len)
{
	for (uint8_t i = 0; i < len; i++) {
		printf("%x ", data[i]);
	}
	printf("\n");
}

static int rcv_tp_dt(j1939_pgn_t pgn, uint8_t priority,
		     uint8_t src, uint8_t dest,
		      uint8_t *data, uint8_t len)
{
	dump_payload(data, len);
	return 0;
}

static void error_handler(j1939_pgn_t pgn, uint8_t priority,
		     	  uint8_t src, uint8_t dest, int err)
{
	printf("Error: %d\n", err);
}

int main(void)
{
	pthread_t tid;
	int ret, ntimes = 5;
	const uint8_t src = 0x80;
	const uint8_t dest = 0x20;
	uint8_t data[32];
	j1939_pgn_t pgn = J1939_INIT_PGN(0x0, 0xFE, 0xF6);

	if (connect_canbus("vcan0") < 0) {
		perror("Opening CANbus vcan0");
		return 1;
	}

	for (uint8_t i = 0; i < 32; i++) {
		data[i] = i;
	}

	j1939_setup(rcv_tp_dt, error_handler);
	stop = 0;
	pthread_create(&tid, NULL, pgn_rx, NULL);

	ret = j1939_tp(pgn, 6, src, dest, data, sizeof(data));
	if (ret < 0) {
		printf("J1939 TP returns with code %d\n", ret);
		return 1;
	}
	stop = 1;
	disconnect_canbus();
	return 0;
}
