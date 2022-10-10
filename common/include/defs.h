#if !defined(__DEFS_H)
#define __DEFS_H

#include "common_defs.h"

#define IP_LABEL(family)         ((AF_INET == (family)) ? ("IPv4") : ("IPv6"))
#define SET_CRC(x, type_size) { if (x->len > 0) x->crc = htonl (crc32 ((u_char *)x + type_size, x->len)); else x->crc = 0; }
#define CHECK_CRC(x, type_size) (0 == x->len || x->crc == htonl (crc32 ((u_char *)x + type_size, x->len)))

#define PUBLIC_V(T, V) \
  T& get##V() { return V; }      \
  void set##V(const T& v) { V = v; }

#define LOCK_V(v)\
public:                 \
  void lock_##v() { pthread_mutex_lock(&_locker_##v); }      \
  void unlock_##v() { pthread_mutex_unlock(&_locker_##v); }    \
protected:              \
 void _init_locker_##v() { pthread_mutex_init(&_locker_##v, NULL); }  \
 void _destroy_locker_##v() { pthread_mutex_destroy(&_locker_##v); }  \
 pthread_mutex_t _locker_##v;

#endif

