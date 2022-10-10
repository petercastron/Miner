/*
	100% free public domain implementation of the HMAC-MD5 algorithm
	by Chien-Chung, Chung (Jim Chung) <jimchung1221@gmail.com>
*/


#ifndef __HMAC_MD5_H__
#define __HMAC_MD5_H__

#include "commonHead.h"

void hmac_md5(const u_char *text, int text_len,const u_char *key, int key_len,u_char *digest);

#endif /* __HMAC_MD5_H__ */
