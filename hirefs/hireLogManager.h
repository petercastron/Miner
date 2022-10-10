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
  HIRE_LOG_ACT_USERLOGIN = 1,       //用户登录
  HIRE_LOG_ACT_CHANGEPWD = 2,       //修改密码
  HIRE_LOG_ACT_RESETPWD = 3,        //重置密码
}HIRE_LOG_ADMIN_LOGIN_ACTION_TYPE;

typedef enum _HIRE_LOG_VOLUME_PACKAGE_ACTION_TYPE {
  HIRE_LOG_ACT_BEGIN_PACKAGE = 11,       //开始封装
  HIRE_LOG_ACT_CONTINUE_PACKAGE = 12,    //开机后继续封装
  HIRE_LOG_ACT_END_PACKAGE = 13,         //封装结束
  HIRE_LOG_ACT_RESET_PACKAGE = 14,       //封装解除
}HIRE_LOG_VOLUME_PACKAGE_ACTION_TYPE;

typedef enum _HIRE_LOG_VOLUME_RENT_ACTION_TYPE {
  HIRE_LOG_ACT_RENT = 21,                             //空间租用
  HIRE_LOG_ACT_RETRIEVE_BY_ENQUIRE_TIMEOUT = 22,      //空间租用质押超时回收
  HIRE_LOG_ACT_RETRIEVE_BY_DUE = 23,                  //空间租用到期回收
}HIRE_LOG_VOLUME_RENT_ACTION_TYPE;

typedef enum _HIRE_LOG_POC_COMMIT_PROGRESS {
  HIRE_LOG_POC_COMMIT_PROGRESS_PLANING = 1,     //正在制定提交计划
  HIRE_LOG_POC_COMMIT_PROGRESS_PLANED = 2,      //计划制定成功，等待(或者重新)提交
  HIRE_LOG_POC_COMMIT_PROGRESS_COMMITED = 3,    //提交成功，等待验证
  HIRE_LOG_POC_COMMIT_PROGRESS_CONFIRMED = 4,   //验证成功
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
  /*管理员登录日志
    @action_type : 见HIRE_LOG_ADMIN_LOGIN_ACTION_TYPE定义
    @src_ip      : 来源IP
  */
  void add_admin_login_log(HIRE_LOG_ADMIN_LOGIN_ACTION_TYPE action_type, const std::string &src_ip);

  /*容量封装和解封装日志
    @action_type : 见HIRE_LOG_VOLUME_PACKAGE_ACTION_TYPE定义
    @hire_block   : 封装基于的区块高度
    @hire_volume  : 单位GB action_type取1,3,4时为封装大小，action_type取2时为已完成的封装大小
  */
  void add_volume_package_log(HIRE_LOG_VOLUME_PACKAGE_ACTION_TYPE action_type, const std::string &hire_block, const std::string &hire_volume);

  /*容量出租和回收日志
    @action_type     : 见HIRE_LOG_VOLUME_RENT_ACTION_TYPE定义
    @hire_voucher     : 租用凭证
    @hire_block       : 租用时基于的区块高度
    @hire_volume_rent : 单位GB 租用的空间
    @hire_volume_free : 单位GB action_type取1为租用后的剩余自由空间，action_type取2为回收后的剩余自由空间
  */
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