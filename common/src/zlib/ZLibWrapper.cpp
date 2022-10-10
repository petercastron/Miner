#include "ZLibWrapper.h"
#include "zlib.h"
#include "commonHead.h"

bool CZLibWrapper::processMem(IN const u_char* inBuffer, IN u_int inBufSize, OUT u_char* &outBuffer, OUT u_int &outBufSize, IN bool bEnc /* = false */)
{
	char* buffer;
	uLongf length;
	int rtCode;

	if (true == bEnc) {
		length = compressBound (inBufSize);
		buffer = new char[length];
		rtCode = compress ((Bytef *)buffer, (uLongf *)&length, inBuffer, inBufSize);
	} else {
		length = outBufSize;
		buffer = new char[length];
		rtCode = uncompress ((Bytef *)buffer, (uLongf *)&length, inBuffer, inBufSize);
	}
	if (Z_OK != rtCode) delete[] buffer;
	else {
		outBuffer = (u_char*) buffer;
		outBufSize = (u_int)length;
	}
	return (Z_OK == rtCode);
}

bool CZLibWrapper::pisces_compress (u_char * in, u_long in_len, u_char ** out, u_long * out_len)
{
  bool bret = false;
  
  if (NULL == in || 0 == in_len || NULL == out || NULL == out_len) goto out;
  
  *out_len = (u_long)(in_len * 1.001) + 12;
  *out =  new u_char[*out_len + 4];
  if (NULL == *out) goto out;
  
  *((u_long*)(*out)) = htonl(in_len);
  
  if (0 != compress2 ((Bytef*)((*out) + 4), (uLongf*)out_len, (Bytef*)in, in_len, 9)) goto out;
  
  *out_len += 4;
  bret = true;
  
out:
  if (true != bret) {
    if (NULL != out && NULL != *out) {
      delete[] (*out);
      *out = NULL;
    }
  }
  return (bret);
}

bool CZLibWrapper::pisces_uncompress (u_char * in, u_long in_len, u_char **out, u_long * out_len)
{
  bool bret = false;
  int n = 0;
  
  if (NULL == in || 0 == in_len || NULL == out_len || NULL == out) goto out;
  
  *out_len = (u_long)(ntohl(*((u_long*)in)) * 1.001) + 12;
  *out = new u_char[*out_len];
  if (NULL == *out) goto out;
  
  if (0 != (n = uncompress ((Bytef*) *out, out_len, (const Bytef*) (in+4), in_len-4))) goto out;
  
  bret = true;
out:
  if (true != bret) {
    if (NULL != out && NULL != *out) {
      delete[] (*out);
      *out = NULL;
    }
  }
  return (bret);
}
