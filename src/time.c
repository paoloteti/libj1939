
#include <stdint.h>
#include <stdbool.h>
#include "j1939_time.h"
#include "j1939.h"

bool elapsed(const uint32_t t, const uint32_t timeout)
{
	uint32_t delta;
	uint32_t now = j1939_get_time();

	if (now >= t) {
		delta = now - t;
	} else {
		delta = UINT32_MAX - t + now;
	}

	return delta > timeout;
}
