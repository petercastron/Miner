#pragma once

#if !defined(IN)
#define IN
#endif

#if !defined(OUT)
#define OUT
#endif

#include <sys/types.h>

class CZLibWrapper
{
public:
	CZLibWrapper(void);
	~CZLibWrapper(void);

  static bool processMem  (IN const u_char* inBuffer, IN u_int inBufSize, OUT u_char* &outBuffer, OUT u_int &outBufSize, IN bool bEnc = false);
  static bool pisces_compress (u_char * in, u_long in_len, u_char ** out, u_long * out_len);
  static bool pisces_uncompress (u_char * in, u_long in_len, u_char **out, u_long * out_len);
};
