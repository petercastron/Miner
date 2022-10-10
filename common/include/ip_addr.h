#ifndef _IP_ADDR_H_
#define _IP_ADDR_H_

#include "uint128.h"
#include <vector>
#include <string>

#pragma pack(push)
#pragma  pack (1)
typedef union _ip_addr_any
{
  uint128_t u128;
  struct in_addr in_addr_ipv4;
  struct in6_addr in6_addr_ipv6;
} in_addr_any;

typedef union _sockaddr_in_any
{
  struct sockaddr_in sockaddr_in_ipv4;
  struct sockaddr_in6 sockaddr_in6_ipv6;
} sockaddr_in_any;

class IPADDR
{
public:
  IPADDR() {
    clean();
  }

  IPADDR(u_int ip) {
    clean();
    set_ipv4(ip);
  }

  IPADDR(const std::string &ip) {
    clean();
    set_ip(ip);
  }

  IPADDR(u_short fam, const void *ip) {
    clean();
    set_ip(fam, ip);
  }

  IPADDR(const IPADDR &ip) {
    family = ip.family;
    ip_addr.u128.hi64 = ip.ip_addr.u128.hi64;
    ip_addr.u128.lo64 = ip.ip_addr.u128.lo64;
  }

  inline void set_ip(u_short fam, const void *ip) {
    clean();
    
    if (NULL != ip) {
      if(AF_INET == fam) {
        family = AF_INET;
        ip_addr.in_addr_ipv4.s_addr = *((u_int *)ip);
      }else if (AF_INET6 == fam) {
        family = AF_INET6;
        in_addr_any * ip_any = (in_addr_any *)ip;
        ip_addr.u128.hi64 = ip_any->u128.hi64;
        ip_addr.u128.lo64 = ip_any->u128.lo64;
      }else {
        family = fam;
        set_ip_addr_none();
      }
    }else {
      family = fam;
      set_ip_addr_none();
    }
  }

  inline void set_ipv4(u_int ip) {
    set_ip(AF_INET, (u_char *) &ip);
  }

  inline void set_ipv6(const void *ip) {
    set_ip(AF_INET6, ip);
  }

  inline void set_ipv6(const in6_addr & sockaddr_in6_ipv6) {
    family = AF_INET6;
    in_addr_any * ip_any = (in_addr_any *)&sockaddr_in6_ipv6;
    ip_addr.u128.hi64 = ip_any->u128.hi64;
    ip_addr.u128.lo64 = ip_any->u128.lo64;
  }

  inline void set_ipv6(const uint128_t & ip) {
    family = AF_INET6;
    ip_addr.u128.hi64 = ip.hi64;
    ip_addr.u128.lo64 = ip.lo64;
  }

  bool set_ip(const char *ip) {
    bool rtCode = false;
    u_short af = AF_UNSPEC;
    
    do {
      
      if (NULL == ip) break;
      
      size_t length = strlen(ip);
      if (0 == length) break;
      
      for (size_t i = 0; i < length; ++i) {
        if ('.' == ip[i]) {
          af = AF_INET;
          break;
        } else if (':' == ip[i]) {
          af = AF_INET6;
          break;
        }
      }
      
      if (AF_UNSPEC == af) break;
      
      clean();
      
      family = af;
      if (inet_pton(af, ip, &ip_addr) <= 0) {
        family = AF_UNSPEC;
        break;
      }

      rtCode = true;
      
    } while (false);
    
    return rtCode;
  }

  bool set_ip(const std::string &ip) {
    if (ip.empty()) return false;
    return set_ip(ip.c_str());
  }

  std::string toString() const {
    char str[INET6_ADDRSTRLEN];
    bzero(str, sizeof(str));

    if (AF_INET6 == family || AF_INET == family)
      inet_ntop(family, &ip_addr, str, INET6_ADDRSTRLEN);
    else sprintf(str, "IPADDR_NONE");

    return (str);
  }
  
  inline u_int get_ipv4() const {
    return ip_addr.in_addr_ipv4.s_addr;
  }

  inline const void *get_ipv6() const {
    return (void *) (&(ip_addr.in6_addr_ipv6));
  }

  IPADDR &operator=(const IPADDR &ip) {
    if (this != &ip) {
      family = ip.family;
      ip_addr.u128.hi64 = ip.ip_addr.u128.hi64;
      ip_addr.u128.lo64 = ip.ip_addr.u128.lo64;
    }
    return *this;
  }

  /**
  val1 < val2   : return true
  val1 >= val2  : return false
  */

  //must strict weakly ordering for std::map's key
  bool operator<(const IPADDR &ip) const {
    bool rtCode = false;

    do{

      if(*this == ip){
        rtCode = false;
        break;
      }

      //cmp family
      if(family < ip.family) {
        rtCode = true;
        break;
      } else if ( family > ip.family){
        rtCode = false;
        break;
      } else{
        if (-1 == uint128_compare(ip_addr.u128, ip.ip_addr.u128)) rtCode = true;
        else rtCode = false;
      }
    }while (false);

    return rtCode;
  }


//  IPADDR& operator=(const u_int& ip) {
//    if(0 != ip){
//      family = AF_INET;
//      bzero(&ip_addr, sizeof(ip_addr));
//      ip_addr.in_addr_ipv4.s_addr = ip;
//    }else{
//      family = 0;
//      bzero(&ip_addr, sizeof(ip_addr));
//    }
//    return *this;
//  }
//  
//  IPADDR& operator=(const sockaddr_in6& ip) {
//    family = AF_INET6;
//    bzero(&ip_addr, sizeof(ip_addr));
//    memcpy(&ip_addr, &ip, sizeof(ip_addr.in6_addr_ipv6));
//    return *this;
//  }

  friend bool operator==(IPADDR const &a, IPADDR const &b) {
    bool rtCode = false;

    if ((&a) != (&b)) {
      if (a.family == b.family) {
        rtCode = (a.ip_addr.u128.u32[0] == b.ip_addr.u128.u32[0] && a.ip_addr.u128.u32[1] == b.ip_addr.u128.u32[1]
               && a.ip_addr.u128.u32[2] == b.ip_addr.u128.u32[2] && a.ip_addr.u128.u32[3] == b.ip_addr.u128.u32[3]);
      }
    } else rtCode = true;

    return rtCode;
  }

  friend bool operator!=(IPADDR const &a, IPADDR const &b) {
    return !(a == b);
  }

  friend std::size_t hash_value(const IPADDR &a) {
    std::size_t nHash = 0;
    char *key = (char *) &a.ip_addr;
    u_int length = sizeof(in_addr_any);
    for (unsigned int i = 0; i < length; ++i)
      nHash = (nHash << 5) + nHash + *key++;
    return nHash;
  }

  inline void set_ip_addr_unspecified() {
    ip_addr.u128.u32[0] = INADDR_ANY;
    ip_addr.u128.u32[1] = INADDR_ANY;
    ip_addr.u128.u32[2] = INADDR_ANY;
    ip_addr.u128.u32[3] = INADDR_ANY;
  }

  inline void set_ip_addr_none() {
    ip_addr.u128.u32[0] = INADDR_NONE;
    ip_addr.u128.u32[1] = INADDR_NONE;
    ip_addr.u128.u32[2] = INADDR_NONE;
    ip_addr.u128.u32[3] = INADDR_NONE;
  }

  inline void clean() {
    family = AF_UNSPEC;
    set_ip_addr_none();
  }

  inline bool is_ipaddr_any() const {
    bool rtCode = false;
  
    if(AF_INET == family) {
        if(INADDR_ANY == ip_addr.in_addr_ipv4.s_addr)
          rtCode = true;
    }else if (AF_INET6 == family) {
        if(INADDR_ANY == ip_addr.u128.u32[0] &&
           INADDR_ANY == ip_addr.u128.u32[1] &&
           INADDR_ANY == ip_addr.u128.u32[2] &&
           INADDR_ANY == ip_addr.u128.u32[3])
          rtCode = true;
    }
    return rtCode;
  }

  inline bool isempty() const {
    if (AF_INET != family && AF_INET6 != family) return true;
    else return false;
  }

#if 0
  inline bool is_ipaddr_none() const {
    if(AF_INET != family && AF_INET6 != family &&
       INADDR_NONE == ip_addr.u128.u32[0] &&
       INADDR_NONE == ip_addr.u128.u32[1] &&
       INADDR_NONE == ip_addr.u128.u32[2] &&
       INADDR_NONE == ip_addr.u128.u32[3])
      return true;
    else return false;
  }
#endif
  
  inline bool is_host_mask() const {
    bool rtCode = false;
    
    if(AF_INET == family) {
      if(INADDR_NONE == ip_addr.in_addr_ipv4.s_addr)
        rtCode = true;
    }else if (AF_INET6 == family) {
      if(INADDR_NONE == ip_addr.u128.u32[0] &&
         INADDR_NONE == ip_addr.u128.u32[1] &&
         INADDR_NONE == ip_addr.u128.u32[2] &&
         INADDR_NONE == ip_addr.u128.u32[3])
        rtCode = true;
    }
    return rtCode;
  }
public:
  u_short family;  // AF_INET or AF_INET6
  in_addr_any ip_addr;
};
#pragma pack(pop)

typedef IPADDR * PIPADDR;
typedef const IPADDR * PCIPADDR;
typedef std::vector<IPADDR> IPADDR_VECTOR;
#define SDVN_IPADDR_NONE IPADDR(AF_UNSPEC, NULL)
#define SDVN_IPADDR_ANY(x) IPADDR(x, &ip_any)
#define SDVN_IPADDR_HOST_MASK(x) IPADDR(x, NULL)
#endif /*_IP_ADDR_H_*/
