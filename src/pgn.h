
#ifndef __PGN_H__
#define __PGN_H__

#define PGN_FORMAT(_x) (((_x) >> 8) & 0xff)
#define PGN_SPECIFIC(_x) ((_x) & 0xff)
#define PGN_DATA_PAGE(_x) (((_x) >> 17) & 0x1)

#define PGN_MASK 0x3FFFFu

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
