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
#include <time.h>
#include <bits/time.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "j1939.h"

extern int connect_canbus(const char *can_ifname);
extern void disconnect_canbus(void);

uint32_t j1939_get_time(void)
{
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	return tv.tv_sec * 1000 + tv.tv_nsec / 1000;
}

int main(void)
{
	int ret, ntimes = 5;
	const uint8_t src = 0x80;
	const uint8_t dest = 0x20;
	j1939_pgn_t pgn = J1939_INIT_PGN(0x0, 0xFE, 0xF6);
	uint8_t bam_data[18];

	uint8_t data[8] = {
		J1930_NA_8, /* Particulate Trap Inlet Pressure (SPN 81) */
		J1930_NA_8, /* Boost Pressure (SPN 102) */
		0x46, /* Intake Manifold 1 Temperature (SPN 105) */
		J1930_NA_8, /* Air Inlet Pressure (SPN 106) */
		J1930_NA_8, /* Air Filter 1 Differential. Pressure (SPN 107) */
		J1930_NA_16_MSB, /* Exhaust Gas Temperature (SPN 173) - MSB */
		J1930_NA_16_LSB, /* Exhaust Gas Temperature (SPN 173) - LSB */
		J1930_NA_8, /* Coolant Filter Differ. Pressure 112) */
	};

	ecu_name_t name = {
		.fields.arbitrary_address_capable = J1939_NO_ADDRESS_CAPABLE,
		.fields.industry_group = J1939_INDUSTRY_GROUP_INDUSTRIAL,
		.fields.vehicle_system_instance = 1,
		.fields.vehicle_system = 1,
		.fields.function = 1,
		.fields.reserved = 0,
		.fields.function_instance = 1,
		.fields.ecu_instance = 1,
		.fields.manufacturer_code = 1,
		.fields.identity_number = 1,
	};

	if (connect_canbus("vcan0") < 0) {
		perror("Opening CANbus vcan0");
		return 1;
	}

	ret = j1939_address_claim(src, name);
	if (ret < 0) {
		printf("J1939 AC returns with code %d\n", ret);
	}

	j1939_address_claimed(src, name);

	do {
		ret = j1939_tp(pgn, 6, src, dest, data, 8);
		if (ret < 0) {
			printf("J1939 TP returns with code %d\n", ret);
		}
		data[2]++;
	} while (ntimes-- && ret >= 0);

	memset(bam_data, 0xAA, sizeof(bam_data));

	ret = send_tp_bam(6, src, bam_data, sizeof(bam_data));
	if (ret < 0) {
		printf("J1939 BAM returns with code %d\n", ret);
	}

	disconnect_canbus();
	return 0;
}
