#ifndef HIRE_MANAGER_H
#define HIRE_MANAGER_H

#include "local_defs.h"
#include "websocket.h"
#include "websocket_server.hpp"
#include "hireVolume.h"
#include "rlpvalue/rlpvalue.h"
#include <list>
#include <map>

#define HIRE_MODULE_NAME                            "HIREBLOCKCHAIN"
#define HIRE_LOGIN_PASSWD_MD5                       "62bd42444ec478f524378a0c19bca2ad"    //admin@hire

#define HIRE_VOLUME_MIN_GB                                   5        //GB

#define HIRE_ENQUIRE_CHECK_PERIOD                            30                                //sec 
#define HIRE_ENQUIRE_PLEDGE_PERIOD                           (10 * HIRE_ENQUIRE_CHECK_PERIOD)   //sec

#define HIRE_COMMIT_CHECK_PERIOD                             30       //sec  
#define HIRE_ONE_BLOCK_NUMBER_TIME                           10       //sec
#define HIRE_ONE_DAY_BLOCK_NUMBER                            8640
#define HIRE_GETBACK_BLOCK_NUMBER                            20
#define HIRE_WAIT_BLOCK_NUMBER_WHEN_COMMIT                   3
#define HIRE_POC_POST_RESULT_CHECK_TIMES                     3

#define HIRE_SYSTEMINFO_CHECK_PERIOD                         5        //sec

#define HIRE_POST_POC_TYPE_VOLUME_PACKAGE                    "do_volume_package_poc"
#define HIRE_POST_POC_TYPE_RENT                              "do_volume_rent_poc"
#define HIRE_POST_POC_TYPE_RENT_RETRIEVE                     "do_volume_rent_retrieve_poc"

//errno message
#define HIRE_OUT_PLEDGE                                      "out pledge"
#define HIRE_CHECK_POC_FAIL                                  "check poc fail"
#define HIRE_ENQUIRE_PLEDGE_FAIL                             "enquire pledge fail"
#define HIRE_ENQUIRE_BLOCK_FAIL                              "enquire block fail"
#define HIRE_ENQUIRE_NONCE_FAIL                              "enquire nonce fail"
#define HIRE_ENQUIRE_GASPRICE_FAIL                           "enquire gasPrice fail"
#define HIRE_ENQUIRE_GAS_FAIL                                "enquire gas fail"
#define HIRE_POST_TO_CHAIN_RPC_FAIL                          "hire post data to chain rpc fail"

typedef struct _ws_session {
  std::string ip;
  std::string request_uri;
  std::string ticket;
  time_t ts;
} ws_session;

typedef struct MEMPACKED         
{  
  char name1[20];      
  unsigned long MemTotal;  

  char name2[20];  
  unsigned long MemFree;

  char name3[20];  
  unsigned long MemAvailable;  

  char name4[20];  
  unsigned long Buffers;  

  char name5[20];  
  unsigned long Cached;  
  
  char name6[20];  
  unsigned long SwapCached;  
} MEM_OCCUPY;  
  
typedef struct CPUPACKED         
{  
  char name[20];      
  unsigned int user; 
  unsigned int nice; 
  unsigned int system;
  unsigned int idle;
  unsigned int lowait;  
  unsigned int irq;  
  unsigned int softirq;  
} CPU_OCCUPY;  

class hireManager
{
public:
  hireManager();
  ~hireManager();
public:
  bool start();
  void stop();
  void wait_stop();

  bool load_hire_config(const std::string &hirefs_config_file_name);
  bool set_hirefs_config(FJson::Value &hirefs_cfg);
public:
  static int write_callback(void *context, u_char *data, size_t size);
public:
  //thread function
  static void *thread_hirefs_websocket_handle_worker(void *param);
  void hirefs_websocket_worker();

  static void *thread_hirefs_enquire_handle_worker(void *param);
  void hirefs_enquire_worker();

  static void *thread_hirefs_commit_poc_handle_worker(void *param);
  void hirefs_commit_poc_worker();
  
  static void *thread_hirefs_getsysteminfo_handle_worker(void *param);
  void hirefs_getsysteminfo_worker();

  static bool websocket_connect_handle(const std::string &ip, u_short port, const std::string &request_uri, void *parmame);
  void process_websocket_connect(const std::string &ip, u_short port, const std::string &request_uri);

  static bool websocket_message_handle(const std::string &ip, u_short port, const std::string &message, std::string &ret_message, void *parmame);
  bool process_websocket_message(const std::string &ip, u_short port, const std::string &message, std::string &ret_message);
public:
  LOCK_V(ws_session);
  LOCK_V(rent_items_outpledge_list);
  LOCK_V(hire_chain_post_urls);
private:
  bool _process_websocket_login(ws_session &session, const std::string &message, std::string &ret_message);
  bool _process_reset_adminpasswd(ws_session &session, const std::string &message, std::string &ret_message);

  bool _process_websocket_device_contrl(ws_session &session, const std::string &message, std::string &ret_message);
  void _process_message_get_hire_info(FJson::Value& root);
  void _process_message_set_hire_info(ws_session &session, FJson::Value& root);
  void _process_message_volume_package(FJson::Value& root);
  void _process_message_get_volume_package_poc(FJson::Value& root);
  void _process_message_volume_rent(FJson::Value& root);
  void _process_message_get_volume_rent_poc(FJson::Value& root);
  void _process_message_volume_retrieve(FJson::Value& root);
  void _process_message_volume_reset(FJson::Value& root);
  void _process_message_get_systeminfo(FJson::Value& root);
  void _process_message_get_hire_log(FJson::Value& root);
  void _process_message_do_poc_post(FJson::Value& root);
  void _process_message_check_rpc_url(FJson::Value& root);
  void _process_message_set_adminpasswd(ws_session &session, FJson::Value& root);

  std::string _httpsession_post_msg(const std::string &url, const std::string &msg);
  std::string _httpsession_get_msg(const std::string &url);
  
  void _get_hire_config(FJson::Value &hire_config);
  bool _save_hire_config();

  bool _load_volume_bytes();
  bool _load_admin_passwd();
  bool _save_admin_passwd();

  bool _load_private_key();
  bool _sava_private_key();

  bool _load_poc_commit_history(const std::string &hire_poc_history_file, u_int64_t &block_number);
  bool _save_poc_commit_history(const std::string &hire_poc_history_file, const std::string &block_number);
  
  //状态维护
  void _check_hire_status();

  //链可能回滚，所以最新块的往前推20块
  std::string _enquire_block_number(const std::string &hire_chain_post_url);
  bool _get_block_as_random_seed(const std::string &hire_chain_post_url, FJson::Value &root);
  //封装剩余空间POC证明
  bool _do_post_volume_package_poc_to_hirechain();
  bool _do_post_volume_package_poc_to_hirechain(const std::string &hire_chain_post_url);
  //提交封装空间和出租空间POC证明
  bool _do_post_poc_collection_to_hirechain_by_hire_poc_group_name(const std::string &hire_poc_group_name, std::string &hire_poc_collection_post_result);
  bool _do_post_poc_collection_to_hirechain_by_hire_poc_group_name(std::map<std::string, std::string> &hire_poc_collection_post_results, const std::string &hire_poc_group_name);
  bool _do_post_poc_collection_to_hirechain_by_hire_poc_group_name(const std::string &hire_chain_post_url, std::map<std::string, std::string> &hire_poc_collection_post_results, const std::string &hire_poc_group_name);
  //每日提交一次
  bool _do_post_poc_collection_to_hirechain_every_day(std::map<std::string, std::string> &hire_poc_collection_post_results);
  //出租空间回收证明
  bool _do_volume_rent_retrieve_poc_to_hirechain();
  bool _do_volume_rent_retrieve_poc_to_hirechain(const std::string &hire_chain_post_url);
  //检测POC证明POST的结果
  bool _check_poc_collection_post_result_from_hirechain(std::map<std::string, std::string> &hire_poc_collection_post_results, std::vector<std::string> &hire_poc_group_name_of_poc_collection_post_fail);
  //RPC接口POST数据到链
  bool _do_post_poc_to_hirechain(const std::string &hire_chain_post_url, const std::string &hire_poc, std::string &result);

  //
  bool _enquire_pledge();
  bool _enquire_pledge(const std::string &hire_chain_post_url);
  bool _enquire_pledge_old(const std::string &hire_enquire_pledge_url);
  bool _enquire_pledge_rpc(const std::string &hire_chain_post_url);

  //hire block chain rpc
  u_int64_t _enquire_lasted_block_number();
  u_int64_t _enquire_lasted_block_number(const std::string &hire_chain_post_url);
  bool _commit_block_to_hirechain(const std::string &hire_chain_post_url, const std::string &hire_block_data, std::string &result);
  bool _check_transaction_status_from_hirechain(const std::string &block_hash, const std::vector<HIRE_RENT_ITEM> &hire_poc_items);
  bool _check_transaction_status_from_hirechain(const std::string &hire_chain_post_url, const std::string &block_hash, const std::vector<HIRE_RENT_ITEM> &hire_poc_items);
  //numer 可以取"0x512", "latest", "earliest", "pending"
  bool _get_block_from_hirechain(const std::string &hire_chain_post_url, const std::string &numer, FJson::Value &result_js);
  bool _get_gasPrice_from_hirechain(const std::string &hire_chain_post_url, u_int64_t &gasPrice);
  bool _get_gasLimit_from_hirechain(const std::string &hire_chain_post_url, const std::string &data_hexstring_out, const std::string &hire_address, u_int64_t value, u_int64_t gasPrice, u_int64_t &gasLimit);
  bool _get_nonce_from_hirechain(const std::string &hire_chain_post_url, const std::string &hire_address, u_int64_t &nonce);
  bool _get_chainid_from_hirechain(const std::string &hire_chain_post_url, u_int64_t &chain_id);
  
  void _hirefs_poc_to_block_data(std::string &data_hexstring_out, const std::string &hire_address, u_int64_t hire_volume_Bytes, const std::string &hirefs_poc);
  void _hirefs_data_to_block(std::string &block_hexstring_out, int chain_id, u_int64_t nonce, u_int64_t gasPrice, 
                           u_int64_t gasLimit, const std::string &hire_address, u_int64_t value,
                           const std::string &data_hexstring, const std::string &hire_der_prikey);
  void _hirefs_rlp_Eip155hash(u_char *hash, const RLPValue &block_rlp, int chain_id);

  //
  void _web3jrpc_request(FJson::Value &request_js, u_int request_id, const std::string &method, FJson::Value &params_js);

  void hire_append_signature_to_rlp(int chain_id, RLPValue &rlp, int v, const std::string &r, const std::string &s);

  //system info  : memery cpu
  u_int _get_mem_used();
  int _get_cpuoccupy(CPU_OCCUPY *cpust);
  double _cal_cpuoccupy(CPU_OCCUPY *o, CPU_OCCUPY *n);
private:
  bool _stop_flag;
  //
  Server _websocket_server;
  std::map<std::string, ws_session> _ws_sessions;

  thread_worker _hirefs_message_handle_thread;
  thread_worker _hirefs_enquire_handle_thread;
  thread_worker _hirefs_commit_handle_thread;
  thread_worker _hirefs_getsysteminfo_handle_thread;

  cond_event _hirefs_enquire_event;
  msg_queue<std::string> _hirefs_enquire_msg_queue;

  cond_event _hirefs_commit_event;
  msg_queue<std::string> _hirefs_commit_msg_queue;

  std::string _hire_admin_passwd_md5;
  std::string _hire_private_key;
  
  bool _bhire_onpledge; //true on pledge, false out pledge
  int _hire_chain_id;
  u_int _hire_one_day_block_number;
  u_int _hire_volume_retrieve_block_number;
  u_int64_t _hire_last_block_number;
  std::vector<std::string> _hire_chain_post_urls;
  std::string _hire_enquire_pledge_url;
  std::string _hirefs_config_file_name;
  std::string _hire_config_name;

  u_int _cpu_used;
  u_int _mem_used;
  u_int64_t _volume_free;
  u_int64_t _volume_total;

  hireVolume _hire_volume;

  time_t _ts_volume_package_poc_commit;
  std::list<std::string> _hire_rent_items_outpledge;
};
#endif //HIRE_MANAGER_H
