#pragma once
#include <cstddef>

class CRC4 {
public:
	typedef struct rc4_key_t {
		unsigned char state[256];
		unsigned char x;
		unsigned char y;
	} rc4_key_t;
#define _swap(a, b) do { unsigned char __tmp = (a); (a) = (b); (b) = __tmp; } while (0)
	static void RC4 (unsigned char *buffer_ptr, int buffer_len, unsigned char *key_data, int key_len) {
		rc4_key_t key;
		_rc4_set_key (key_data, key_len, &key);
		_rc4_crypt (buffer_ptr, buffer_len, &key);
	}
private:
	static void _rc4_set_key(const unsigned char *buf, size_t len, rc4_key_t * key) {
		unsigned char j = 0;
		unsigned char *state = key->state;
		int i;
		for (i = 0;  i < 256; ++i) state[i] = i;
		key->x = 0;
		key->y = 0;
		for (i = 0; i < 256; ++i) {
			j = j + state[i] + buf[i % len];
			_swap(state[i], state[j]);
		}
	}
	static void _rc4_crypt(unsigned char *buf, size_t len, rc4_key_t * key) {
		unsigned char x, y;
		unsigned char *state = key->state;
		unsigned int  i;
		x = key->x;    y = key->y;
		for (i = 0; i < len; i++) {
			y = y + state[++x];
			_swap(state[x], state[y]);
			buf[i] ^= state[(state[x] + state[y]) & 0xff];
		}
		key->x = x;    key->y = y;
	}
};

#define RC4(buf,buflen,key,keylen) CRC4::RC4(buf,buflen,key,keylen)
