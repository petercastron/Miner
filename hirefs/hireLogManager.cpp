#include "hireLogManager.h"
#include "loggerLocal.h"
#include "local_defs.h"
#include "auxHelper.h"

hireLogManager::hireLogManager()
{
  _init_locker_hire_log();
  _init_locker_hire_poc_post_details_list();

  _load_logs();
}

hireLogManager::~hireLogManager()
{
  _destroy_locker_hire_log();
  _destroy_locker_hire_poc_post_details_list();
}

void hireLogManager::add_admin_login_log(HIRE_LOG_ADMIN_LOGIN_ACTION_TYPE action_type, const std::string &src_ip)
{
  /*
  "data" : {
		"action_type" :                       
		"src_ip" : "x.x.x.x",                 
	}
  */
  FJson::Value hire_admin_login_log_data;
  
  hire_admin_login_log_data["action_type"] = action_type;
  hire_admin_login_log_data["src_ip"] = src_ip;

  _add_log(HIRE_LT_ADMIN_LOGIN, hire_admin_login_log_data.toCompatString());
}

void hireLogManager::add_volume_package_log(HIRE_LOG_VOLUME_PACKAGE_ACTION_TYPE action_type, const std::string &hire_block, const std::string &hire_volume)
{
  /*
  "data" : {
    "action_type" : x,                    
		"utg_block": "xxx",                   
    "utg_volume": "10"                    
  }   
  */
  FJson::Value hire_volume_package_log_data;
  
  hire_volume_package_log_data["action_type"] = action_type;
  hire_volume_package_log_data["utg_block"] = hire_block;
  hire_volume_package_log_data["utg_volume"] = hire_volume;

  _add_log(HIRE_LT_VOLUME_PACKAGE, hire_volume_package_log_data.toCompatString());
}

void hireLogManager::add_volume_rent_log(HIRE_LOG_VOLUME_RENT_ACTION_TYPE action_type, const std::string &hire_voucher, const std::string &hire_block, const std::string &hire_volume_rent, const std::string &hire_volume_free)
{
  /*
  "data" : {
    "action_type" : xx,                    
    "utg_voucher": "xxx",                  
    "utg_volume_rent": "xxx",              
    "utg_volume_free": "xxx", 	           
    "utg_block": "xxx"                     
  }
  */
  FJson::Value hire_volume_rent_log_data;
  
  hire_volume_rent_log_data["action_type"] = action_type;
  hire_volume_rent_log_data["utg_voucher"] = hire_voucher;
  hire_volume_rent_log_data["utg_block"] = hire_block;
  hire_volume_rent_log_data["utg_volume_rent"] = hire_volume_rent;
  hire_volume_rent_log_data["utg_volume_free"] = hire_volume_free;

  _add_log(HIRE_LT_VOLUME_RENT, hire_volume_rent_log_data.toCompatString());
}

void hireLogManager::_add_log(HIRE_LOG_TYPE log_type, const std::string &data, time_t ts)
{
  HIRE_LOG hire_log_item;
  hire_log_item.log_type = log_type;
  hire_log_item.data = data;
  if (0 == ts) hire_log_item.ts = time(NULL);
  else hire_log_item.ts = ts;

  _add_log(hire_log_item);
}

void hireLogManager::_add_log(HIRE_LOG & hire_log_item)
{ 
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "add log type : %u, ts : %u, data : %s", hire_log_item.log_type, hire_log_item.ts, hire_log_item.data.c_str());

  lock_hire_log();
  _hire_log_list.push_back(hire_log_item);
  unlock_hire_log();

  _save_logs();
}


void hireLogManager::add_poc_post_record(const std::string &hire_voucher, const std::string &hire_block, const std::string &hire_rpc_url, const std::string &hire_commit_result)
{
  debugEntry (LL_DEBUG, LOG_MODULE_INDEX_HIRE, "add poc record voucher(%s), block_number(%s), hire_rpc_url(%s), result(%s)", 
                                                hire_voucher.c_str(), hire_block.c_str(), hire_rpc_url.c_str(), hire_commit_result.c_str());
  
  HIRE_POC_POST_RECORD poc_post_record_item;
  poc_post_record_item.hire_voucher = hire_voucher;
  poc_post_record_item.hire_block = hire_block;
  poc_post_record_item.hire_rpc_url = hire_rpc_url;
  if (hireHelper::isBlockHash(hire_commit_result))
    poc_post_record_item.hire_poc_commit_result = hire_commit_result;
  else poc_post_record_item.hire_poc_commit_error = hire_commit_result;
  poc_post_record_item.hire_commit_ts = time(NULL);
  
  lock_hire_poc_post_details_list();
  _hire_poc_post_data.hire_poc_post_details_list.push_back(poc_post_record_item);
  unlock_hire_poc_post_details_list();
}

void hireLogManager::set_poc_post_commit_confirm(const std::string &hire_commit_result, const std::string &hire_commit_confirm)
{
  std::list<HIRE_POC_POST_RECORD>::iterator it;

  lock_hire_poc_post_details_list();
  for (it = _hire_poc_post_data.hire_poc_post_details_list.begin(); it != _hire_poc_post_data.hire_poc_post_details_list.end(); it++) {
    if (it->hire_poc_commit_result == hire_commit_result) {
        it->hire_poc_commit_confirm = hire_commit_confirm;
        debugEntry (LL_DEBUG, LOG_MODULE_INDEX_HIRE, 
                    "set poc post commit confirm voucher(%s), block_number(%s), hire_rpc_url(%s), result(%s), confirm(%s)",
                     it->hire_voucher.c_str(), it->hire_block.c_str(), it->hire_rpc_url.c_str(), it->hire_poc_commit_result.c_str(),
                      it->hire_poc_commit_confirm.c_str());
    }
  }
  unlock_hire_poc_post_details_list();
}

void hireLogManager::get_logs(u_int log_type, FJson::Value &logs)
{
  std::list<HIRE_LOG>::iterator it;

  lock_hire_log();
  for (it = _hire_log_list.begin(); it != _hire_log_list.end(); it++) {
    if (0 != (log_type & it->log_type)) {
        FJson::Value log_item, log_data;
        log_item["log_type"] = it->log_type;
        log_item["ts"] = it->ts;

        FJson::Parser parser;
        parser.load_string(it->data, log_data);
        log_item["data"] = log_data;

        logs.push_back(log_item);
    }
  }
  unlock_hire_log();

  if (0 != (HIRE_LT_VOLUME_POC_POST_STATUS & log_type)) {
      FJson::Value log_item, log_data, hire_poc_commit_details;
      log_item["log_type"] = HIRE_LT_VOLUME_POC_POST_STATUS;
      log_item["ts"] = time(NULL);
      
      //
      log_data["utg_chain_day"] = _hire_poc_post_data.hire_chain_day;
      log_data["utg_poc_commit_progress"] = _hire_poc_post_data.hire_poc_commit_progress;
      log_data["utg_poc_commit_retry"] = _hire_poc_post_data.hire_poc_commit_retry;
      log_data["utg_poc_commit_error"] = _hire_poc_post_data.hire_poc_commit_error;
      log_data["utg_poc_commit_plan_ts"] = _hire_poc_post_data.hire_poc_commit_plan_ts;
      char szbuffer[128] = {0};
      sprintf(szbuffer, "%llu", _hire_poc_post_data.hire_poc_commit_plan_block);
      log_data["utg_poc_commit_plan_block"] = std::string(szbuffer);

      std::list<HIRE_POC_POST_RECORD>::iterator it;
      lock_hire_poc_post_details_list();
      for (it = _hire_poc_post_data.hire_poc_post_details_list.begin(); it != _hire_poc_post_data.hire_poc_post_details_list.end(); it++) {
        FJson::Value hire_poc_commit_record;
        hire_poc_commit_record["utg_voucher"] = it->hire_voucher;
        hire_poc_commit_record["utg_poc_commit_result"] = it->hire_poc_commit_result;
        hire_poc_commit_record["utg_poc_commit_error"] = it->hire_poc_commit_error;
        hire_poc_commit_record["utg_poc_commit_confirm"] = it->hire_poc_commit_confirm;
        hire_poc_commit_record["utg_rpc_url"] = it->hire_rpc_url;
        hire_poc_commit_record["utg_block"] = it->hire_block;
        hire_poc_commit_record["utg_commit_ts"] = it->hire_commit_ts;

        hire_poc_commit_details.push_back(hire_poc_commit_record);
      }
      unlock_hire_poc_post_details_list();

      if (hire_poc_commit_details.isArray() && 0 < hire_poc_commit_details.size())
        log_data["utg_poc_commit_details"] = hire_poc_commit_details;

      log_item["data"] = log_data;

      logs.push_back(log_item);
  }
}

void hireLogManager::_save_logs()
{
  FJson::Value root, logs;

  get_logs(HIRE_LT_MAX, logs);
  if (logs.isArray() && 0 < logs.size()) {
    root["logs"] = logs;
    auxHelper::save_to_file(HIREFS_LOGS, root.toCompatString());
  }
}

void hireLogManager::_load_logs()
{
  /*
  {
    "logs" : [
      {
        "log_type" 1,                             
        "ts" : xx,                     
        "data" : {}
      }
    ]
   }
  */
  FJson::Value root, hire_logs_js;
  FJson::Parser parser;
  std::string hire_logs = "";
  
  do {
    
    if (false == auxHelper::read_from_file(HIREFS_LOGS, hire_logs)) {
      debugEntry (LL_WARN, LOG_MODULE_INDEX_HIRE, "read hirefs poc post history log file(%s) fail.", HIREFS_LOGS);
      break;
    }
    
    if (false == parser.load_string(hire_logs, root)) {
      debugEntry (LL_WARN, LOG_MODULE_INDEX_HIRE, "can not parser poc post history log file.");
      break;
    }
  
    if (root.isMember("logs")) {
      hire_logs_js = root["logs"];
      if (hire_logs_js.isArray() && 0 < hire_logs_js.size()) {
        for (size_t i = 0; i < hire_logs_js.size(); i++) {
          HIRE_LOG hire_log_item;

          FJson::Value hire_logs_item = hire_logs_js[i];
          if (hire_logs_item["log_type"].isInt())
            hire_log_item.log_type = (HIRE_LOG_TYPE)hire_logs_item["log_type"].asUInt();

          if (hire_logs_item["ts"].isInt())
           hire_log_item.ts = hire_logs_item["ts"].asUInt();

          if (hire_logs_item.isMember("data"))
           hire_log_item.data = hire_logs_item["data"].toCompatString();

          lock_hire_log();
          _hire_log_list.push_back(hire_log_item);
          unlock_hire_log();

          debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "load log type : %u, ts : %u, data : %s", hire_log_item.log_type, hire_log_item.ts, hire_log_item.data.c_str());
        }
      }
    }
  } while (false); 
}
