#if !defined(__COMMON_DEFS_H)
#define __COMMON_DEFS_H

typedef enum
{
  CE_SUCC = 0,
  CE_FAIL,
  CE_SOCKET,        // connect fail
  CE_DISCONNECTED,
  CE_VERSION_DISMATCH,
  CE_PROTOCOL,
  CE_TIMEOUT,
  CE_INVALID_USER, // server return error
  CE_INVLIAD_PASS,
  CE_MAX_DEVICE,
  CE_NO_GATEWAY = 10,
  CE_NO_NETWORK,
  CE_DEVICE_NOTEXISTS,
  CE_DEVICE_ONLINED,
  CE_DEVICE_DISABLED,
  CE_USER_UNACTIVE,
  CE_USER_LOCKED,
  CE_INVALID_TUN_DEVICE, // end of server error
  CE_CANCEL,
  CE_REDIRECT,
  CE_AUX_AUTH_DISMATCH = 20,
  CE_INVALID_PARTNERID,
  CE_INVALID_APPID,
  CE_PENDING,
  CE_STATUS,
  CE_UNINITIALIZED,
  CE_NETWORK_UNREACHABLE,
  CE_INVALID_AUTHORIZATION,
  CE_UNKNOWN_DEVCODE,
  CE_INVALID_SN,
  CE_NO_SN_SELECTED = 30,
  CE_OPERATION_DENIED,
  CE_MEMERY_OUT,
  CE_INVALID_SMS,
  CE_INVALID_TICKET,
  CE_TRY_TOO_MANY_TIMES,
  CE_INVALID_DEIVE_CLASS,
  CE_CALL_THIRD_API_FAIL,
  CE_INVALID_CODE,
  CE_MODULE_LOST,
  CE_GW_AUTH_TIMEOUT = 40,
  CE_QOS_METER,
  CE_PACKAGE_DROP,
  CE_PACKAGE_PROCESSED_BY_OTHER,
  CE_AUTHORIZING,
  CE_AUTHORIZIE_DENIAL,
  CE_INVALID_SIGNATURE,
  CE_OTHER_INSTANCE_LOGIN,
  CE_AREALOCK,
  CE_CORE_TIMEOUT = 1000,
  CE_INVALID_PARAMETER = 1001,
  CE_MODULE_BUSY = 1002,
  CE_INTERNAL_SERVER_ERROR = 2000,
} CN_ERR;

#endif

