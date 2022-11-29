#ifndef HIRE_LOG_MANAGER_H
#define HIRE_LOG_MANAGER_H

#include "singleton.h"
#include "FJson.h"
#include "defs.h"
#include "commonHead.h"
#include <list>

typedef enum _HIRE_LOG_TYPE
{
  HIRE_LT_NONE = 0,
  HIRE_LT_ADMIN_LOGIN = 0x1,                       
  HIRE_LT_VOLUME_PACKAGE = 0x2,                    
  HIRE_LT_VOLUME_RENT = 0x4,                       

  //
  HIRE_LT_VOLUME_POC_POST_STATUS = 0x10000,        //65536
  HIRE_LT_MAX = 0xffffffff,                        //all log
} HIRE_LOG_TYPE;

typedef struct _HIRE_LOG
{
  HIRE_LOG_TYPE log_type;
  time_t ts;
  std::string data;
}HIRE_LOG;

typedef enum _HIRE_LOG_ADMIN_LOGIN_ACTION_TYPE {
  HIRE_LOG_ACT_USERLOGIN = 1,       
  HIRE_LOG_ACT_CHANGEPWD = 2,       
  HIRE_LOG_ACT_RESETPWD = 3,        
}HIRE_LOG_ADMIN_LOGIN_ACTION_TYPE;

typedef enum _HIRE_LOG_VOLUME_PACKAGE_ACTION_TYPE {
  HIRE_LOG_ACT_BEGIN_PACKAGE = 11,       
  HIRE_LOG_ACT_CONTINUE_PACKAGE = 12,    
  HIRE_LOG_ACT_END_PACKAGE = 13,         
  HIRE_LOG_ACT_RESET_PACKAGE = 14,       
}HIRE_LOG_VOLUME_PACKAGE_ACTION_TYPE;

typedef enum _HIRE_LOG_VOLUME_RENT_ACTION_TYPE {
  HIRE_LOG_ACT_RENT = 21,                              
  HIRE_LOG_ACT_RETRIEVE_BY_ENQUIRE_TIMEOUT = 22,       
  HIRE_LOG_ACT_RETRIEVE_BY_DUE = 23,                   
  HIRE_LOG_ACT_RETRIEVE_BY_MANUAL = 24,                
  HIRE_LOG_ACT_RENT_VOLUME_PARTITIONING = 25,          
  HIRE_LOG_ACT_RENT_VOLUME_PARTITIONING_RETRIEVE = 26, 
}HIRE_LOG_VOLUME_RENT_ACTION_TYPE;

typedef enum _HIRE_LOG_POC_COMMIT_PROGRESS {
  HIRE_LOG_POC_COMMIT_PROGRESS_PLANING = 1,     
  HIRE_LOG_POC_COMMIT_PROGRESS_PLANED = 2,      
  HIRE_LOG_POC_COMMIT_PROGRESS_COMMITED = 3,    
  HIRE_LOG_POC_COMMIT_PROGRESS_CONFIRMED = 4,   
}HIRE_LOG_POC_COMMIT_PROGRESS;

typedef struct _HIRE_POC_POST_RECORD
{
  std::string hire_voucher;
  std::string hire_block;
  std::string hire_rpc_url;
  std::string hire_poc_commit_result;
  std::string hire_poc_commit_error;
  std::string hire_poc_commit_confirm;
  time_t hire_commit_ts;
} HIRE_POC_POST_RECORD;

typedef struct _HIRE_POC_POST_DATA {
  u_int hire_chain_day;
  HIRE_LOG_POC_COMMIT_PROGRESS hire_poc_commit_progress;
  bool hire_poc_commit_retry;
  std::string hire_poc_commit_error;
  u_int64_t hire_poc_commit_plan_block;
  time_t hire_poc_commit_plan_ts;
  std::list<HIRE_POC_POST_RECORD> hire_poc_post_details_list;
} HIRE_POC_POST_DATA;

class hireLogManager : public singleton<hireLogManager>
{
public:
  hireLogManager();
  ~hireLogManager();
public:
  LOCK_V(hire_log);
  LOCK_V(hire_poc_post_details_list);
public:
  void add_admin_login_log(HIRE_LOG_ADMIN_LOGIN_ACTION_TYPE action_type, const std::string &src_ip);
  void add_volume_package_log(HIRE_LOG_VOLUME_PACKAGE_ACTION_TYPE action_type, const std::string &hire_block, const std::string &hire_volume);
  void add_volume_rent_log(HIRE_LOG_VOLUME_RENT_ACTION_TYPE action_type, const std::string &hire_voucher, const std::string &hire_block, const std::string &hire_volume_rent, const std::string &hire_volume_free);

  HIRE_POC_POST_DATA &get_hire_poc_post_data() { return _hire_poc_post_data; }
  void add_poc_post_record(const std::string &hire_voucher, const std::string &hire_block, const std::string &hire_rpc_url, 
                           const std::string &hire_commit_result);
  void set_poc_post_commit_confirm(const std::string &hire_commit_result, const std::string &hire_commit_confirm);

  void get_logs(u_int log_type, FJson::Value &logs);
private:
  void _add_log(HIRE_LOG_TYPE log_type, const std::string &data, time_t ts = 0);
  void _add_log(HIRE_LOG & hire_log_item);

  void _save_logs();
  void _load_logs();
private:
  std::list<HIRE_LOG> _hire_log_list;
  HIRE_POC_POST_DATA _hire_poc_post_data;
};

#endif //HIRE_LOG_MANAGER_H