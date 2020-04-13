
#ifndef __BITS_H__
#define __BITS_H__

static inline uint64_t swap64(uint64_t v)
{
#if defined(__llvm__) || defined(__GNUC__)
	return __builtin_bswap64(v);
#else
	return (uint64_t)swap32(v << 32) | (uint64_t)swap32(v >> 32);
#endif
}

static inline uint32_t swap32(uint32_t v)
{
#if defined(__llvm__) || defined(__GNUC__)
	return __builtin_bswap32(v);
#else
	uint32_t b0 = v & 0x000000FF;
	uint32_t b1 = v & 0x0000FF00;
	uint32_t b2 = v & 0x00FF0000;
	uint32_t b3 = v & 0xFF000000;
	return (b0 << 24) | (b1 << 8) | (b2 >> 8) | (b3 >> 24);
#endif
}

static inline uint16_t swap16(uint16_t v)
{
#if defined(__llvm__) || defined(__GNUC__)
	return __builtin_bswap16(v);
#else
	uint16_t b0 = v & 0x00FF;
	uint16_t b1 = v & 0xFF00;
	return (b0 << 8) | (b1 >> 8);
#endif
}
#endif /* __BITS_H__ */
