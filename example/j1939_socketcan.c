/* SPDX-License-Identifier: Apache-2.0 */

/*
 * NOTE: This is just an example.
 *
 * Linux has is own J1939 kernel module, so there is no need to use
 * this library.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/j1939.h>

#include "j1939.h"

static int cansock;

static int connect_canbus(const char *can_ifname)
{
	int ret, sock;
	struct ifreq ifr;
	struct sockaddr_can addr;

	sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (sock < 0) {
		return sock;
	}

	strncpy(ifr.ifr_name, can_ifname, IFNAMSIZ);
	ioctl(sock, SIOCGIFINDEX, &ifr);

	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		return ret;
	}
	return sock;
}

int j1939_filter(struct j1939_pgn_filter *filter, uint32_t num_filters)
{
	struct j1939_filter filt[num_filters];

	for (int i = 0; i < num_filters; i++) {
		filt[i].pgn = j1939_pgn_to_id(&filter[i].pgn);
		filt[i].pgn_mask = j1939_pgn_to_id(&filter[i].pgn_mask);
	}

	setsockopt(cansock, SOL_CAN_J1939, SO_J1939_FILTER, &filt,
		   sizeof(filt));
}

int j1939_cansend(uint32_t id, uint8_t *data, uint8_t len)
{
	int ret;
	struct can_frame frame;

	frame.can_id = id | CAN_EFF_FLAG;
	frame.can_dlc = len;
	memcpy(frame.data, data, frame.can_dlc);

	ret = write(cansock, &frame, sizeof(frame));
	if (ret != sizeof(frame)) {
		return -1;
	}
	return ret;
}

int j1939_canrcv(uint32_t *id, uint8_t *data)
{
	int ret;
	struct can_frame frame;

	ret = read(cansock, &frame, sizeof(frame));
	if (ret != sizeof(frame)) {
		return -1;
	}

	memcpy(data, frame.data, frame.can_dlc);
	return frame.can_dlc;
}

int main(void)
{
	int ret, ntimes = 5;
	const uint8_t src = 0x80;
	const uint8_t dest = 0x20;
	struct j1939_pgn pgn = J1939_INIT_PGN(0x0, 0xFE, 0xF6);

	uint8_t data[8] = {
		J1930_NA_8, /* Particulate Trap Inlet Pressure (SPN 81) */
		J1930_NA_8, /* Boost Pressure (SPN 102) */
		0x46, /* Intake Manifold 1 Temperature (SPN 105) */
		J1930_NA_8, /* Air Inlet Pressure (SPN 106) */
		J1930_NA_8, /* Air Filter 1 Differential. Pressure (SPN 107) */
		0x00, /* Exhaust Gas Temperature (SPN 173) - MSB */
		0xFF, /* Exhaust Gas Temperature (SPN 173) - LSB */
		J1930_NA_8, /* Coolant Filter Differ. Pressure 112) */
	};

	cansock = connect_canbus("vcan0");
	if (cansock < 0) {
		perror("Opening CANbus vcan0");
		return 1;
	}

	do {
		ret = j1939_tp(&pgn, 6, src, dest, data, 8);
		if (ret < 0) {
			printf("J1939 send returns with code %d\n", ret);
		}
		data[2]++;
	} while (ntimes-- && ret >= 0);

	close(cansock);
	return 0;
}
