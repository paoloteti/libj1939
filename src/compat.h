
#ifndef __COMPAT_H__
#define __COMPAT_H__

#if defined(__linux__)
#include <endian.h>
#else
#include <machine/endian.h>
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htobe64 __bswap64
#else
#define htobe64
#endif /* __BYTE_ORDER__ */
#endif

#endif /* __COMPAT_H__ */

