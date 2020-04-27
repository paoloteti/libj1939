/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __J1939_H__
#define __J1939_H__

#define EARGS 		1
#define ETIMEOUT	2
#define EBUSY 		3
#define EINCOMPLETE	4

#define J1939_MAX_DATA_LEN 1785 /*<! Maximum data stream length */

#define ADDRESS_GLOBAL 0xFFu
#define ADDRESS_NOT_CLAIMED 0xFEu
#define ADDRESS_NULL 0xEFu

#define J1939_PRIORITY_HIGH 0x0u
#define J1939_PRIORITY_DEFAULT 0x6u
#define J1939_PRIORITY_LOW 0x7u

#define J1939_EBUSY 		100
#define J1939_EWRONG_DATA_LEN	101
#define J1939_ENO_RESOURCE	102
#define J1939_EIO		103
#define J1939_EINCOMPLETE	104

/** @brief indicates that the parameter is "not available" */
#define J1930_NOT_AVAILABLE_8 0xFFu
#define J1930_NA_8 J1930_NOT_AVAILABLE_8
#define J1930_NOT_AVAILABLE_16 0xFF00u
#define J1930_NA_16 J1930_NOT_AVAILABLE_16
#define J1930_NA_16_MSB (J1930_NOT_AVAILABLE_16 >> 8)
#define J1930_NA_16_LSB (J1930_NOT_AVAILABLE_16 & 0xFF)

/** @brief indicates that the parameter is "not valid" or "in error" */
#define J1930_NOT_VALID_8 0xFEu
#define J1930_NV_8 J1930_NOT_VALID_8
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

typedef union {
	struct {
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
		* Instance number of a function to distinguish two or more
		* devices with the same function number.
		* The first instance is assigned to the instance number 0.
		*/
		uint64_t function_instance : 5;
		/*
		* 8-bit Function
		* One of the predefined J1939 functions. The same function value
		* (upper 128 only) may mean different things for different
		* Industry Groups or Vehicle Systems.
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
		* segments and may be connected from the vehicle.
		* A Vehicle System may be made of one or more functions.
		* The Vehicle System depends on the Industry Group definition.
		*/
		uint64_t vehicle_system : 7;
		/*
		* 4-bit Vehicle System Instance
		* Instance number of a vehicle system to distinguish two or
		* more device with the same Vehicle System number in the same
		* J1939 network.
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
		* Set to 1 if the device is Arbitrary Address Capable,
		* set to 0 if it's Single Address Capable.
		*/
		uint64_t arbitrary_address_capable : 1;
	} fields;
	uint64_t value;
} ecu_name_t;

/** @brief J1939 PGN according to SAE J1939/21 */
typedef uint32_t j1939_pgn_t;

struct j1939_pgn_filter {
	j1939_pgn_t pgn;
	j1939_pgn_t pgn_mask;
	uint8_t addr;
	uint8_t addr_mask;
};

#define SEND_PERIOD 50 /* <! Send period [msec] */

/** @brief Timeouts ([msec]) according to SAE J1939/21 */
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
		(((_dp)     & 0x01u) << 17) |                                  \
		(((_format) & 0xffu) << 8)  |                                  \
		((_specific)& 0xffu)                                           \
	}

extern int j1939_cansend(uint32_t id, uint8_t *data, uint8_t len);
extern int j1939_canrcv(uint32_t *id, uint8_t *data);
extern int j1939_filter(struct j1939_pgn_filter *filter, uint32_t num_filters);
extern uint32_t j1939_get_time(void);


bool static inline j1939_valid_priority(const uint8_t p)
{
	return p <= J1939_PRIORITY_LOW;
}

int j1939_send(const j1939_pgn_t pgn, const uint8_t priority, const uint8_t src,
	       const uint8_t dst, uint8_t *data, const uint32_t len);

int j1939_receive(j1939_pgn_t *pgn, uint8_t *priority, uint8_t *src,
		  uint8_t *dst, uint8_t *data, uint32_t *len);

/**
 * @brief J1939 Transport Protocol (TP)
 *
 * J1939 transport protocol breaks up PGs larger than 8 data bytes and up to
 * 1785 bytes, into multiple packets. The transport protocol defines the rules
 * for packaging, transmitting, and reassembling the data.
 *
 * Messages that have multiple packets are transmitted with a dedicated PGN, and
 * have the same message ID and similar functionality.
 * The length of each message in the packet must be 8 bytes or fewer.
 * The first byte in the data field of a message specifies the sequence of the
 * message (one to 255) and the next seven bytes contain the original data.
 * All unused bytes in the data field are set to zero.
 *
 * @param pgn PGN to be sent
 * @param priority PGN priority
 * @param src source address
 * @param dest destination address
 * @param data array of bytes to be sent
 * @param len data length (in bytes)
 * @return -1 in case of error, 0 otherwise
 */
int j1939_tp(j1939_pgn_t pgn, const uint8_t priority, const uint8_t src,
	     const uint8_t dst, uint8_t *data, const uint16_t len);

int j1939_address_claimed(uint8_t src, ecu_name_t name);

int j1939_address_claim(const uint8_t src, ecu_name_t name);

int j1939_cannot_claim_address(ecu_name_t name);

int j1939_send_tp_cts(const uint8_t src, const uint8_t dst,
		      const uint8_t num_packets, const uint8_t next_packet);

int send_tp_bam(const uint8_t priority, const uint8_t src, uint8_t *data,
		const uint16_t len);

typedef int (*pgn_callback_t)(j1939_pgn_t pgn, uint8_t priority,
			      uint8_t src, uint8_t dest,
			      uint8_t *data, uint8_t len);


typedef void (*pgn_error_cb_t)(j1939_pgn_t pgn, uint8_t priority,
			      uint8_t src, uint8_t dest, int err);

int j1939_setup(pgn_callback_t rcv_tp, pgn_error_cb_t err_cb);
int j1939_dispose(void);

#endif /* __J1939_H__ */
