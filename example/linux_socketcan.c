#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <bits/time.h>
#include <pthread.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "j1939.h"

extern void j1939_task_yield(void);

static int cansock = -1;

int connect_canbus(const char *can_ifname);
int disconnect_canbus(void);

static inline ssize_t xread(int fd, void *buf, size_t len)
{
	ssize_t nr;
	while (1) {
		nr = read(fd, buf, len);
		if ((nr < 0) && (errno == EAGAIN || errno == EINTR))
			continue;
		return nr;
	}
}

static inline ssize_t xwrite(int fd, const void *buf, size_t len)
{
	ssize_t nr;
	while (1) {
		nr = write(fd, buf, len);
		if ((nr < 0) && (errno == EAGAIN || errno == EINTR))
			continue;
		return nr;
	}
}

int connect_canbus(const char *can_ifname)
{
	int ret, sock;
	struct ifreq ifr;
	struct sockaddr_can addr;

	sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (sock < 0) {
		return sock;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, can_ifname, IFNAMSIZ - 1);
	ioctl(sock, SIOCGIFINDEX, &ifr);

	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		return ret;
	}
	cansock = sock;
	return 0;
}

int disconnect_canbus(void)
{
	return close(cansock);
}

int j1939_filter(struct j1939_pgn_filter *filter, uint32_t num_filters)
{
	struct can_filter rfilter[num_filters];
	uint32_t id;

	for (size_t i = 0; i < num_filters; i++) {
		id = j1939_pgn2id(filter[i].pgn, filter[i].priority,
				  filter[i].addr);
		rfilter[i].can_id = id | CAN_EFF_FLAG;
		rfilter[i].can_mask = filter[i].pgn_mask;
	}
	return setsockopt(cansock, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter,
			  sizeof(rfilter));
}

int j1939_cansend(uint32_t id, uint8_t *data, uint8_t len)
{
	int ret;
	struct can_frame frame;

	frame.can_id = id | CAN_EFF_FLAG;
	frame.can_dlc = len;
	memcpy(frame.data, data, frame.can_dlc);

	ret = xwrite(cansock, &frame, sizeof(frame));
	if (ret != sizeof(frame)) {
		return -1;
	}
	return frame.can_dlc;
}

int j1939_canrcv(uint32_t *id, uint8_t *data)
{
	int ret;
	struct can_frame frame;

	ret = xread(cansock, &frame, sizeof(frame));
	if (ret != sizeof(frame)) {
		return -1;
	}

	memcpy(data, frame.data, frame.can_dlc);
	*id = frame.can_id;
	return frame.can_dlc;
}

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
