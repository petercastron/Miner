//******************************************************************************
//* HMAC_SHA1.cpp : Implementation of HMAC MD5 algorithm
//*                 Comfort to RFC 2104
//*
//******************************************************************************
#include "md5.h"
#include "hmac_md5.h"

void hmac_md5(const u_char *date, int date_length, const u_char *key, int key_len, u_char *digest)
{
  u_char k_ipad[65] = {0};    // inner padding -
  u_char k_opad[65] = {0};    // outer padding -

  /* start out by storing key in pads */
  memcpy( k_ipad, key, key_len);
  memcpy( k_opad, key, key_len);
  
  /* XOR key with ipad and opad values */
  for (int i = 0; i < 64; ++i) {
    k_ipad[i] ^= 0x36;
    k_opad[i] ^= 0x5c;
  }

  //perform inner MD5
  MD5 algo;
  algo.update(k_ipad, 64);
  algo.update(date, date_length);
  algo.finalize().hexdigest(digest);
  
  //perform outer MD5
  algo.clear();
  algo.update(k_opad, 64);
  algo.update(digest, 16);
  algo.finalize().hexdigest(digest);
}
