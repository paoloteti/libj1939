#include <stdbool.h>
#include <stdint.h>
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

int connect_canbus(const char *can_ifname)
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
	cansock = sock;
	return 0;
}

int disconnect_canbus(void)
{
	close(cansock);
}

int j1939_filter(struct j1939_pgn_filter *filter, uint32_t num_filters)
{
	return 0;
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
	return frame.can_dlc;
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
	*id = frame.can_id;
	return frame.can_dlc;
}