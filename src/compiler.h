/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __COMPILER_H__
#define __COMPILER_H__


#if defined(__GNUC__)

#define __arg_unused	__attribute__((unused))
#define __weak		__attribute__((weak))
#define __naked		__attribute__((naked))
#define __noreturn	__attribute__((noreturn))
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#else

#define __arg_unused
#define __weak
#define __naked
#define __noreturn
#define likely(x) (x)
#define unlikely(x) (x)

#endif /* __GNUC__ */


#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifdef MISRAC
#define IS_NULL(p) ((p) == NULL)
#else
#define IS_NULL(p) (!p)
#endif /* MISRAC */

#define NOT_NULL(p) !IS_NULL(p)

#endif /* __COMPILER_H__ */
