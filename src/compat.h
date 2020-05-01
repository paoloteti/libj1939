/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __COMPAT_H__
#define __COMPAT_H__

#if defined(__linux__)
#include <endian.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#if defined(_MSC_VER)
#include <stdlib.h>
#define htobe64(x) _byteswap_uint64(x)
#define htobe16(x) _byteswap_uint16(x)
#elif defined(__GNUC__) || defined(__clang__)
#define htobe64(x) __builtin_bswap64(x)
#define htobe16(x) __builtin_bswap16(x)
#endif
#endif
#else
#include <machine/endian.h>
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htobe64 __bswap64
#define htobe16 __bswap16
#else
#define htobe64
#define htobe16
#endif
#endif

#endif /* __COMPAT_H__ */
