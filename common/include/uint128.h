#ifndef _UINT_128_H_
#define _UINT_128_H_

#include "commonHead.h"

#pragma pack(push)
#pragma  pack (1)
struct uint128_64
{
#if defined(CIFER_BIG_ENDIAN)
  u_int64_t hi;
  u_int64_t lo;
#else
  u_int64_t lo;
  u_int64_t hi;
#endif
};

typedef union {
  struct uint128_64 u64;
  u_int32_t u32[4];
  u_int16_t u16[8];
  u_int8_t  u8[16];
} uint128_t;
#define hi64 u64.hi
#define lo64 u64.lo

#if defined(CIFER_BIG_ENDIAN) 
#define UINT128_U64_INIT(x,y) {(x), (y)}
#else
#define UINT128_U64_INIT(x,y) {(y), (x)}
#endif
#pragma pack(pop)

#ifndef htonll
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#endif

#ifndef ntohll
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

static const uint128_t ip_none = UINT128_U64_INIT (0xffffffffffffffff, 0xffffffffffffffff);
static const uint128_t ip_any  = UINT128_U64_INIT (0, 0);

inline uint128_t uint128_inc (const uint128_t& val)
{
  uint128_t ret = UINT128_U64_INIT (0, 0);
  u_int64_t t = (val.lo64 + 1);

  ret.hi64 = val.hi64 + (((val.lo64 ^ t) & val.lo64) >> 63);
  ret.lo64 = t;

  return ret;
}

inline uint128_t uint128_dec (const uint128_t& val)
{
  uint128_t ret = UINT128_U64_INIT (0, 0);
  u_int64_t t = (val.lo64 - 1);

  ret.hi64 = val.hi64 - (((t ^ val.lo64) & t) >> 63);
  ret.lo64 = t;

  return ret;
}

inline uint128_t uint128_add (const uint128_t& val1, const uint128_t& val2)
{
  uint128_t ret = UINT128_U64_INIT (0, 0);
  u_int64_t c = (((val1.lo64 & val2.lo64) & 1) + (val1.lo64 >> 1) + (val2.lo64 >> 1)) >> 63;

  ret.hi64 = val1.hi64 + val2.hi64 + c;
  ret.lo64 = val1.lo64 + val2.lo64;

  return ret;
}

inline uint128_t uint128_sub (const uint128_t& val1, const uint128_t& val2)
{
  uint128_t ret = UINT128_U64_INIT (0, 0);
  u_int64_t c = 0;
  
  ret.lo64 = val1.lo64 - val2.lo64;

  c = (((ret.lo64 & val2.lo64) & 1) + (val2.lo64 >> 1) + (ret.lo64 >> 1)) >> 63;

  ret.hi64 = val1.hi64 - (val2.hi64 + c);

  return ret;
}

/**
  val1 = val2 : return 0
  val1 > val2 : return 1
  val1 < val2 : return -1
 */
inline int uint128_compare (const uint128_t& val1, const uint128_t& val2)
{
  return (((val1.hi64 > val2.hi64) || ((val1.hi64 == val2.hi64) && (val1.lo64 > val2.lo64))) ? 1 : 0) -  (((val1.hi64 < val2.hi64) || ((val1.hi64 == val2.hi64) && (val1.lo64 < val2.lo64))) ? 1 : 0);
}


inline uint128_t uint128_shift_left (const uint128_t& val, int count)
{
  uint128_t ret = UINT128_U64_INIT (0, 0);
  
  ret.hi64 = (val.hi64 << count) | (val.lo64 >> (-count & 63));
  ret.lo64 = val.lo64 << count;

  return ret;
}

inline uint128_t uint128_shift_right (const uint128_t& val, int count)
{
  uint128_t ret = UINT128_U64_INIT (0, 0);
  
  ret.hi64 = val.hi64 >> count;
  ret.lo64 = (val.hi64 << (-count & 63)) | (val.lo64 >> count);

  return ret;
}

inline uint128_t uint128_neg (const uint128_t& val)
{
  uint128_t ret = UINT128_U64_INIT (0, 0);
  
  if (val.lo64) {
    ret.hi64 = ~val.hi64;
    ret.lo64 = ~val.lo64;
  } else {
    ret.hi64 = ~val.hi64;
  }
  return ret;

}

inline uint128_t uint128_and (const uint128_t& val1, const uint128_t& val2)
{
  uint128_t ret = UINT128_U64_INIT (0, 0);

  ret.hi64 = val1.hi64 & val2.hi64;
  ret.lo64 = val1.lo64 & val2.lo64;
  
  return ret;
}

inline uint128_t uint128_htobe (const uint128_t& val)
{
  uint128_t ret = UINT128_U64_INIT (0, 0);

  ret.hi64 = htonll (val.lo64);
  ret.lo64 = htonll (val.hi64);

  return ret;
}

inline uint128_t uint128_betoh (const uint128_t& val)
{
  uint128_t ret = UINT128_U64_INIT (0, 0);
  
  ret.hi64 = ntohll (val.lo64);
  ret.lo64 = ntohll (val.hi64);

  return ret;
}

inline bool in6_is_addr_linklocal(const uint128_t * a)
{
	return ((a->u8[0] == 0xfe) && ((a->u8[1] & 0xc0) == 0x80));
}

//////////////////////////////////////////////////////////////////////////
inline bool is_valid_ipv6_netmask (const uint128_t * a)
{
	u_int64_t y = ~ ntohll (a->u64.lo);
	u_int64_t z = y + 1;

	return 0 == (z & y);
}

inline bool in6_is_addr_unspecified(const uint128_t * a)
{
	//
	// We can't use the in6addr_any variable, since that would
	// require existing callers to link with a specific library.
	//
	return (bool)((a->u16[0] == 0) &&
		(a->u16[1] == 0) &&
		(a->u16[2] == 0) &&
		(a->u16[3] == 0) &&
		(a->u16[4] == 0) &&
		(a->u16[5] == 0) &&
		(a->u16[6] == 0) &&
		(a->u16[7] == 0));
}


inline bool in6_is_addr_loopback(const uint128_t * a)
{
	//
	// We can't use the in6addr_loopback variable, since that would
	// require existing callers to link with a specific library.
	//
	return (bool)((a->u16[0] == 0) &&
		(a->u16[1] == 0) &&
		(a->u16[2] == 0) &&
		(a->u16[3] == 0) &&
		(a->u16[4] == 0) &&
		(a->u16[5] == 0) &&
		(a->u16[6] == 0) &&
		(a->u16[7] == 0x0100));
}

inline bool in6_is_addr_multicast(const uint128_t * a)
{
	return (bool)(a->u8[0] == 0xff);
}

//
//  Does the address have a format prefix
//  that indicates it uses EUI-64 interface identifiers?
//
inline bool in6_is_addr_eui64(const uint128_t * a)
{
	//
	// Format prefixes 001 through 111, except for multicast.
	//
	return (bool)(((a->u8[0] & 0xe0) != 0) && !in6_is_addr_multicast(a));
}

//
//  Is this the subnet router anycast address?
//  See RFC 2373.
//
inline bool in6_is_addr_subnet_router_anycast(const uint128_t * a)
{
	return (bool)(in6_is_addr_eui64(a) &&
		(a->u16[4] == 0) &&
		(a->u16[5] == 0) &&
		(a->u16[6] == 0) &&
		(a->u16[7] == 0));
}

//
//  Is this a subnet reserved anycast address?
//  See RFC 2526. It talks about non-EUI-64
//  addresses as well, but IMHO that part
//  of the RFC doesn't make sense. For example,
//  it shouldn't apply to multicast or v4-compatible
//  addresses.
//
inline bool in6_is_addr_subnet_reserved_anycast(const uint128_t * a)
{
	return (bool)(in6_is_addr_eui64(a) &&
		(a->u16[4] == 0xfffd) &&
		(a->u16[5] == 0xffff) &&
		(a->u16[6] == 0xffff) &&
		((a->u16[7] & 0x80ff) == 0x80ff));
}


//
//  As best we can tell from simple inspection,
//  is this an anycast address?
//
inline bool in6_is_addr_anycast(const uint128_t * a)
{
	return (in6_is_addr_subnet_reserved_anycast(a) || in6_is_addr_subnet_router_anycast(a));
}

inline bool in6_is_addr_sitelocal(const uint128_t * a)
{
	return ((a->u8[0] == 0xfe) && ((a->u8[1] & 0xc0) == 0xc0));
}

inline bool in6_is_addr_global(const uint128_t * a)
{
	//
	// Check the format prefix and exclude addresses
	// whose high 4 bits are all zero or all one.
	// This is a cheap way of excluding v4-compatible,
	// v4-mapped, loopback, multicast, link-local, site-local.
	//
	u_long High = (a->u8[0] & 0xf0);
	return ((High != 0) && (High != 0xf0));
}

inline bool in6_is_addr_v4mapped(const uint128_t * a)
{
	return (bool)((a->u16[0] == 0) &&
		(a->u16[1] == 0) &&
		(a->u16[2] == 0) &&
		(a->u16[3] == 0) &&
		(a->u16[4] == 0) &&
		(a->u16[5] == 0xffff));
}

inline bool in6_is_addr_v4compat(const uint128_t * a)
{
	return (bool)((a->u16[0] == 0) &&
		(a->u16[1] == 0) &&
		(a->u16[2] == 0) &&
		(a->u16[3] == 0) &&
		(a->u16[4] == 0) &&
		(a->u16[5] == 0) &&
		!((a->u16[6] == 0) &&
		(a->u8[14] == 0) &&
		((a->u8[15] == 0) || (a->u8[15] == 1))));
}

inline bool in6_is_addr_v4translated(const uint128_t *a)
{
	return (bool)((a->u16[0] == 0) &&
		(a->u16[1] == 0) &&
		(a->u16[2] == 0) &&
		(a->u16[3] == 0) &&
		(a->u16[4] == 0xffff) &&
		(a->u16[5] == 0));
}

inline bool in6_is_addr_mc_nodelocal(const uint128_t * a)
{
	return (bool)(in6_is_addr_multicast(a) && ((a->u8[1] & 0xf) == 1));
}

inline bool in6_is_addr_mc_linklocal(const uint128_t * a)
{
	return (bool)(in6_is_addr_multicast(a) && ((a->u8[1] & 0xf) == 2));
}

inline bool in6_is_addr_mc_sitelocal(const uint128_t * a)
{
	return (bool)(in6_is_addr_multicast(a) && ((a->u8[1] & 0xf) == 5));
}

inline bool in6_is_addr_mc_orglocal(const uint128_t * a)
{
	return (bool)(in6_is_addr_multicast(a) && ((a->u8[1] & 0xf) == 8));
}

inline bool in6_is_addr_mc_global(const uint128_t * a)
{
	return (bool)(in6_is_addr_multicast(a) && ((a->u8[1] & 0xf) == 0xe));
}

inline void in6_set_addr_unspecified(uint128_t * a)
{
	//
	// We can't use the in6addr_any variable, since that would
	// require existing callers to link with a specific library.
	//
	a->u64.hi = 0; 
	a->u64.lo = 0;
}

inline void in6_set_addr_loopback(uint128_t * a)
{
	//
	// We can't use the in6addr_loopback variable, since that would
	// require existing callers to link with a specific library.
	//
	a->u64.hi = 0; 
	a->u64.lo = 0;
	a->u8[15] = 1;
}

#endif //_UINT_128_H_
