#include "blockChainTools.h"
#ifdef SECP256K1_SUPPORT
#include "auxHelper.h"
#include "hmac_sha256.h"
#include "sha3.h"
#include "secp256k1.h"
#include "secp256k1_ecdh.h"
#include "secp256k1_recovery.h"
#include "rlpvalue/InfInt.h"
#endif

//////////////////////////////TOOLS////////////////////////////////
#ifdef SECP256K1_SUPPORT
//SECP256K1 KEY AND ADDRESS
std::string blockChainTools::bct_create_der_prikey()
{
  u_char random_buf[32] = {0};

  auxHelper::generate_random(random_buf, 32);

  return auxHelper::byte2hexstring(random_buf, 32);
}

std::string blockChainTools::bct_der_prikey_to_pubkey(const std::string &der_prikey)
{
  std::string der_pubkey = "";

  secp256k1_context *ctx = NULL;

  do
  {
    if (der_prikey.empty()) break;

    ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    if (NULL == ctx) break;
    
    secp256k1_pubkey pubkey;
    u_char pem_prikey[32] = {0};
    u_int pem_key_len = 32;
    auxHelper::hexstring2Byte(der_prikey, pem_prikey, pem_key_len);
    int ret = secp256k1_ec_pubkey_create(ctx, &pubkey, pem_prikey);
    if (!ret) break;

    u_char pubkey_data[65] = {0};
    size_t pubkey_len = 65;
    //Expect single byte header of value 0x04 -- uncompressed public key.
    ret = secp256k1_ec_pubkey_serialize(ctx, pubkey_data, &pubkey_len, &pubkey, SECP256K1_EC_UNCOMPRESSED);
    if (!ret) break;
    
    if (65 != pubkey_len || 0x04 != pubkey_data[0]) break;

    // Create the Public skipping the header.
    der_pubkey = auxHelper::byte2hexstring(pubkey_data + 1, pubkey_len - 1);

  } while (false);

  if (NULL != ctx) secp256k1_context_destroy(ctx);

  return der_pubkey;
}

std::string blockChainTools::bct_der_pubkey_to_address(const std::string &der_pubkey)
{
  std::string der_address = "";
  
  if (der_pubkey.empty()) return der_address;

  std::string pem_pubkey = auxHelper::der_to_pem(der_pubkey);

  u_char sha3[32] = {0}; 
  SHA3_256((u_int8_t *)sha3, (const u_int8_t *)pem_pubkey.c_str(), pem_pubkey.size());
  
  //Create the address skipping left 12 byte, using the right 20 bytes.
  der_address = auxHelper::byte2hexstring(sha3 + 12, 20);

  return der_address;
}

std::string blockChainTools::bct_der_prikey_to_address(const std::string &der_prikey)
{
  return bct_der_pubkey_to_address(bct_der_prikey_to_pubkey(der_prikey));
}

//SECP256K1 SIGNATURE AND SIGNATURE-VERIFY
bool blockChainTools::bct_signature(const std::string &payer_der_prikey, const u_char *hash, int &v, std::string &r, std::string &s)
{
  bool rtCode = false;
  static const InfInt c_secp256k1n("115792089237316195423570985008687907852837564279074904382605163141518161494337");
  secp256k1_context *ctx = NULL;

  do
  {
    if (payer_der_prikey.empty()) break;

    ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    if (NULL == ctx) break;

    secp256k1_ecdsa_recoverable_signature rawSig;
    int ret = secp256k1_ecdsa_sign_recoverable(ctx, &rawSig, hash, (const u_char *)auxHelper::der_to_pem(payer_der_prikey).c_str(), nullptr, nullptr);
    if (0 == ret) break;

    u_char sig_data[64] = {0};
    secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, sig_data, &v, &rawSig);
    
    r.assign((const char *)sig_data, 32);
    s.assign((const char *)sig_data + 32, 32);

    //EIP-2 : 0 < s < secp256k1n ÷ 2 + 1
    u_char cv = static_cast<u_char>(v);
    InfInt s_num = decode_bignum(s);
    if (s_num > (c_secp256k1n / 2)) {
      v = static_cast<u_char>(cv ^ 1);
      s_num = c_secp256k1n - s_num;
      s = encode_bignum(s_num);
    }

    rtCode = true;

  } while (false);

  if (NULL != ctx) secp256k1_context_destroy(ctx);
  
  return rtCode;
}

bool blockChainTools::is_bct_signature_valid(int v, const std::string &r, const std::string &s)
{
  static const InfInt s_max("115792089237316195423570985008687907852837564279074904382605163141518161494337");
  static const InfInt s_zero;
  
  InfInt r_bignum = decode_bignum(r);
  InfInt s_bignum = decode_bignum(s);

  return (v <= 1 && r_bignum > s_zero && s_bignum > s_zero && r_bignum < s_max && s_bignum < s_max);
}

bool blockChainTools::bct_signature_verify(const u_char *hash, const std::string &sin_data, const std::string &payer_der_addr)
{
  int v; std::string r, s;
  u_char sig_data[65] = {0};

  memcpy(sig_data, sin_data.c_str(), 65);

  r.assign((const char *)sig_data, 32);
  s.assign((const char *)sig_data + 32, 32);
  v = sig_data[64];

  return bct_signature_verify(hash, v, r, s, payer_der_addr);
}

bool blockChainTools::bct_signature_verify(const u_char *hash, int v, const std::string &r, const std::string &s, const std::string &payer_der_addr)
{
  bool rtCode = false;
  secp256k1_context *ctx = NULL;

  do
  {
    if (v > 3) break;

    ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    if (NULL == ctx) break;

    secp256k1_ecdsa_recoverable_signature rawSig;
    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &rawSig, (const u_char *)(r + s).c_str(), v)) break;

    secp256k1_pubkey rawPubkey;
    if (!secp256k1_ecdsa_recover(ctx, &rawPubkey, &rawSig, hash)) break;

    u_char pubkey_data[65] = {0};
    size_t pubkey_len = 65;
    secp256k1_ec_pubkey_serialize(ctx, pubkey_data, &pubkey_len, &rawPubkey, SECP256K1_EC_UNCOMPRESSED);
    if (65 != pubkey_len || 0x04 != pubkey_data[0]) break;

    // Create the Public skipping the header.
    std::string der_pubkey = auxHelper::byte2hexstring(pubkey_data + 1, pubkey_len - 1);
    rtCode = (payer_der_addr == blockChainTools::bct_der_pubkey_to_address(der_pubkey));

  } while (false);

  if (NULL != ctx) secp256k1_context_destroy(ctx);
  
  return rtCode;
}

#endif