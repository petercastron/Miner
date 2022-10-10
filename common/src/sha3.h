#pragma once

// Visual Studio doesn't support the inline keyword in C mode
#if defined(_MSC_VER) && !defined(__cplusplus)
#define inline __inline
#endif

// pretend restrict is a standard keyword
#if defined(_MSC_VER)
#define restrict __restrict
#else
#define restrict __restrict__
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#define decsha3(bits) \
	int sha3_##bits(uint8_t*, size_t, uint8_t const*, size_t);

decsha3(256)
decsha3(512)

static inline void SHA3_256(uint8_t* ret, uint8_t const* data, size_t const size)
{
	sha3_256(ret, 32, data, size);
}

static inline void SHA3_512(uint8_t* ret, uint8_t const* data, size_t const size)
{
	sha3_512(ret, 64, data, size);
}

#ifdef __cplusplus
}
#endif
