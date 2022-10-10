#if !defined(__HIREFS_LOCAL_DEFS_H)
#define __HIREFS_LOCAL_DEFS_H

#include <string>

#define HIREFS_VOLUME_DIRECTORY   "/data/utgfs"
#define HIREFS_FILE_NAME HIREFS_VOLUME_DIRECTORY "/.utgfs.cfg"
#define HIREFS_ADMIN_PASSWD_FILE_NAME HIREFS_VOLUME_DIRECTORY "/.adminpasswd"
#define HIREFS_PRIVATE_KEY_FILE_NAME HIREFS_VOLUME_DIRECTORY "/.privatekey"

//NEED DELETE WHEN RESET
#define HIREFS_VOLUME_STATUS_FILE_NAME HIREFS_VOLUME_DIRECTORY "/.status"
#define TOPMOST_MERKLETREE_FILE_NAME "/.topmost_merkletree"
#define HIREFS_VOLUME_PACKAGE_TOPMOST_MERKLETREE_FILE_NAME   HIREFS_VOLUME_DIRECTORY TOPMOST_MERKLETREE_FILE_NAME
#define HIREFS_VOLUME_PACKAGE_POC_COMMIT_HISTORY HIREFS_VOLUME_DIRECTORY "/.volume_package_poc_commit_history"
#define HIREFS_LOGS HIREFS_VOLUME_DIRECTORY "/.utg_logs"

typedef enum _HIRE_COMMAND
{
  HIRECMD_NONE = 0,
  HIRECMD_GET_HIRE_INFO = 1,
  HIRECMD_SET_HIRE_INFO = 2,
  HIRECMD_VOLUME_PACKAGE = 3,
  HIRECMD_GET_VOLUME_PACKAGE_POC = 4,
  HIRECMD_VOLUME_RENT = 5,
  HIRECMD_GET_VOLUME_RENT_POC = 6,
  HIRECMD_VOLUME_RETRIEVE = 7,
  HIRECMD_VOLUME_RESET = 8,
  HIRECMD_GET_SYSTEMINFO = 9,
  HIRECMD_VOLUME_PACKAGE_CONTINUE = 10,
  HIRECMD_GET_HIRE_LOG = 11,
  HIRECMD_DO_POST_POC = 12,
  HIRECMD_CHECK_RPC_URL = 13,
  HIRECMD_SET_ADMIN_PASSWD = 100,
} HIRE_COMMAND;

typedef enum _HIRE_STATUS
{
  HIREST_VOLUME_UNKNOW = 0,
  HIREST_VOLUME_UNPACKAGE = 1,
  HIREST_VOLUME_PACKAGING = 2,
  HIREST_VOLUME_PACKAGED = 3,
  HIREST_VOLUME_RENT_PACKAGING = 4,
  HIREST_VOLUME_IN_SERVICE = 5,
  HIREST_VOLUME_RETRIEVING = 6,
  HIREST_VOLUME_RESETING = 7
} HIRE_STATUS;


class hireHelper
{
public:
  static bool isBlockHash(const std::string &block_hash) {
    std::string empty_hash = "0x0000000000000000000000000000000000000000000000000000000000000000";
    int ipos_ux = block_hash.find("ux");
    int ipos_0x = block_hash.find("0x");
    if ((0 == ipos_ux || 0 == ipos_0x) && (empty_hash.size() == block_hash.size())) 
      return true;
    else return false;
  }

  static std::string from0xToux(const std::string &block_hash) {
    std::string block_hash_ux = block_hash;
    int ipos_0x = block_hash_ux.find("0x");
    if (0 == ipos_0x && 0 < block_hash_ux.size()) {
      if ('0' == block_hash_ux[0]) 
        block_hash_ux[0] = 'u';
    }

    return block_hash_ux;
  }

  static std::string fromuxTo0x(const std::string &block_hash) {
    std::string block_hash_0x = block_hash;
    int ipos_ux = block_hash_0x.find("ux");
    if (0 == ipos_ux && 0 < block_hash_0x.size()) {
      if ('u' == block_hash_0x[0]) 
        block_hash_0x[0] = '0';
    }

    return block_hash_0x;
  }
};
#endif //__HIREFS_LOCAL_DEFS_H
