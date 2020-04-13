/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __J1939_H__
#define __J1939_H__

#define ADDRESS_GLOBAL 0xFFu
#define ADDRESS_NULL 0xEFu

#define J1939_PRIORITY_HIGH 0x0u
#define J1939_PRIORITY_LOW 0x7u

#define REASON_BUSY 0x01u /*<! Node is busy */
#define REASON_NO_RESOURCE 0x02u /*<! Lacking the necessary resources */
#define REASON_TIMEOUT 0x03u /*<! A timeout occurred */
#define REASON_CTS_WHILE_DT 0x04u /*<! CTS received when during transfer */

/** @brief indicates that the parameter is "not available" */
#define J1930_NOT_AVAILABLE_8 0xFFu
#define J1930_NA_8 J1930_NOT_AVAILABLE_8
#define J1930_NOT_AVAILABLE_16 0xFF00u
#define J1930_NA_16 J1930_NOT_AVAILABLE_16
#define J1930_NA_16_MSB (J1930_NOT_AVAILABLE_16 >> 8)
#define J1930_NA_16_LSB (J1930_NOT_AVAILABLE_16 & 0xFF)

/** @brief indicates that the parameter is "not valid" or "in error" */
#define J1930_NOT_VALID_8 0xFEu
#define J1930_NV_8 J J1930_NOT_VALID_8
#define J1930_NOT_VALID_16 0xFE00u
#define J1930_NV_16 J1930_NOT_VALID_16

/** @brief raw parameter values must not exceed the following values */
#define J1930_MAX_8 0xFAu
#define J1930_MAX_16 0xFAFFu

#define J1939_INDUSTRY_GROUP_GLOBAL 0u
#define J1939_INDUSTRY_GROUP_ON_HIGHWAY 1u
#define J1939_INDUSTRY_GROUP_AGRICULTURAL 2u
#define J1939_INDUSTRY_GROUP_CONSTRUCTION 3u
#define J1939_INDUSTRY_GROUP_MARINE 4u
#define J1939_INDUSTRY_GROUP_INDUSTRIAL 5u

#define J1939_NO_ADDRESS_CAPABLE 0u
#define J1939_ADDRESS_CAPABLE 1u

struct j1939_name {
	/*
	 * 21-bit Identity Number
	 * A unique number which identifies the particular device in a
	 * manufacturer specific way.
	 */
	uint64_t identity_number : 21;
	/*
	 * 11-bit Manufacturer Code
	 * One of the predefined J1939 manufacturer codes.
	 */
	uint64_t manufacturer_code : 11;
	/*
	 * 3-bit ECU Instance
	 * Identify the ECU instance if multiple ECUs are involved in
	 * performing a single function. Normally set to 0.
	 */
	uint64_t ecu_instance : 3;
	/*
	 * 5-bit Function Instance
	 * Instance number of a function to distinguish two or more devices
	 * with the same function number in the same J1939 network.
	 * The first instance is assigned to the instance number 0.
	 */
	uint64_t function_instance : 5;
	/*
	 * 8-bit Function
	 * One of the predefined J1939 functions. The same function value
	 * (upper 128 only) may mean different things for different Industry
	 * Groups or Vehicle Systems.
	 */
	uint64_t function : 8;
	/*
	 * 1-bit Reserved
	 * This field is reserved for future use by SAE.
	 */
	uint64_t reserved : 1;
	/*
	 * 7-bit Vehicle System
	 * A subcomponent of a vehicle, that includes one or more J1939
	 * segments and may be connected or disconnected from the vehicle.
	 * A Vehicle System may be made of one or more functions. The Vehicle
	 * System depends on the Industry Group definition.
	 */
	uint64_t vehicle_system : 7;
	/*
	 * 4-bit Vehicle System Instance
	 * Instance number of a vehicle system to distinguish two or more
	 * device with the same Vehicle System number in the same J1939
	 * network.
	 * The first instance is assigned to the instance number 0.
	 */
	uint64_t vehicle_system_instance : 4;
	/*
	 * 3-bit Industry Group
	 * One of the predefined J1939 industry groups.
	 */
	uint64_t industry_group : 3;
	/*
	 * 1-bit Arbitrary Address Capable
	 * Indicate the capability to solve address conflicts.
	 * Set to 1 if the device is Arbitrary Address Capable, set to 0 if
	 * it's Single Address Capable.
	 */
	uint64_t arbitrary_address_capable : 1;
};

struct j1939_pgn {
	uint8_t data_page;
	uint8_t pdu_format;
	uint8_t pdu_specific;
};

struct j1939_pgn_filter {
	struct j1939_pgn pgn;
	struct j1939_pgn pgn_mask;
	uint8_t addr;
	uint8_t addr_mask;
};

/** @brief Timeouts ([msec]) according SAE J1939/21 */
enum j1939_timeouts {
	/* Response Time */
	Tr = 200,
	/* Holding Time */
	Th = 500,
	T1 = 750,
	T2 = 1250,
	T3 = 1250,
	T4 = 1050,
	/* timeout for multi packet broadcast messages 50..200ms */
	Tb = 50,
};

/** @brief Initialize Parameter Group Number using 3 bytes */
#define J1939_INIT_PGN(_dp, _format, _specific)                                \
	{                                                                      \
		.data_page = _dp & 0x01u,                                      \
		.pdu_format = _format,                                         \
		.pdu_specific = _specific,                                     \
	}

extern int j1939_cansend(uint32_t id, uint8_t *data, uint8_t len);
extern int j1939_canrcv(uint32_t *id, uint8_t *data);
extern int j1939_filter(struct j1939_pgn_filter *filter, uint32_t num_filters);

bool static inline j1939_valid_priority(const uint8_t p)
{
	return p <= J1939_PRIORITY_LOW;
}

/** @brief Check if PDU format < 240 (peer-to-peer) */
static inline bool j1939_pdu_is_p2p(const struct j1939_pgn *pgn)
{
	return pgn->pdu_format < 240u;
}

/** @brief Check if PDU format >= 240 (broadcast) */
static inline bool j1939_pdu_is_broadcast(const struct j1939_pgn *pgn)
{
	return !j1939_pdu_is_p2p(pgn);
}

static inline void j1939_pgn_from_id(struct j1939_pgn *pgn, const uint32_t id)
{
	pgn->pdu_format = (id >> 16) & 0x000000FFu;
	pgn->pdu_specific = (id >> 8) & 0x000000FFu;
	pgn->data_page = (id >> 24) & 0x00000001u;
}

static inline uint32_t j1939_pgn_to_id(const struct j1939_pgn *pgn)
{
	return ((uint32_t)pgn->data_page << 16) |
	       ((uint32_t)pgn->pdu_format << 8) | (uint32_t)pgn->pdu_specific;
}

int j1939_send(const struct j1939_pgn *pgn, const uint8_t priority,
	       const uint8_t src, const uint8_t dst, uint8_t *data,
	       const uint32_t len);

int j1939_receive(struct j1939_pgn *pgn, uint8_t *priority, uint8_t *src,
		  uint8_t *dst, uint8_t *data, uint32_t *len);

int j1939_tp(struct j1939_pgn *pgn, const uint8_t priority, const uint8_t src,
	     const uint8_t dst, uint8_t *data, const uint16_t len);

int j1939_address_claimed(uint8_t src, struct j1939_name *name);

#endif /* __J1939_H__ */
