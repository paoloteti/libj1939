/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __PGN_H__
#define __PGN_H__

#define PGN_FORMAT(_x) (((_x) >> 8) & 0xffu)
#define PGN_SPECIFIC(_x) ((_x) & 0xffu)
#define PGN_DATA_PAGE(_x) (((_x) >> 17) & 0x1u)

#define PRGN_PRIORITY_MASK 0x3u
#define PGN_MASK 0x3FFFFu
#define PGN_FROM(_f, _s, _dp)                                                  \
	(((_dp) & 0x1u) << 17) | (((_f) & 0xffu) << 8) | ((_s) & 0xffu)

#define BAM 	0x00FEECu
#define TP_CM 	0x00EC00u
#define TP_DT 	0x00EB00u
/** @brief Address Claimed */
#define AC 	0x00EE00u
/** @brief Request for Address Claimed */
#define RAC 	0x00EA00u

/** @brief Check if PDU format < 240 (peer-to-peer) */
static inline bool j1939_pdu_is_p2p(const j1939_pgn_t pgn)
{
	return PGN_FORMAT(pgn) < 240u;
}

/** @brief Check if PDU format >= 240 (broadcast) */
static inline bool j1939_pdu_is_broadcast(const j1939_pgn_t pgn)
{
	return !j1939_pdu_is_p2p(pgn);
}

#endif /* __PGN_H__ */
