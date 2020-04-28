
#ifndef __PGN_POOL_H__
#define __PGN_POOL_H__

#include "j1939.h"

#define ERR_NONE 		0
#define ERR_TOO_MANY_PGN	1
#define ERR_PGN_UNKNOWN		2
#define ERR_DUPLICATE_PGN	3



int pgn_register(const uint32_t pgn, uint8_t code, pgn_callback_t cb);
int pgn_deregister(const uint32_t pgn, uint8_t code);
void pgn_deregister_all(void);
int pgn_pool_receive(void);

#endif /* __PGN_POOL_H__ */
