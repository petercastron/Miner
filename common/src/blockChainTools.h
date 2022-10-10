#ifndef BLOCK_CHAIN_TOOLS_H
#define BLOCK_CHAIN_TOOLS_H

#include "commonHead.h"
#include <string>

class blockChainTools
{
public:
//////////////////////////////TOOLS////////////////////////////////
#ifdef SECP256K1_SUPPORT
  //secp256k1 key and address
  static std::string bct_create_der_prikey();
  static std::string bct_der_prikey_to_pubkey(const std::string &der_prikey);
  static std::string bct_der_pubkey_to_address(const std::string &der_pubkey);
  static std::string bct_der_prikey_to_address(const std::string &der_prikey);

  //secp256k1 signature and signature-verify
  static bool bct_signature(const std::string &payer_der_prikey, const u_char *hash, int &v, std::string &r, std::string &s);
  static bool is_bct_signature_valid(int v, const std::string &r, const std::string &s);
  static bool bct_signature_verify(const u_char *hash, const std::string &sin_data, const std::string &payer_der_addr);
  static bool bct_signature_verify(const u_char *hash, int v, const std::string &r, const std::string &s, const std::string &payer_der_addr);
#endif
};

#endif //BLOCK_CHAIN_TOOLS_H
