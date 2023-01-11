#include "hireManager.h"
#include "loggerLocal.h"
#include "local_defs.h"
#include "auxHelper.h"
#include "rc4.h"
#include "hmac_sha256.h"
#include "blockChainTools.h"
#include "httpsession.h"
#include "rlpvalue/InfInt.h"
#include "sha3.h"
#include "hireversion.h"
#include "hireLogManager.h"
#include <openssl/sha.h>
#include <sys/statfs.h>
#include <cmath>

hireManager::hireManager() 
{
  _stop_flag = true;
  _cpu_used = 0;
  _mem_used = 0;
  _volume_free = 0;
  _volume_total = 0;
  _bhire_onpledge = false;
  _hire_admin_passwd_md5 = HIRE_LOGIN_PASSWD_MD5;
  _ts_volume_package_poc_commit = 0;
  _hire_one_day_block_number = HIRE_ONE_DAY_BLOCK_NUMBER;
  _hire_volume_retrieve_block_number = HIRE_ONE_DAY_BLOCK_NUMBER;
  _hire_config_name = "";
  _hire_chain_id = 0;
  _hire_last_block_number = 0;

  _init_locker_ws_session();
  _init_locker_rent_items_outpledge_list();
  _init_locker_hire_chain_post_urls();
}

hireManager::~hireManager()
{
  _destroy_locker_ws_session();
  _destroy_locker_rent_items_outpledge_list();
  _destroy_locker_hire_chain_post_urls();
}

bool hireManager::start()
{
  _stop_flag = false;
  
  _hirefs_enquire_event.event_init("hirefs_enquire_event");
  _hirefs_commit_event.event_init("hirefs_commit_event");
  _hirefs_rent_volume_partitioning_event.event_init("hirefs_rent_volume_partitioning_event");
  
  debugEntry(LL_INFO, LOG_MODULE_INDEX_HIRE, "HIRE VERSION : %s", HIRE_PRODUCT_VERSION);

  _load_admin_passwd();
  _load_private_key();

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "create hirefs message handle worker thread.");
  if (false == _hirefs_message_handle_thread.thread_worker_start(hireManager::thread_hirefs_websocket_handle_worker, this)) {
    _stop_flag = true;
    debugEntry(LL_ERROR, LOG_MODULE_INDEX_HIRE, "create hirefs message handle worker thread fail.");
    return false;
  }

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "create hirefs enquire handle worker thread.");
  if (false == _hirefs_enquire_handle_thread.thread_worker_start(hireManager::thread_hirefs_enquire_handle_worker, this)) {
    _stop_flag = true;
    debugEntry(LL_ERROR, LOG_MODULE_INDEX_HIRE, "create hirefs enquire handle worker thread fail.");
    return false;
  }

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "create hirefs commit handle worker thread.");
  if (false == _hirefs_commit_handle_thread.thread_worker_start(hireManager::thread_hirefs_commit_poc_handle_worker, this)) {
    _stop_flag = true;
    debugEntry(LL_ERROR, LOG_MODULE_INDEX_HIRE, "create hirefs commit handle worker thread fail.");
    return false;
  }

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "create hirefs get systeminfo handle worker thread.");
  if (false == _hirefs_getsysteminfo_handle_thread.thread_worker_start(hireManager::thread_hirefs_getsysteminfo_handle_worker, this)) {
    _stop_flag = true;
    debugEntry(LL_ERROR, LOG_MODULE_INDEX_HIRE, "create hirefs get systeminfo handle worker thread fail.");
    return false;
  }

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "create hirefs rent volume partitioning handle worker thread.");
  if (false == _hirefs_rent_volume_partitioning_handle_thread.thread_worker_start(hireManager::thread_hirefs_rent_volume_partitioning_handle_worker, this)) {
    _stop_flag = true;
    debugEntry(LL_ERROR, LOG_MODULE_INDEX_HIRE, "create hirefs rent volume partitioning handle worker thread fail.");
    return false;
  }

  _hire_volume.start();
}

void hireManager::stop()
{
  _stop_flag = true;

  _hirefs_enquire_event.event_notify();
  _hirefs_commit_event.event_notify();
  _hirefs_rent_volume_partitioning_event.event_notify();
  _websocket_server.stop();
  _hire_volume.stop();
}

void hireManager::wait_stop()
{  
  _hirefs_message_handle_thread.wait_thread_stop();
  _hirefs_enquire_handle_thread.wait_thread_stop();
  _hirefs_commit_handle_thread.wait_thread_stop();
  _hirefs_rent_volume_partitioning_handle_thread.wait_thread_stop();
  _hire_volume.wait_stop();

  _hirefs_enquire_event.event_close();
  _hirefs_commit_event.event_close();
  _hirefs_rent_volume_partitioning_event.event_close();
}

bool hireManager::load_hire_config(const std::string &hirefs_config_file_name)
{
  FJson::Value root;
  FJson::Parser parser;
  std::string config_data = "";
  
  _hirefs_config_file_name = hirefs_config_file_name;
  if (false == auxHelper::read_from_file(hirefs_config_file_name, config_data)) {
    debugEntry (LL_WARN, LOG_MODULE_INDEX_HIRE, "read hirefs config file(%s) fail.", hirefs_config_file_name.c_str());
    return false;
  }
  
  if (false == parser.load_string(config_data, root)) {
    debugEntry (LL_WARN, LOG_MODULE_INDEX_HIRE, "can not parser hirefs config file.");
    return false;
  }

  debugEntry (LL_DEBUG, LOG_MODULE_INDEX_HIRE, "load hirefs config :  %s.", root.toCompatString().c_str());

  return set_hirefs_config(root);
}

bool hireManager::set_hirefs_config(FJson::Value &hirefs_cfg)
{
  /*
  {
    "name" : "xxx",
    "chain_id": xxx,
    "utg_retrieve_block_number" : xxx,
    "utg_one_day_block_number"  : xxxx,
    "utg_enquire_pledge_url" : "xxx",
    "utg_chain_urls" : [
      "https://rpc.ultronglow.io",
      "https://rpc.ultronglow.io"
    ]
  }
  */
  bool rtCode = false;

  do {
    if (hirefs_cfg.isNull()) break;

    if (hirefs_cfg.isMember("name") && hirefs_cfg["name"].isString())
      _hire_config_name = hirefs_cfg["name"].asString();

    if (hirefs_cfg.isMember("chain_id") && hirefs_cfg["chain_id"].isInt())
      _hire_chain_id = hirefs_cfg["chain_id"].asInt();
    if (0 == _hire_chain_id) break;

    if (hirefs_cfg.isMember("utg_enquire_pledge_url")) 
      _hire_enquire_pledge_url = hirefs_cfg["utg_enquire_pledge_url"].asString();

    if (hirefs_cfg.isMember("utg_chain_urls")) {
      FJson::Value hire_chain_post_urls_js = hirefs_cfg["utg_chain_urls"];
      
      if (hire_chain_post_urls_js.isArray() && 0 < hire_chain_post_urls_js.size()) {
        _hire_chain_post_urls.clear();
        for (size_t i = 0; i < hire_chain_post_urls_js.size(); i++) {
          if (hire_chain_post_urls_js[i].isString()) {
            std::string hire_chain_post_url = hire_chain_post_urls_js[i].asString();
            _hire_chain_post_urls.push_back(hire_chain_post_url);
            debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "HIRE : add hire_chain_post_url : %s", hire_chain_post_url.c_str());
          }
        }
      }
    }
    if (_hire_chain_post_urls.empty()) break;


    if (hirefs_cfg.isMember("utg_retrieve_block_number") && hirefs_cfg["utg_retrieve_block_number"].isInt())
      _hire_volume_retrieve_block_number = hirefs_cfg["utg_retrieve_block_number"].asUInt();

    if (hirefs_cfg.isMember("utg_one_day_block_number") && hirefs_cfg["utg_one_day_block_number"].isInt())
      _hire_one_day_block_number = hirefs_cfg["utg_one_day_block_number"].asUInt();
    
    rtCode = true;
    
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "HIRE : chain id(0x%x), hire_enquire_pledge_url(%s), hire_one_day_block_number(%u), hire_retrieve_block_number(%u)", 
              _hire_chain_id, _hire_enquire_pledge_url.c_str(), _hire_one_day_block_number, _hire_volume_retrieve_block_number);

  } while (false);
  
  return rtCode;
}

void *hireManager::thread_hirefs_websocket_handle_worker(void *param)
{
  hireManager *_this = (hireManager *) param;
  _this->hirefs_websocket_worker();
  return NULL;
}

void *hireManager::thread_hirefs_enquire_handle_worker(void *param)
{
  hireManager *_this = (hireManager *) param;
  _this->hirefs_enquire_worker();
  return NULL;
}

void *hireManager::thread_hirefs_commit_poc_handle_worker(void *param)
{
  hireManager *_this = (hireManager *) param;
  _this->hirefs_commit_poc_worker();
  return NULL;
}

void *hireManager::thread_hirefs_getsysteminfo_handle_worker(void *param)
{
  hireManager *_this = (hireManager *) param;
  _this->hirefs_getsysteminfo_worker();
  return NULL;
}

void *hireManager::thread_hirefs_rent_volume_partitioning_handle_worker(void *param)
{
  hireManager *_this = (hireManager *) param;
  _this->hirefs_rent_volume_partitioning_worker();
  return NULL;
}

void hireManager::hirefs_websocket_worker()
{
  _websocket_server.run(hireManager::websocket_connect_handle, hireManager::websocket_message_handle, this);
}

void hireManager::hirefs_enquire_worker()
{
  std::string do_enquire_message = "";
  time_t ts_enquire_pledge = 0, ts_enquire_block_number = 0;
  u_int64_t last_volume_package_poc_commit_block_number = 0;
  HIRE_POC_POST_DATA &hire_poc_post_data = hireLogManager::instance().get_hire_poc_post_data();
  hire_poc_post_data.hire_poc_commit_progress = HIRE_LOG_POC_COMMIT_PROGRESS_PLANING;

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hireManager enquire worker begin.");

  _load_poc_commit_history(HIREFS_VOLUME_PACKAGE_POC_COMMIT_HISTORY, last_volume_package_poc_commit_block_number);

  while (!_stop_flag) {
    do_enquire_message = _hirefs_enquire_msg_queue.get_from_queue();
    if (!do_enquire_message.empty()) {

    } else {
      int event_rtCode = _hirefs_enquire_event.event_timewait(HIRE_ENQUIRE_CHECK_PERIOD);
      if (ETIMEDOUT == event_rtCode) {

        _check_hire_status();

        //enquire pledge
        if (HIRE_ENQUIRE_PLEDGE_PERIOD <= auxHelper::get_time_diff(ts_enquire_pledge)) {
          ts_enquire_pledge = time(NULL);

          if (HIREST_VOLUME_UNPACKAGE == _hire_volume.get_hire_status() || HIREST_VOLUME_PACKAGING == _hire_volume.get_hire_status()) {
            bool bhire_onpledge = false;
            if (bhire_onpledge != _bhire_onpledge) {
              _bhire_onpledge = bhire_onpledge;
              _sava_private_key();
            }
          } else {
            _enquire_pledge();
          }
        }

        //enquire block number
        if (ts_enquire_block_number <= time(NULL)) {
          ts_enquire_block_number = time(NULL);

          if (_bhire_onpledge) {
            u_int64_t hire_block_number = _enquire_lasted_block_number();
            if (0 < hire_block_number && hire_block_number > _hire_last_block_number) {
              //
              u_int today_pass_block_number = hire_block_number % _hire_one_day_block_number;
              u_int today_block_number_begin = hire_block_number - today_pass_block_number;
              u_int today_block_number_end = today_block_number_begin + _hire_one_day_block_number;
              u_int today_rest_block_number = _hire_one_day_block_number - today_pass_block_number;

              //next enquire block number time
              ts_enquire_block_number = time(NULL) + (today_rest_block_number * 1.08 * HIRE_ONE_BLOCK_NUMBER_TIME);
              debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "I will enquire block number  after : %s.", auxHelper::get_time_str(ts_enquire_block_number).c_str());
              
              //
              if (last_volume_package_poc_commit_block_number >= today_block_number_end) 
                last_volume_package_poc_commit_block_number = 0;
              
              if (last_volume_package_poc_commit_block_number < today_block_number_begin) {
                int rand_num_begin = _hire_one_day_block_number * 0.01;
                int rand_num_end = today_rest_block_number / 2;
                if (rand_num_begin == rand_num_end) rand_num_end += 1;
                int rand_num = auxHelper::get_rand_num(rand_num_begin, rand_num_end);
                u_int random_duration =  rand_num * HIRE_ONE_BLOCK_NUMBER_TIME;
                _ts_volume_package_poc_commit = time(NULL) + random_duration;
                debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "I will commit volume package poc after : %s.", auxHelper::get_time_str(_ts_volume_package_poc_commit).c_str());
                
                //
                hire_poc_post_data.hire_chain_day = hire_block_number / _hire_one_day_block_number;
                hire_poc_post_data.hire_poc_commit_progress = HIRE_LOG_POC_COMMIT_PROGRESS_PLANED;
                hire_poc_post_data.hire_poc_commit_retry = false;
                hire_poc_post_data.hire_poc_commit_error = "";
                hire_poc_post_data.hire_poc_commit_plan_ts = _ts_volume_package_poc_commit;
                hire_poc_post_data.hire_poc_commit_plan_block = today_pass_block_number + rand_num;
                hire_poc_post_data.hire_poc_post_details_list.clear();
              }
            }
          }
        }
      }
    }
  }

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hireManager enquire worker end.");
}

void hireManager::hirefs_commit_poc_worker()
{
  std::map<std::string, std::string> hire_poc_collection_post_results;
  std::vector<std::string> hire_poc_group_name_of_poc_collection_post_fail;
  std::string do_commit_message = "";
  u_char check_times = 0;
  HIRE_POC_POST_DATA &hire_poc_post_data = hireLogManager::instance().get_hire_poc_post_data();

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hireManager commit worker begin.");

  while (!_stop_flag) {
    do_commit_message = _hirefs_commit_msg_queue.get_from_queue();
    if (!do_commit_message.empty()) {

      if (HIRE_POST_POC_TYPE_VOLUME_PACKAGE == do_commit_message)
        _do_post_volume_package_poc_to_hirechain();
      else if (HIRE_POST_POC_TYPE_RENT == do_commit_message) {
        _do_post_poc_collection_to_hirechain_every_day(hire_poc_collection_post_results);
        hire_poc_collection_post_results.clear();
      } else if (HIRE_POST_POC_TYPE_RENT_RETRIEVE == do_commit_message)
        _do_volume_rent_retrieve_poc_to_hirechain();
    } else {
      int event_rtCode = _hirefs_commit_event.event_timewait(HIRE_COMMIT_CHECK_PERIOD);
      if (ETIMEDOUT == event_rtCode) {
        do
        {
          if (!_bhire_onpledge || 0 == _ts_volume_package_poc_commit || _ts_volume_package_poc_commit > time(NULL)) break;

          if (hire_poc_collection_post_results.empty()) {
            // 
            debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "commit poc collection every day...");
            if (_do_post_poc_collection_to_hirechain_every_day(hire_poc_collection_post_results)) { 
              //
              _ts_volume_package_poc_commit = time(NULL) + (_hire_one_day_block_number * 0.008 * HIRE_COMMIT_CHECK_PERIOD);

              //
              hire_poc_post_data.hire_poc_commit_progress = HIRE_LOG_POC_COMMIT_PROGRESS_COMMITED;
              hire_poc_post_data.hire_poc_commit_plan_ts = _ts_volume_package_poc_commit;
              hire_poc_post_data.hire_poc_commit_plan_block += (HIRE_POC_POST_RESULT_CHECK_TIMES + _hire_one_day_block_number * 0.008);
            } else {
              //
               _ts_volume_package_poc_commit = time(NULL) + (_hire_one_day_block_number * 0.008 * HIRE_COMMIT_CHECK_PERIOD);

              //
              hire_poc_post_data.hire_poc_commit_progress = HIRE_LOG_POC_COMMIT_PROGRESS_PLANED;
              hire_poc_post_data.hire_poc_commit_retry = true;
              hire_poc_post_data.hire_poc_commit_plan_ts = _ts_volume_package_poc_commit;
              hire_poc_post_data.hire_poc_commit_plan_block += _hire_one_day_block_number * 0.008;
            }
          } else {
            debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "check poc collection status : [%u times]...", check_times++);
            if (_check_poc_collection_post_result_from_hirechain(hire_poc_collection_post_results, hire_poc_group_name_of_poc_collection_post_fail)) {
              //
              check_times = 0;
              _ts_volume_package_poc_commit = 0;
              hire_poc_collection_post_results.clear();

              //
              hire_poc_post_data.hire_poc_commit_progress = HIRE_LOG_POC_COMMIT_PROGRESS_CONFIRMED;
              hire_poc_post_data.hire_poc_commit_retry = false;
              hire_poc_post_data.hire_poc_commit_error = "";
            } else {
              //
              if (HIRE_POC_POST_RESULT_CHECK_TIMES <= check_times ) {
                check_times = 0;

                //
                for (size_t i = 0; i < hire_poc_group_name_of_poc_collection_post_fail.size(); i++) {
                  std::string hire_poc_collection_post_result;
                  if (_do_post_poc_collection_to_hirechain_by_hire_poc_group_name(hire_poc_group_name_of_poc_collection_post_fail[i], hire_poc_collection_post_result)) {
                    std::map<std::string, std::string>::iterator it;
                    it = hire_poc_collection_post_results.find(hire_poc_group_name_of_poc_collection_post_fail[i]);
                    if (it != hire_poc_collection_post_results.end()) hire_poc_collection_post_results.erase(it);
                    hire_poc_collection_post_results.insert(std::make_pair(hire_poc_group_name_of_poc_collection_post_fail[i], hire_poc_collection_post_result));
                  }
                }

              }
            }
          }

        } while (false);
         
      }
    }
  }
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hireManager commit worker end.");
}

void hireManager::hirefs_getsysteminfo_worker()
{
  CPU_OCCUPY o, n;

  while (!_stop_flag) {
    //CPU
    _get_cpuoccupy(&o);
    auxHelper::sleep_time_with_cancle(HIRE_SYSTEMINFO_CHECK_PERIOD, &_stop_flag);
    _get_cpuoccupy(&n);
    _cpu_used = _cal_cpuoccupy(&o, &n);
 
    //MEM
    _mem_used = _get_mem_used();

    _load_volume_bytes();
  }
}

void hireManager::hirefs_rent_volume_partitioning_worker()
{
  std::string do_rent_volume_partitioning_message = "";
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hireManager rent volume partitioning worker begin.");

  while (!_stop_flag) {
    do_rent_volume_partitioning_message = _hirefs_rent_volume_partitioning_queue.get_from_queue();
    if (!do_rent_volume_partitioning_message.empty()) {
      _do_rent_volume_process(do_rent_volume_partitioning_message);
    } else {
      int event_rtCode = _hirefs_rent_volume_partitioning_event.event_timewait(HIRE_ENQUIRE_CHECK_PERIOD);
      if (ETIMEDOUT == event_rtCode) {

      }
    }
  }

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hireManager rent volume partitioning worker end.");
}

void hireManager::_notify_rent_volume_partitioning(HIRE_RENT_ITEM &hire_rent_item)
{
  if (hire_rent_item.hire_rent_addr.empty() || hire_rent_item.hire_rent_voucher.empty() || 0 == hire_rent_item.hire_rent_time) return;

  FJson::Value root;
  root["cmd"] = RENT_VOMULE_PARTITIONING;
  root["utg_rent_voucher"] = hire_rent_item.hire_rent_voucher;
  root["utg_rent_addr"] = hire_rent_item.hire_rent_addr;
  root["utg_rent_index"] = hire_rent_item.hire_rent_index;
  root["utg_block_number"] = hire_rent_item.hire_block_number;
  root["utg_rent_volume"] = hire_rent_item.hire_rent_volume;
  root["utg_rent_time"] = hire_rent_item.hire_rent_time;
  
  bool bempty = false;
  _hirefs_rent_volume_partitioning_queue.add_to_queue(root.toCompatString(), bempty);
  _hirefs_rent_volume_partitioning_event.event_notify();

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "rent volume partitioning notify : %s", root.toCompatString().c_str());
}

void hireManager::_notify_rent_volume_partitioning_retrieve(const std::string &hire_rent_voucher)
{
  FJson::Value root;

  root["cmd"] = RENT_VOMULE_PARTITIONING_RETRIEVE;
  root["utg_rent_voucher"] = hire_rent_voucher;
  
  bool bempty = false;
  _hirefs_rent_volume_partitioning_queue.add_to_queue(root.toCompatString(), bempty);
  _hirefs_rent_volume_partitioning_event.event_notify();

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "rent volume partitioning retrieve notify : %s", root.toCompatString().c_str());
}

void hireManager::_notify_rent_volume_partitioning_retrieve_all()
{
  FJson::Value root, hire_rent_items;

  _hire_volume.get_all_rent_items(hire_rent_items);
  if (0 < hire_rent_items.size()) root["utg_rent_items"] = hire_rent_items;
  root["cmd"] = RENT_VOMULE_PARTITIONING_RETRIEVE_ALL;

  bool bempty = false;
  _hirefs_rent_volume_partitioning_queue.add_to_queue(root.toCompatString(), bempty);
  _hirefs_rent_volume_partitioning_event.event_notify();

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "rent volume partitioning retrieve notify : %s", root.toCompatString().c_str());
}

void hireManager::_do_rent_volume_process(const std::string &hire_rent_message)
{
  /*
  {
    "cmd" : RENT_VOMULE_PARTITIONING,
    "utg_rent_voucher":"ux56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
    "utg_rent_addr":"ux56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
    "utg_rent_index":"536870912-805306367",
    "utg_block_number": "12324",
    "utg_rent_volume": "5368709120",
    "utg_rent_time": 30
  }
  */
  FJson::Parser parser;
  FJson::Value root;

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "rent volume partitioning process : %s", hire_rent_message.c_str());

  if (!parser.load_string(hire_rent_message, root)) return;

  switch (root["cmd"].asUInt()) {
  case RENT_VOMULE_PARTITIONING:
    _do_rent_volume_partitioning(root);
    break;
  case RENT_VOMULE_PARTITIONING_RETRIEVE:
    _do_rent_volume_partitioning_retrieve(root);
    break;
  case RENT_VOMULE_PARTITIONING_RETRIEVE_ALL:
    _do_rent_volume_partitioning_retrieve_all(root);
    break;
  default:
    break;
  }
}

void hireManager::_do_rent_volume_partitioning(FJson::Value &root)
{
  std::string hire_rent_voucher = root["utg_rent_voucher"].asString();
  std::string hire_rent_addr = root["utg_rent_addr"].asString();

  std::string hire_rent_volume = root["utg_rent_volume"].asString();
  u_int64_t hire_rent_volume_bytes = strtoll(hire_rent_volume.c_str(), NULL, 10);

  std::string hire_block_number = root["utg_block_number"].asString();
  u_int64_t hire_block_number_number = strtoll(hire_block_number.c_str(), NULL, 10);
  u_int64_t hire_rent_time = root["utg_rent_time"].asUInt();
  u_int64_t hire_rent_retrieve_time = time(NULL) + (hire_rent_time * _hire_one_day_block_number * HIRE_ONE_BLOCK_NUMBER_TIME);
  
  std::string hire_rent_index = root["utg_rent_index"].asString();
  _hire_volume.do_volume_rent_volume_partitioning(hire_rent_index);

  if (_notify_filesystem_do_rent_volume_partitioning(hire_rent_voucher, hire_rent_addr, hire_rent_volume_bytes, hire_rent_time, hire_rent_retrieve_time)) {
    _hire_volume.update_rent_item_volume_partitioning(hire_rent_voucher);
    char szbuffer[128] = {0};
    sprintf(szbuffer, "%llu", _hire_last_block_number);
    hireLogManager::instance().add_volume_rent_log(HIRE_LOG_ACT_RENT_VOLUME_PARTITIONING, hire_rent_voucher, std::string(szbuffer), auxHelper::intBytesToGBStr(hire_rent_volume_bytes), auxHelper::intBytesToGBStr(_hire_volume.get_hire_package_volume_free()));
  }
}

void hireManager::_do_rent_volume_partitioning_retrieve(FJson::Value &root)
{
  std::string hire_rent_voucher = root["utg_rent_voucher"].asString();
  if (_notify_filesystem_do_rent_volume_partitioning_retrieve(hire_rent_voucher)) {
    char szbuffer[128] = {0};
    sprintf(szbuffer, "%llu", _hire_last_block_number);
    hireLogManager::instance().add_volume_rent_log(HIRE_LOG_ACT_RENT_VOLUME_PARTITIONING_RETRIEVE, hire_rent_voucher, std::string(szbuffer), std::string(""), auxHelper::intBytesToGBStr(_hire_volume.get_hire_package_volume_free()));
  }
}

void hireManager::_do_rent_volume_partitioning_retrieve_all(FJson::Value &root)
{
  FJson::Value hire_rent_items;

  hire_rent_items = root["utg_rent_items"];
  if (hire_rent_items.isArray() && 0 < hire_rent_items.size()) {
    for (size_t i = 0; i < hire_rent_items.size(); i++) {
      std::string hire_rent_voucher = hire_rent_items[i]["utg_rent_voucher"].asString();
      if (_notify_filesystem_do_rent_volume_partitioning_retrieve(hire_rent_voucher)) {
        char szbuffer[128] = {0};
        sprintf(szbuffer, "%llu", _hire_last_block_number);
        hireLogManager::instance().add_volume_rent_log(HIRE_LOG_ACT_RENT_VOLUME_PARTITIONING_RETRIEVE, hire_rent_voucher, std::string(szbuffer), std::string(""), auxHelper::intBytesToGBStr(_hire_volume.get_hire_package_volume_free()));
      }
    }
  }
}

bool hireManager::_notify_filesystem_do_rent_volume_partitioning(const std::string &hire_rent_voucher, const std::string &hire_rent_addr, u_int64_t hire_rent_volume, u_int64_t utg_rent_time, u_int64_t rent_retrieve_time)
{
  /*
  input : 
  {
    "rent_voucher":"ux56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",    
    "rent_addr":"ux56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",       
    "rent_volume": 5368709120,   
    "rent_time":30, 
    "rent_retrieve_time":166857635112  
  }
  output:
  {
    "resualt": xxx
  }
  */
  bool rtCode = false;
  FJson::Value request_js, response_js;
  FJson::Parser parser;
  std::string request_message = "", response_message = "";

  do {
    request_js["rent_voucher"] = hire_rent_voucher;
    request_js["rent_addr"] = hire_rent_addr;
    request_js["rent_volume"] = hire_rent_volume / (1024 * 1024 *1024);
    request_js["rent_time"] = utg_rent_time;
    request_js["rent_retrieve_time"] = rent_retrieve_time;
    
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "POST http://127.0.0.1:8080/v1/utg/rent  request : %s", request_js.toCompatString().c_str());

    response_message = _httpsession_post_msg("http://127.0.0.1:8080/v1/utg/rent", request_js.toCompatString());
    if (response_message.empty()) break;
    
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "POST http://127.0.0.1:8080/v1/utg/rent  response : %s", response_message.c_str());

    if (!parser.load_string(response_message, response_js)) break;

    if (0 == response_js["result"].asUInt()) rtCode = true;

  } while (false);
  
  return rtCode;
}

bool hireManager::_notify_filesystem_do_rent_volume_partitioning_retrieve(const std::string &hire_rent_voucher)
{
  /*
  input : 
  {
    "rent_voucher":"ux56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421"
  }
  output:
  {
    "resualt": xxx
  }
  */
  bool rtCode = false;
  FJson::Value request_js, response_js;
  FJson::Parser parser;
  std::string request_message = "", response_message = "";

  do {
    request_js["rent_voucher"] = hire_rent_voucher;
    
     debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "POST http://127.0.0.1:8080/v1/utg/free  request : %s", request_js.toCompatString().c_str());

    response_message = _httpsession_post_msg("http://127.0.0.1:8080/v1/utg/free", request_js.toCompatString());
    if (response_message.empty()) break;
    
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "POST http://127.0.0.1:8080/v1/utg/free  response : %s", response_message.c_str());

    if (!parser.load_string(response_message, response_js)) break;

    if (0 == response_js["result"].asUInt()) rtCode = true;

  } while (false);
  
  return rtCode;
}

bool hireManager::websocket_connect_handle(const std::string &ip, u_short port, const std::string &request_uri, void *parmame)
{
  hireManager *_this = (hireManager *) parmame;
  _this->process_websocket_connect(ip, port, request_uri);
  return true;
}

bool hireManager::websocket_message_handle(const std::string &ip, u_short port, const std::string &message, std::string &ret_message, void *parmame)
{
  hireManager *_this = (hireManager *) parmame;
  return _this->process_websocket_message(ip, port, message, ret_message);
}

void hireManager::process_websocket_connect(const std::string &ip, u_short port, const std::string &request_uri)
{
  std::pair<std::map<std::string, ws_session>::iterator, bool> ret;
  std::map<std::string, ws_session>::iterator it_cache;

  std::string ip_and_port = ip + ":" + std::to_string(ntohs(port));
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "ws connection from: %s", ip_and_port.c_str());
  debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "request_uri: %s", request_uri.c_str());

  ws_session ws_session_item;
  ws_session_item.ip = ip;
  ws_session_item.request_uri = request_uri;
  
  lock_ws_session();
  ret = _ws_sessions.insert(std::make_pair(ip, ws_session_item));
  if (false == ret.second) {
    it_cache = ret.first;
    it_cache->second.request_uri = request_uri;
  }
  unlock_ws_session();
}

bool hireManager::process_websocket_message(const std::string &ip, u_short port, const std::string &message, std::string &ret_message)
{
  bool rtCode = false;
  std::string ip_and_port = ip + ":" + std::to_string(ntohs(port));
  debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "ws messsage from: %s", ip_and_port.c_str());

  std::map<std::string, ws_session>::iterator it;

  lock_ws_session();
  it = _ws_sessions.find(ip);
  if (it == _ws_sessions.end()) { 
    debugEntry(LL_WARN, LOG_MODULE_INDEX_HIRE, "ws session expired : %s", ip_and_port.c_str());
    return false;
  }
  unlock_ws_session();

  if ("/utglogin" == it->second.request_uri) {
    rtCode = _process_websocket_login(it->second, message, ret_message);
  } else if ("/utgdevicectl" == it->second.request_uri) {
    rtCode = _process_websocket_device_contrl(it->second, message, ret_message);
  } else if ("/utgresetpasswd" == it->second.request_uri) {
    rtCode = _process_reset_adminpasswd(it->second, message, ret_message);
  } else {
    debugEntry(LL_WARN, LOG_MODULE_INDEX_HIRE, "ws recv unknow request uri %s from %s", 
              it->second.request_uri.c_str(), ip_and_port.c_str());
    return false;
  }

  return rtCode;
}

bool hireManager::_process_websocket_login(ws_session &session, const std::string &message, std::string &ret_message)
{
  /*
  input:
  {
    "enc_ni":"704872bd6735309b83b2a87041db4cd1",    
    "ni":"xxxxx"                                    
  }
  output:
  {
      "result" xxx,          
      "enc_ticket":"xxxxx"   
  }
  */
  CN_ERR rtCode = CE_SUCC;
  FJson::Parser parser;
  FJson::Value root;
  std::string ni = "", enc_ni = "", my_enc_ni = "", str_temp = "", session_ticket = "", en_session_ticket = "";
  u_char keyBuf[16] = {0};
  u_int keyBuf_len = 16;

  debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "receive websocket login message :\n%s", message.c_str());

  do
  {
    if (!parser.load_string(message, root)) { 
      rtCode = CE_PROTOCOL;
      break;
    }

    ni = root["ni"].asString();
    enc_ni = root["enc_ni"].asString();

    auxHelper::hexstring2Byte(_hire_admin_passwd_md5, keyBuf, keyBuf_len);

    str_temp = ni;
    RC4((u_char *)str_temp.c_str(), str_temp.length(), keyBuf, 16);

    my_enc_ni = hireVolume::getMD5(auxHelper::byte2hexstring((const u_char *)str_temp.c_str(), str_temp.length()));

    debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "my_enc_ni :%s", my_enc_ni.c_str());

    if (my_enc_ni != enc_ni) {
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "invalue password");
      rtCode = CE_INVLIAD_PASS;
      break;
    }
    
    session_ticket = hireVolume::getMD5(my_enc_ni);
    //update ticket
    session.ticket = session_ticket;
    session.ts = time(NULL);
    
    str_temp = session_ticket;
    RC4((u_char *)str_temp.c_str(), str_temp.length(), keyBuf, 16);

    en_session_ticket = auxHelper::byte2hexstring((const u_char *)str_temp.c_str(), str_temp.size());
  
    rtCode = CE_SUCC;

    hireLogManager::instance().add_admin_login_log(HIRE_LOG_ACT_USERLOGIN, session.ip);

  } while (false);
  
  root.clear();
  root["result"] = rtCode;
  if (CE_SUCC == rtCode) {
    root["enc_ticket"] = en_session_ticket;
    root["ticket"] = session_ticket;
  }

  ret_message = root.toCompatString();

  debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "return websocket login message:\n%s", ret_message.c_str());

  return true;
}

bool hireManager::_process_reset_adminpasswd(ws_session &session, const std::string &message, std::string &ret_message)
{
  /*
  input:
  {
    "ni":"xxxxx"                  
    "sin":"xxxxx",                
  }
  output:
  {
      "result" xxx,               
  }
  */
  CN_ERR rtCode = CE_SUCC;
  FJson::Parser parser;
  FJson::Value root;
  u_char keyBuf[16] = {0};
  u_int keyBuf_len = 16;

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "receive websocket resetpasswd message :\n%s", message.c_str());

  do
  {
    std::string ni = "", sin = "";
    if (!parser.load_string(message, root)) { 
      rtCode = CE_PROTOCOL;
      break;
    }

    if (std::string(HIRE_LOGIN_PASSWD_MD5) == _hire_admin_passwd_md5) {
      rtCode = CE_SUCC;
      break;
    }

    ni = root["ni"].asString();
    sin = root["sin"].asString();
    bool sin_check =  blockChainTools::bct_signature_verify((const u_char *)ni.c_str(), sin, _hire_volume.get_hire_address());
    if (!sin_check) { 
      rtCode = CE_INVALID_SIGNATURE;
      break;
    }

    _hire_admin_passwd_md5 = HIRE_LOGIN_PASSWD_MD5;
    if (0 == access(HIREFS_ADMIN_PASSWD_FILE_NAME, R_OK))
      unlink(HIREFS_ADMIN_PASSWD_FILE_NAME);
    
    hireLogManager::instance().add_admin_login_log(HIRE_LOG_ACT_RESETPWD, session.ip);

    rtCode = CE_SUCC;

  } while (false);
  
  root.clear();
  root["result"] = rtCode;

  ret_message = root.toCompatString();

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "return websocket login message:\n%s", ret_message.c_str());

  return true;
}

bool hireManager::_process_websocket_device_contrl(ws_session &session, const std::string &message, std::string &ret_message)
{
  FJson::Parser parser;
  FJson::Value root;
  HIRE_COMMAND cmd = HIRECMD_NONE;

  do
  {
    std::string ni = "", sin = "", my_sin = "";
    u_char buffer_sign[32] = {0};

    if (!parser.load_string(message, root)) {
      root["result"] = CE_PROTOCOL;
      debugEntry(LL_WARN, LOG_MODULE_INDEX_HIRE, "error protocol : %s", message.c_str());
      break;
    }

    if (false == root["cmd"].isInt()) {
      debugEntry(LL_WARN, LOG_MODULE_INDEX_HIRE, "invalue parameter : %s", message.c_str());
      root["result"] = CE_INVALID_PARAMETER;
      break;
    }

    cmd = (HIRE_COMMAND)root["cmd"].asUInt();
    debugEntry(((HIRECMD_GET_HIRE_INFO == cmd) ? LL_VERBOSE : LL_DEBUG), LOG_MODULE_INDEX_HIRE, "receive websocket device control message :\n%s", message.c_str());

    ni = root["ni"].asString();
    sin = root["sin"].asString();

    hmac_sha256(session.ticket.c_str(), session.ticket.length(), ni.c_str(), ni.length(), buffer_sign, 32);
    my_sin = auxHelper::byte2hexstring(buffer_sign, 32);
    
    debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "my_sin : %s", my_sin.c_str());

    if (my_sin != sin) {
      debugEntry(LL_WARN, LOG_MODULE_INDEX_HIRE, "invalue signature");
      root["result"] = CE_INVALID_SIGNATURE;
      break;
    }

    switch (cmd) {
      case HIRECMD_GET_HIRE_INFO:
        _process_message_get_hire_info(root);
        break;
      case HIRECMD_SET_HIRE_INFO:
        _process_message_set_hire_info(session, root);
        break;
      case HIRECMD_VOLUME_PACKAGE:
        _process_message_volume_package(root);
        break;
      case HIRECMD_GET_VOLUME_PACKAGE_POC:
        _process_message_get_volume_package_poc(root);
        break;
      case HIRECMD_VOLUME_RENT:
        _process_message_volume_rent(root);
        break;
      case HIRECMD_GET_VOLUME_RENT_POC:
        _process_message_get_volume_rent_poc(root);
        break;
      case HIRECMD_VOLUME_RETRIEVE:
        _process_message_volume_retrieve(root);
        break;
      case HIRECMD_VOLUME_RESET:
        _process_message_volume_reset(root);
        break;
      case HIRECMD_GET_SYSTEMINFO:
        _process_message_get_systeminfo(root);
        break;
      case HIRECMD_GET_HIRE_LOG:
        _process_message_get_hire_log(root);
        break;
      case HIRECMD_DO_POST_POC:
        _process_message_do_poc_post(root);
        break;
      case HIRECMD_CHECK_RPC_URL:
        _process_message_check_rpc_url(root);
        break;
      case HIRECMD_SET_ADMIN_PASSWD:
        _process_message_set_adminpasswd(session, root);
        break;        
      default:
        root["result"] = CE_PROTOCOL;
        break;
    }

  } while (false);
  
  ret_message = root.toCompatString();
  
  debugEntry(((HIRECMD_GET_HIRE_INFO == cmd) ? LL_VERBOSE : LL_DEBUG), LOG_MODULE_INDEX_HIRE, "return websocket message:\n%s", ret_message.c_str());

  return true;
}

void hireManager::_process_message_get_hire_info(FJson::Value& root)
{
  /*
  input:
  {
    "cmd" : HIRECMD_GET_HIRE_INFO,
    "ni":"xxxxx"                
    "sin":"xxxxx",              //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
    "cmd": HIRECMD_GET_HIRE_INFO,
    "utg_addr": "xxx",                
    "utg_prikey_exsit": true | false,  
    "utg_status": xxx,                
    "rate":{                          
        "completion": 20,             
        "remaining_time":12312        
      },
    "utg_volume": "20",               
    "utg_volume_free": "5",           
    "utg_rent_items":[                
        {
          "utg_block_number":"12324",              
          "utg_rent_index":"536870912-805306367",   
          "utg_rent_onpledge":true,                 
          "utg_rent_volume":"5368709120",           
          "utg_rent_voucher":"xxxxxx",              
          "utg_need_retrieve_by_manual" false       
        },
        {
          "utg_block_number":"3214",
          "utg_rent_index":"268435456-536870911",
          "utg_rent_onpledge":false,
          "utg_rent_volume":"5368709120",
          "utg_rent_voucher":"",
          "utg_need_retrieve_by_manual" false 
        }
  	],
    "utg_config" : {
      "chain_id": xxx,
      "utg_retrieve_block_number" : xxx,
      "utg_one_day_block_number"  : xxxx,
      "utg_enquire_pledge_url" : "xxx",
      "utg_chain_urls" : [
        "https://rpc.ultronglow.io",
        "https://rpc.ultronglow.io"
      ]
    },
    "volume": "50",                   
    "volume_total": "50",             
    "version" : "xxxx",               
    "result": CE_SUCC                 
  }
  */
  HIRE_STATUS hire_status = _hire_volume.get_hire_status();

  root.clear();
  root["cmd"] = HIRECMD_GET_HIRE_INFO;
  root["version"] = std::string(HIRE_PRODUCT_VERSION);
  root["utg_addr"] = _hire_volume.get_hire_address();
  root["utg_prikey_exsit"] = !_hire_private_key.empty();
  root["utg_status"] = hire_status;

  if (HIREST_VOLUME_PACKAGING == hire_status) {
    if (0 < _hire_volume.get_hire_package_volume_expect()) {
      u_int completion = 100 * ((_hire_volume.get_hire_package_volume_actual() * 1.0) / (_hire_volume.get_hire_package_volume_expect() * 1.0));
      root["rate"]["completion"] = completion;
      root["rate"]["remaining_time"] = _hire_volume.get_remaining_time();
    }
  }

  if (hireVolume::is_volume_packaged(hire_status) || HIREST_VOLUME_PACKAGING == hire_status || HIREST_VOLUME_OUTPLEDGE == hire_status) {
    root["utg_volume"] = auxHelper::intBytesToGBStr(_hire_volume.get_hire_package_volume_expect());
    root["utg_volume_free"] = auxHelper::intBytesToGBStr(_hire_volume.get_hire_package_volume_free());

    FJson::Value hire_rent_items;
    _hire_volume.get_all_rent_items(hire_rent_items, true);
    if (0 < hire_rent_items.size()) root["utg_rent_items"] = hire_rent_items;
  }
  
  root["volume"] = auxHelper::intBytesToGBStr(_volume_free);
  root["volume_total"] = auxHelper::intBytesToGBStr(_volume_total);

  FJson::Value hire_config;
  _get_hire_config(hire_config);
  root["utg_config"] = hire_config;

  root["result"] = CE_SUCC;
}

void hireManager::_process_message_set_hire_info(ws_session &session, FJson::Value& root)
{
  /*
  input:
  {
    "cmd": HIRECMD_SET_HIRE_INFO,
    "utg_addr": "xxx",          
    "enc_utg_prikey": "xxx",    
    "utg_prikey_md5": "xxx",    
    "utg_config" : {
      "chain_id": xxx,
      "utg_retrieve_block_number" : xxx,
      "utg_one_day_block_number"  : xxxx,
      "utg_enquire_pledge_url" : "xxx",
      "utg_chain_urls" : [
        "https://rpc.ultronglow.io",
        "https://rpc.ultronglow.io"
      ]
    },
    "ni":"xxxxx",               
    "sin":"xxxxx",              //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
      "cmd": HIRECMD_SET_HIRE_INFO,
      "result": CE_SUCC                    
  }
  */
  CN_ERR rtCode = CE_SUCC;
  std::string hire_addr = "", enc_hire_prikey = "", hire_prikey_md5 = "";
  u_char keyBuf[16] = {0}, enc_data[256] = {0};
  u_int enc_data_len = 256;
  bool need_update_address = false;
  
  do
  {
    if (root.isMember("utg_addr") && root["utg_addr"].isString()) {
      hire_addr = root["utg_addr"].asString();
      std::transform(hire_addr.begin(), hire_addr.end(), hire_addr.begin(), ::tolower);
      need_update_address = true;
    }

    if (root.isMember("enc_utg_prikey")) {
      if (!root["enc_utg_prikey"].isString()) {
        rtCode = CE_PROTOCOL;
        break;
      }

      enc_hire_prikey = root["enc_utg_prikey"].asString();
      if (!enc_hire_prikey.empty()) {
        if (!root["utg_prikey_md5"].isString()) {
          rtCode = CE_PROTOCOL;
          break;
        }
        hire_prikey_md5 = root["utg_prikey_md5"].asString();

        auxHelper::hexstring2Byte(enc_hire_prikey, enc_data, enc_data_len);

        hireVolume::getMD5(session.ticket, keyBuf);
        RC4((u_char *)enc_data, enc_data_len, keyBuf, 16);

        std::string hire_private_key = std::string((char *)enc_data);
        if (hire_prikey_md5 != hireVolume::getMD5(hire_private_key)) {
          rtCode = CE_INVALID_PARAMETER;
          break;
        }

        _hire_private_key = hire_private_key;

      } else _hire_private_key = "";
      
      need_update_address = true;
      //debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "hire private key : %s", _hire_private_key.c_str());
    }

    if (need_update_address) {
      std::string hire_address;
      if (!_hire_private_key.empty()) {
        hire_address = blockChainTools::bct_der_prikey_to_address(_hire_private_key);
      } else hire_address = hire_addr;

      if (!hire_address.empty()) {
        _hire_volume.set_hire_address(hire_address);
        _hire_volume.set_volume_package_filename_prefix(std::string("utg_") + hire_address);
      }

      _sava_private_key();
    }

    if (root.isMember("utg_config")) {
      FJson::Value hire_config = root["utg_config"];

      if (hire_config.isNull() || !set_hirefs_config(hire_config)) {
        rtCode = CE_INVALID_PARAMETER;
        break;
      }  
      
      _save_hire_config();
    }

  } while (false);
  
  root.clear();
  root["cmd"] = HIRECMD_SET_HIRE_INFO;
  root["result"] = rtCode;
}

void hireManager::_process_message_volume_package(FJson::Value& root)
{
  /*
  input:
  {
    "cmd": HIRECMD_VOLUME_PACKAGE,
    "volume": "10"               
    "utg_block": "xxx",          
    "utg_nonce" : "xxx,          
    "utg_block_hash": "xxx",     
    "ni":"xxxxx"                 
    "sin":"xxxxx",              
  }
  output:
  {
      "cmd": HIRECMD_VOLUME_PACKAGE,
      "result": CE_SUCC          
  }
  */
  CN_ERR rtCode = CE_SUCC;
  FJson::Value volume_message;
  HIRE_STATUS hire_status = _hire_volume.get_hire_status();

  do
  {
    if (HIREST_VOLUME_UNPACKAGE != hire_status) {
      rtCode = CE_STATUS;
      break;
    }

    if (!root["volume"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    std::string volume_GB_str = root["volume"].asString();
    u_int64_t volume_GB = atoll(volume_GB_str.c_str());
    if (0 != volume_GB % HIRE_VOLUME_MIN_GB) {
      rtCode = CE_INVALID_PARAMETER;
      break; 
    }
    volume_message["volume"] = volume_GB_str;

    if (!root["utg_block"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    volume_message["utg_block"] = root["utg_block"].asString();  

    if (!root["utg_nonce"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    volume_message["utg_nonce"] = root["utg_nonce"].asString();  

    if (!root["utg_block_hash"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    volume_message["utg_block_hash"] = root["utg_block_hash"].asString();

    if (_hire_volume.get_hire_address().empty()) {
      rtCode = CE_STATUS;
      break;
    }
    volume_message["utg_address"] = _hire_volume.get_hire_address();

    volume_message["cmd"] = HIRECMD_VOLUME_PACKAGE;

    _hire_volume.notify_hireVolume(volume_message.toCompatString());

    rtCode = CE_PENDING;

  } while (false);

  root.clear();
  root["cmd"] = HIRECMD_VOLUME_PACKAGE;
  root["result"] = rtCode;
}

void hireManager::_process_message_get_volume_package_poc(FJson::Value& root)
{
  /*
  input:
  {
      "cmd": HIRECMD_GET_VOLUME_PACKAGE_POC,
      "utg_block": "xxx",          
      "utg_nonce" : "xxx,          
      "utg_block_hash": "xxx",     
      "ni":"xxxxx"                
      "sin":"xxxxx",              //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
      "utg_package_poc": "xxx",    
      "utg_block": "xxx",          
      "utg_nonce" : "xxx,          
      "utg_block_hash": "xxx",     
      "utg_volume": "20",          
      "utg_volume_free": "15",     
      "cmd": HIRECMD_GET_VOLUME_PACKAGE_POC,
      "result": CE_SUCC           
  }
  */
  
  CN_ERR rtCode = CE_SUCC;
  FJson::Value volume_message;
  HIRE_STATUS hire_status = _hire_volume.get_hire_status();

  do
  {
    if (!hireVolume::is_volume_packaged(hire_status)) {
      rtCode = CE_STATUS;
      break;
    }

    if (!root["utg_block"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }

    if (!root["utg_nonce"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }

    if (!root["utg_block_hash"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    
    root["need_true"] = true;
    rtCode = _hire_volume.do_volume_package_poc(root);

  } while (false);

  root["cmd"] = HIRECMD_GET_VOLUME_PACKAGE_POC;
  root["result"] = rtCode;
}

void hireManager::_process_message_volume_rent(FJson::Value& root)
{
  /*
  input:
  {
    "cmd": HIRECMD_VOLUME_RENT,
    "utg_block": "xxx",          
    "utg_nonce" : "xxx,          
    "utg_block_hash": "xxx",     
    "volume": "5"             
    "voucher":"xxx",
    "rentAddr":"xxx",     
    "rentTime": 30,
    "ni":"xxxxx"                 
    "sin":"xxxxx",               //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
    "cmd": HIRECMD_VOLUME_RENT,
    "utg_rent_poc": "xxx",        
    "utg_package_poc": "xxx",    
    "utg_block": "xxx",          
    "utg_nonce" : "xxx,          
    "utg_block_hash": "xxx",     
    "utg_volume": "20",          
    "utg_volume_free": "15",     
    "result": CE_SUCC           
  }
  */
  std::string volume_GB_str = "", voucher = "", hire_block_number = "", rentAddr = "";
  CN_ERR rtCode = CE_SUCC;
  FJson::Value volume_message;
  u_int64_t rentTime = 0;

  do
  {
    if (!root["utg_block"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    hire_block_number = auxHelper::strToDecimalStr(root["utg_block"].asString());
    u_int64_t hire_block_number_int = strtoll(hire_block_number.c_str(), NULL, 10);
    u_int64_t abs_block_number = std::labs(hire_block_number_int - _hire_last_block_number);
    if (abs_block_number > _hire_one_day_block_number) {
      rtCode = CE_INVALID_PARAMETER;
      break;
    }

    if (!root["volume"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    volume_GB_str = root["volume"].asString();
    u_int64_t volume_GB = atoll(volume_GB_str.c_str());
    if (0 != volume_GB % HIRE_VOLUME_MIN_GB) {
      rtCode = CE_INVALID_PARAMETER;
      break; 
    }

    if (!root["voucher"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }  
    voucher = hireHelper::from0xToux(root["voucher"].asString());
    root["voucher"] = voucher;


    if (!root["rentAddr"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    rentAddr = root["rentAddr"].asString();

    if (!root["rentTime"].isInt()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    rentTime = root["rentTime"].asUInt();

    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume rent : %s %s GB at block number %s by %s(%lu days)", voucher.c_str(), volume_GB_str.c_str(), hire_block_number.c_str(), rentAddr.c_str(), rentTime);
    
    rtCode = _hire_volume.do_volume_rent(root);

  } while (false);
  
  root["cmd"] = HIRECMD_VOLUME_RENT;
  root["result"] = rtCode;
}

void hireManager::_process_message_get_volume_rent_poc(FJson::Value& root)
{
  /*
  input:
  {
    "cmd": HIRECMD_GET_VOLUME_RENT_POC,
    "voucher":"xxx",             
    "utg_block": "xxx",          
    "utg_nonce" : "xxx,          
    "utg_block_hash": "xxx",     
    "ni":"xxxxx"                 
    "sin":"xxxxx",               //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
    "utg_rent_poc": "xxx",             
    "cmd": HIRECMD_GET_VOLUME_RENT_POC,
    "result": CE_SUCC                  
  }
  */

  std::string voucher = "", hire_block = "";
  CN_ERR rtCode = CE_SUCC;
  FJson::Value volume_message;
  HIRE_STATUS hire_status = _hire_volume.get_hire_status();

  do
  {
    if (HIREST_VOLUME_IN_SERVICE != hire_status) {
      rtCode = CE_STATUS;
      break;
    }

    if (!root["utg_block"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    hire_block = root["utg_block"].asString();

    if (!root["utg_nonce"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }

    if (!root["utg_block_hash"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }

    if (!root["voucher"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    voucher = hireHelper::from0xToux(root["voucher"].asString());
    root["voucher"] = voucher;

    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "get volume rent poc : %s at block number : %s", voucher.c_str(), hire_block.c_str());

    rtCode = _hire_volume.do_volume_rent_poc(root);

  } while (false);
  
  root["cmd"] = HIRECMD_GET_VOLUME_RENT_POC;
  root["result"] = rtCode;
}

void hireManager::_process_message_volume_retrieve(FJson::Value& root)
{
  /*
  input:
  {
    "cmd": HIRECMD_VOLUME_RETRIEVE,
    "voucher":"xxx",             
    "utg_block": "xxx",        
    "ni":"xxxxx"                 
    "sin":"xxxxx",               //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
    "cmd": HIRECMD_VOLUME_RETRIEVE,
    "result": CE_SUCC            
  }
  */
  CN_ERR rtCode = CE_SUCC;
  std::string voucher = "", hire_block = "";
  HIRE_STATUS hire_status = _hire_volume.get_hire_status();

  do
  {
    if (!root["utg_block"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    hire_block = auxHelper::strToDecimalStr(root["utg_block"].asString());

    if (!root["voucher"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    voucher = hireHelper::from0xToux(root["voucher"].asString());
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "retrieve volume : %s at block number %s", voucher.c_str(), hire_block.c_str());

    rtCode = _hire_volume.do_volume_retrieve(voucher, hire_block);
    _notify_rent_volume_partitioning_retrieve(voucher);

  } while (false);
  
  root.clear();
  root["cmd"] = HIRECMD_VOLUME_RETRIEVE;
  root["result"] = rtCode;
}

void hireManager::_process_message_volume_reset(FJson::Value& root)
{
  /*
  input:
  {
    "cmd": HIRECMD_VOLUME_RESET,
    "ni":"xxxxx"                 
    "sin":"xxxxx",               //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
    "cmd": HIRECMD_VOLUME_RESET,
    "result": CE_SUCC            
  }
  */
  CN_ERR rtCode = CE_PENDING;

  if (!_bhire_onpledge) {
    _notify_rent_volume_partitioning_retrieve_all();
    _hire_volume.set_stop_volume_package_flag(true);
    _hire_volume.notify_hireVolume(root.toCompatString());
  } else rtCode = CE_STATUS;

  root.clear();
  root["cmd"] = HIRECMD_VOLUME_RESET;
  root["result"] = rtCode;
}

void hireManager::_process_message_get_systeminfo(FJson::Value& root)
{
  /*
  input:
  {
    "cmd": HIRECMD_GET_SYSTEMINFO,
    "ni":"xxxxx"                
    "sin":"xxxxx",               //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
    "cmd": HIRECMD_GET_SYSTEMINFO,
    "cpu": xxx,                   
    "mem": xxx,                   
    "volume": "50",               
    "volume_total": "50",         
    "result": CE_SUCC            
  }
  */

  root.clear();
  root["cmd"] = HIRECMD_GET_SYSTEMINFO;
  root["mem"] = _mem_used;
  root["cpu"] = _cpu_used;
  root["volume"] = auxHelper::intBytesToGBStr(_volume_free);
  root["volume_total"] = auxHelper::intBytesToGBStr(_volume_total);
  root["result"] = CE_SUCC;
}

void hireManager::_process_message_get_hire_log(FJson::Value& root)
{
  /*
  input:
  {
      "cmd": HIRECMD_GET_LOG,
      "log_type": xxx,             
      "ni":"xxxxx"                 
      "sin":"xxxxx",               //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
    "cmd": HIRECMD_GET_LOG,
    "logs" : [
      {
      "log_type" 1,                             
      "ts" : xx,                                
      "data" : {
        "action_type" :                       
        "src_ip" : "x.x.x.x",                 
      }
    },
      {
      "log_type" 2,                             
      "ts" : xx,                               
      "data" : {
          "action_type" : 1                     
        "utg_block": "xxx",                   
        "utg_volume": "10"                   
      }   
    },
    {
      "log_type" 4,                             
      "ts" : xx,                               
      "data" : {
        "action_type" : xx,                    
        "utg_voucher": "xxx",                  
        "utg_volume_rent": "xxx",              
        "utg_volume_free": "xxx", 	          
        "utg_block": "xxx"                     
      }
    },
    {
      "log_type": 65536,                         
      "ts": xx,                                 
      "data" : {
      "utg_chain_day": 147,                    
      "utg_poc_commit_progress":1,             
      "utg_poc_commit_retry" : true | false    
      "utg_poc_commit_error": "xxx",           
      "utg_poc_commit_plan_block": "xxx",      
      "utg_poc_commit_plan_ts": xxx,          
      "utg_poc_commit_details" [               
        {
            "utg_voucher":"xxxx",           
          "utg_poc_commit_result":"xxxx",  
          "utg_poc_commit_error": "xxx",   
          "utg_poc_commit_confirm":"xxx",  
          "utg_rpc_url" : "xxxx"，         
          "utg_block" : "xxxx"，           
          "utg_commit_ts" : xx             
        }
      ]
      }
    }
    ],
    "result": CE_SUCC             
  }
  */
  u_int log_type = HIRE_LT_NONE;

  if (root["log_type"].isInt())
    log_type = root["log_type"].asUInt();

  FJson::Value logs;
  hireLogManager::instance().get_logs(log_type, logs);

  root.clear();
  root["logs"] = logs;
  root["cmd"] = HIRECMD_GET_HIRE_LOG;
  root["result"] = CE_SUCC;
}

void hireManager::_process_message_do_poc_post(FJson::Value& root)
{
  /*
  input:
  {
    "cmd": HIRECMD_DO_POST_POC,
    "poc_type" : xxxx,           //HIRECMD_GET_VOLUME_PACKAGE_POC or HIRECMD_GET_VOLUME_RENT_POC
    "ni":"xxxxx"                 
    "sin":"xxxxx",               //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
    "cmd": HIRECMD_DO_POST_POC,
    "result": CE_SUCC             
  }
  */
  bool bempty = false;
  u_int poc_type = 0;
  CN_ERR rtCode = CE_SUCC;

  if (root["poc_type"].isInt())
    poc_type = root["poc_type"].asUInt();

  if (HIRECMD_GET_VOLUME_PACKAGE_POC == poc_type) {
    _hirefs_commit_msg_queue.add_to_queue(HIRE_POST_POC_TYPE_VOLUME_PACKAGE, bempty);
    _hirefs_commit_event.event_notify();
  } else if (HIRECMD_GET_VOLUME_RENT_POC == poc_type) {
    _hirefs_commit_msg_queue.add_to_queue(HIRE_POST_POC_TYPE_RENT, bempty);
    _hirefs_commit_event.event_notify();
  } else rtCode = CE_PROTOCOL;

  root.clear();
  root["cmd"] = HIRECMD_DO_POST_POC;
  root["result"] = rtCode;
}

void hireManager::_process_message_check_rpc_url(FJson::Value& root)
{
  /*
  input:
  {
  "cmd": HIRECMD_CHECK_RPC_URL,
  "utg_chain_urls" : [                    
    "https://rpc.ultronglow.io",  
    "https://rpc.ultronglow.io"
    ],
    "ni":"xxxxx"                          
    "sin":"xxxxx",                        //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
    "cmd": HIRECMD_CHECK_RPC_URL,
    "utg_chain_urls" : [                   
      {
        "check_result":true,
        "rpc_url":"http://192.168.9.203:8545"
      },
      {
        "check_result":true,
        "rpc_url":"http://192.168.9.202:8545"
      }
    ],
    "result": CE_SUCC
  }
  */
 CN_ERR rtCode = CE_SUCC;
  FJson::Value hire_chain_post_urls_js, hire_chain_urls_check_result_js;

  do
  {  
    if (!root.isMember("utg_chain_urls")) { 
      rtCode = CE_PROTOCOL;
      break;
    }
    hire_chain_post_urls_js = root["utg_chain_urls"];

    if (hire_chain_post_urls_js.isArray() && 0 < hire_chain_post_urls_js.size()) { 
      for (size_t i = 0; i < hire_chain_post_urls_js.size(); i++) {
        if (hire_chain_post_urls_js[i].isString()) {
          std::string hire_chain_post_url = hire_chain_post_urls_js[i].asString();
          debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "check chain id from : %s ...", hire_chain_post_url.c_str());
          u_int64_t chain_id = 0;
          bool check_result = _get_chainid_from_hirechain(hire_chain_post_url, chain_id);
          debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "check chain id from : %s(%lu)  %s", hire_chain_post_url.c_str(), chain_id, check_result ?  "succ" : "fail");
          if (check_result) check_result = (chain_id == _hire_chain_id);

          FJson::Value url_check_result;
          url_check_result["rpc_url"] = hire_chain_post_url;
          url_check_result["check_result"] = check_result;
          hire_chain_urls_check_result_js.push_back(url_check_result);
        }
      }
    }

  } while (false);

  root.clear();
  if (0 < hire_chain_urls_check_result_js.size())
    root["utg_chain_urls"] = hire_chain_urls_check_result_js;
  root["cmd"] = HIRECMD_CHECK_RPC_URL;
  root["result"] = rtCode;
}

void hireManager::_process_message_set_adminpasswd(ws_session &session, FJson::Value& root)
{
  /*
  input:
  {
    "cmd": HIRECMD_SET_ADMIN_PASSWD,
    "newpasswd": "xxxxxx",        //HexString(RC4(newpasswd, MD5(ticket)))) 
    "newpasswd_md5": "xxxxxx",    
    "ni":"xxxxx"                  
    "sin":"xxxxx",                //HexString(HMAC_SHA256(ni, ticket))
  }
  output:
  {
    "cmd": HIRECMD_SET_ADMIN_PASSWD,
    "result": CE_SUCC
  }
  */

  std::string enc_newpasswd = "", newpasswd_md5 = "";
  u_char keyBuf[16] = {0}, enc_data[256] = {0};
  u_int enc_data_len = 256;
  CN_ERR rtCode = CE_SUCC;

  do
  {
    if (!root["newpasswd"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    enc_newpasswd = root["newpasswd"].asString();
  
    if (!root["newpasswd_md5"].isString()) {
      rtCode = CE_PROTOCOL;
      break;
    }
    newpasswd_md5 = root["newpasswd_md5"].asString();

    auxHelper::hexstring2Byte(enc_newpasswd, enc_data, enc_data_len);

    hireVolume::getMD5(session.ticket, keyBuf);
    RC4((u_char *)enc_data, enc_data_len, keyBuf, 16);

    _hire_admin_passwd_md5 = hireVolume::getMD5(std::string((char *)enc_data));

    if (_hire_admin_passwd_md5 != newpasswd_md5) {
      rtCode = CE_INVALID_PARAMETER;
      break;
    }

    std::map<std::string, ws_session>::iterator it;
    lock_ws_session();
    it = _ws_sessions.find(session.ip);
    if (it != _ws_sessions.end()) 
      _ws_sessions.erase(it);
    unlock_ws_session();

    _save_admin_passwd();

    hireLogManager::instance().add_admin_login_log(HIRE_LOG_ACT_CHANGEPWD, session.ip);

  } while (false);
  
  root.clear();
  root["cmd"] = HIRECMD_SET_ADMIN_PASSWD;
  root["result"] = rtCode;
}

bool hireManager::_load_volume_bytes()
{
  struct statfs64 disk_statfs64;

  if (0 == statfs64(HIREFS_VOLUME_DIRECTORY, &disk_statfs64)) {
    _volume_total = disk_statfs64.f_blocks * disk_statfs64.f_bsize;
    u_int64_t volume_free = disk_statfs64.f_bfree * disk_statfs64.f_bsize;
    u_int64_t volume_free_reserve = HIRE_VOLUME_PACKAGE_FILE_SIZE * 2; 
    if (volume_free_reserve < volume_free)
      _volume_free = volume_free - volume_free_reserve;
    else _volume_free = 0;

    //debugEntry (LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "hire volume free : %llu(%llu) Bytes, total : %llu Bytes", _volume_free, volume_free, _volume_total);
    return true;
  } else {
    debugEntry (LL_WARN, LOG_MODULE_INDEX_HIRE, "statfs64 %s fail errno : %d.", HIREFS_VOLUME_DIRECTORY, errno);
    return false;
  }
}

bool hireManager::_load_admin_passwd()
{
  std::string hire_admin_passwd_md5 = "";

  if (false == auxHelper::read_from_file(HIREFS_ADMIN_PASSWD_FILE_NAME, hire_admin_passwd_md5)) {
    debugEntry (LL_WARN, LOG_MODULE_INDEX_HIRE, "read admin password file(%s) fail.", HIREFS_ADMIN_PASSWD_FILE_NAME);
    return false;
  }

  if (!hire_admin_passwd_md5.empty()) 
    _hire_admin_passwd_md5 = hire_admin_passwd_md5;

  return true;
}

void hireManager::_get_hire_config(FJson::Value &hire_config)
{
  /*
  {
    "name" : "xxx",
    "chain_id": xxx,
    "utg_retrieve_block_number" : xxx,
    "utg_one_day_block_number"  : xxxx,
    "utg_enquire_pledge_url" : "xxx",
    "utg_chain_urls" : [
      "https://rpc.ultronglow.io",
      "https://rpc.ultronglow.io"
    ]
  }
  */

  hire_config["name"] = _hire_config_name;
  hire_config["chain_id"] = _hire_chain_id;
  hire_config["utg_retrieve_block_number"] = _hire_volume_retrieve_block_number;
  hire_config["utg_one_day_block_number"] = _hire_one_day_block_number;
  hire_config["utg_enquire_pledge_url"] = _hire_enquire_pledge_url;

  FJson::Value hire_chain_urls_js;
  lock_hire_chain_post_urls();
  for (size_t i = 0; i < _hire_chain_post_urls.size(); i++) {
    hire_chain_urls_js.push_back(_hire_chain_post_urls[i]);
  }
  unlock_hire_chain_post_urls();

  if (0 < hire_chain_urls_js.size()) hire_config["utg_chain_urls"] = hire_chain_urls_js;
}

bool hireManager::_save_hire_config()
{
  bool rtCode = true;

  FJson::Value hire_config;
  _get_hire_config(hire_config);

  rtCode = auxHelper::save_to_file(_hirefs_config_file_name, hire_config.toCompatString());
  debugEntry (LL_DEBUG, LOG_MODULE_INDEX_HIRE, "save hire config : %s.", hire_config.toCompatString().c_str());
  
  return rtCode;
}

bool hireManager::_save_admin_passwd()
{
  bool rtCode = true;

  rtCode = auxHelper::save_to_file(HIREFS_ADMIN_PASSWD_FILE_NAME, _hire_admin_passwd_md5);
  debugEntry (LL_DEBUG, LOG_MODULE_INDEX_HIRE, "save admin password %s.", _hire_admin_passwd_md5.c_str());
  
  return rtCode;
}

bool hireManager::_load_private_key()
{
  std::string hire_info = "";
  FJson::Value root;
  FJson::Parser parser;

  if (false == auxHelper::read_from_file(HIREFS_PRIVATE_KEY_FILE_NAME, hire_info)) {
    debugEntry (LL_WARN, LOG_MODULE_INDEX_HIRE, "read private key file(%s) fail.", HIREFS_PRIVATE_KEY_FILE_NAME);
    return false;
  }

  if (!hire_info.empty()) {
    if (parser.load_string(hire_info, root)) {
      _hire_private_key = root["utg_prikey"].asString();
      _bhire_onpledge = root["utg_onpledge"].asBool();

       std::string hire_address = root["utg_addr"].asString();
      _hire_volume.set_hire_address(hire_address);
      _hire_volume.set_volume_package_filename_prefix(std::string("utg_") + hire_address);
    }
  }

  return true;
}

bool hireManager::_sava_private_key()
{
  bool rtCode = true;
  FJson::Value root;

  root["utg_prikey"] = _hire_private_key;
  root["utg_addr"] = _hire_volume.get_hire_address();
  root["utg_onpledge"] = _bhire_onpledge;

  rtCode = auxHelper::save_to_file(HIREFS_PRIVATE_KEY_FILE_NAME, root.toCompatString());
  //debugEntry (LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "save private key %s.", root.toCompatString().c_str());
  
  return rtCode;
}

bool hireManager::_load_poc_commit_history(const std::string &hire_poc_history_file, u_int64_t &block_number)
{
  std::string hire_poc_history = "";

  if (false == auxHelper::read_from_file(hire_poc_history_file.c_str(), hire_poc_history)) {
    debugEntry (LL_WARN, LOG_MODULE_INDEX_HIRE, "read poc history file(%s) fail.", hire_poc_history_file.c_str());
    return false;
  }

  if (!hire_poc_history.empty()) {
    block_number = strtoll(hire_poc_history.c_str(), NULL, 10);
  }

  return true;
}

bool hireManager::_save_poc_commit_history(const std::string &hire_poc_history_file, const std::string &block_number)
{
  return auxHelper::save_to_file(hire_poc_history_file.c_str(), block_number);
}

std::string hireManager::_httpsession_post_msg(const std::string &url, const std::string &msg)
{
  std::string rep_message = "";
  httpsession hp;

  do {
    
    debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "hire httpsession post request : %s", msg.c_str());

    if (url.empty()) break;

    httpsession::global_init();

    //Content-Type: application/json;charset=UTF-8
    if (false == hp.init_content_type(NULL, NULL, "Content-Type: application/json;charset=UTF-8", 30, 30)) break;
    
    if (false == hp.post(url, msg, false, "")) break;

    rep_message = hp.get_content();

    httpsession::global_cleanup();

    debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "hire httpsession post response : %s", rep_message.c_str());
  } while (false);

  return rep_message;
}

std::string hireManager::_httpsession_get_msg(const std::string &url)
{
  std::string rep_message = "";
  httpsession hp;

  do {
    
    debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "hire httpsession get url: %s", url.c_str());

    if (url.empty()) break;

    httpsession::global_init();

    //Content-Type: application/json;charset=UTF-8
    if (false == hp.init_content_type(NULL, NULL, "Content-Type: application/json;charset=UTF-8", 30, 30)) break;

    if (false == hp.get(url, false, 0, "")) break;

    rep_message = hp.get_content();

    httpsession::global_cleanup();

    debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "hire httpsession get response : %s", rep_message.c_str());
    
  } while (false);

  return rep_message;
}

void hireManager::hire_append_signature_to_rlp(int chain_id, RLPValue &rlp, int v, const std::string &r, const std::string &s)
{
  //V
  int vOffset = chain_id * 2 + 35;                       //vOffset  加入链id进行偏移，验证签名的时候要把偏移重置回来，相当于验证链id
  rlp.push_back(RLPValue(encode_bignum(v + vOffset)));   //v  : int
  
  //R  : u_char[32] but need remove leading zero
  InfInt r_num = decode_bignum(r);
  r_num.removeLeadingZeros();
  rlp.push_back(RLPValue(encode_bignum(r_num)));

  //S  : u_char[32] but need remove leading zero   
  InfInt s_num = decode_bignum(s);
  s_num.removeLeadingZeros();          
  rlp.push_back(RLPValue(encode_bignum(s_num)));
}

void hireManager::_check_hire_status()
{
  HIRE_STATUS hire_status = _hire_volume.get_hire_status();
  if (_bhire_onpledge && (HIREST_VOLUME_PACKAGED == hire_status || HIREST_VOLUME_OUTPLEDGE == hire_status))
    _hire_volume.set_hire_status(HIREST_VOLUME_IN_SERVICE);

  if (!_bhire_onpledge && HIREST_VOLUME_IN_SERVICE == hire_status)
    _hire_volume.set_hire_status(HIREST_VOLUME_OUTPLEDGE);
}

bool hireManager::_enquire_pledge()
{
  bool rtCode = false;

  do{
    
    lock_hire_chain_post_urls();
    for (size_t i = 0; i < _hire_chain_post_urls.size(); i++) {
      rtCode = _enquire_pledge_rpc(_hire_chain_post_urls[i]);
      if (rtCode) break;
    }
    unlock_hire_chain_post_urls();

    if (rtCode) break;
    
    rtCode = _enquire_pledge_old(_hire_enquire_pledge_url);
    
  } while (false);

  return rtCode;
}

bool hireManager::_enquire_pledge(const std::string &hire_chain_post_url)
{
  bool rtCode = false;

  do{

    rtCode = _enquire_pledge_rpc(hire_chain_post_url);
    if (rtCode) break;
    
    rtCode = _enquire_pledge_old(_hire_enquire_pledge_url);
    
  } while (false);

  return rtCode;
}

bool hireManager::_enquire_pledge_old(const std::string &hire_enquire_pledge_url)
{
  /*
  request : http://testexplorerapi.storage.io/platform/getStoragePledgeInfo?device_addr=xxx
  response:
  {
    "message":"successful", 
    "result":{
      "rent_hashs":[        
        "uxaaaa",
        "uxbbbb"
      ],
      "statue":1    
    },
    "statusCode":0  
  }
  */
  bool rtCode = false;
  FJson::Value root, result_js, rent_vouchers_js;
  FJson::Parser parser;
  std::string hire_address = "", enquire_pledge_url = "", response_message = "";
  bool  bhire_onpledge = _bhire_onpledge;
  
  do {
    
    hire_address = _hire_volume.get_hire_address();
    if (hire_enquire_pledge_url.empty() || hire_address.empty()) break;
    enquire_pledge_url = hire_enquire_pledge_url + hire_address;

    if (enquire_pledge_url.empty()) break;
    response_message = _httpsession_get_msg(enquire_pledge_url);
    
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "enquire_pledge_old (%s) response_message : %s", enquire_pledge_url.c_str(), response_message.c_str());
    if (response_message.empty()) break;

    if (!parser.load_string(response_message, root)) break;
    
    if (!root["statusCode"].isInt()) break;

    rtCode = true;

    if (0 != root["statusCode"].asInt()) { 
      bhire_onpledge = false;
      break;
    }

    result_js = root["result"];
    if (result_js["statue"].isInt()) {
      int pledgeStatus = result_js["statue"].asInt();
      bhire_onpledge =  (0 == pledgeStatus) ? true : false;
    }
    
    if (bhire_onpledge) _hire_volume.update_poc_groups();
      
  } while (false);
  
  if (_bhire_onpledge && !bhire_onpledge) _hire_volume.do_outpledg();

  if (bhire_onpledge != _bhire_onpledge) {
    _bhire_onpledge = bhire_onpledge;
    _sava_private_key();
  }

  return rtCode;
}

bool hireManager::_enquire_pledge_rpc(const std::string &hire_chain_post_url)
{
  /*
  request : 
  {
    "id":1,
    "jsonrpc":"2.0",
    "method":"alien_getSPledgeInfoByAddr",
    "params":[
      "ux894061376836017252597be087def12e3efc8844"
    ]
  }
  response:
  {
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
      "spledgeinfo": {
        "ux894061376836017252597be087def12e3efc8844": { 
          "pledgeStatus": 0, 
          "totalcapacity": 5368709120,
          "leftcapacity":  5368709120,
          "lease":[
            {
              "hash":"ux07e0395d024c4ea8a80f73da3207a41493df819ce458837233f9ee0f4614b2fd",
              "status": 0 
            },
            {
              "hash":"ux408a2c754334a52f37f3862cecd61f5f782d76735d956854188a630a4fa86f87",
              "status":6
            }
          ],
        }
      }
    }
  }
  */

  bool rtCode = false;
  FJson::Value request_js, response_js, params_js, result_js, spledgeinfo_js, hire_address_status_js, rent_items_status_js;
  FJson::Parser parser;
  std::string hire_address = "", request_message = "", response_message = "", ux_hire_address = "", u0x_hire_address = "";
  bool bhire_onpledge = _bhire_onpledge;
  std::map<std::string, int> rent_items_status;
  std::list<HIRE_RENT_ITEM> rent_items_volume_partitioning;

  do {

    u_int64_t hire_lasted_block_number = _enquire_lasted_block_number(hire_chain_post_url);
    if (0 == hire_lasted_block_number || hire_lasted_block_number <= _hire_last_block_number) break;
    _hire_last_block_number = hire_lasted_block_number;

    hire_address = _hire_volume.get_hire_address();
    if (hire_address.empty()) break;

    ux_hire_address = std::string("ux") + hire_address;
    u0x_hire_address = hireHelper::fromuxTo0x(ux_hire_address);
    params_js.push_back(ux_hire_address);
    _web3jrpc_request(request_js, 1, "alien_getSPledgeInfoByAddr", params_js);

    if (hire_chain_post_url.empty()) break;
    response_message = _httpsession_post_msg(hire_chain_post_url, request_js.toCompatString());

    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "enquire_pledge_rpc (%s) response_message : %s", hire_chain_post_url.c_str(), response_message.c_str());
    if (response_message.empty()) break;
    
    if (!parser.load_string(response_message, response_js)) break;
    
    if (response_js.isMember("error")) break;
    
    if (!response_js.isMember("result") || response_js["result"].isNull()) break;

    result_js = response_js["result"];
    if (!result_js.isMember("spledgeinfo")) break; 

    rtCode = true;

    spledgeinfo_js = result_js["spledgeinfo"];
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "spledgeinfo : %s", spledgeinfo_js.toCompatString().c_str());
    if (spledgeinfo_js.isNull()) {
      bhire_onpledge = false;
      _hire_volume.update_rent_items_onpledge(rent_items_status, _hire_rent_items_outpledge, rent_items_volume_partitioning);

      u_int64_t hire_volume_retrieve_block_number = _hire_volume_retrieve_block_number + _hire_one_day_block_number * 0.1;
      _hire_volume.update_rent_items(hire_lasted_block_number, hire_volume_retrieve_block_number);
      break;
    }
    
    if (!spledgeinfo_js.isMember(ux_hire_address) && !spledgeinfo_js.isMember(u0x_hire_address)) break;

    if (spledgeinfo_js.isMember(ux_hire_address)) {
      hire_address_status_js = spledgeinfo_js[ux_hire_address];
      debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "%s's spledgeinfo : %s",  ux_hire_address.c_str(), hire_address_status_js.toCompatString().c_str());
    }

    if (spledgeinfo_js.isMember(u0x_hire_address)) {
      hire_address_status_js = spledgeinfo_js[u0x_hire_address];
      debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "%s's spledgeinfo : %s",  ux_hire_address.c_str(), hire_address_status_js.toCompatString().c_str());
    }

    if (!hire_address_status_js["pledgeStatus"].isInt()) break;
    int pledgeStatus = hire_address_status_js["pledgeStatus"].asInt();
    bhire_onpledge = (0 == pledgeStatus) ? true : false;

    if (bhire_onpledge) {
      rent_items_status_js = hire_address_status_js["lease"];
      if (rent_items_status_js.isArray() && 0 < rent_items_status_js.size()) {

        for (size_t i = 0; i < rent_items_status_js.size(); i++) {
          FJson::Value rent_item_js = rent_items_status_js[i];

          std::string ux_rent_voucher = "";
          if (rent_item_js.isMember("hash") && rent_item_js["hash"].isString()) {
            ux_rent_voucher = rent_item_js["hash"].asString();
            ux_rent_voucher = hireHelper::from0xToux(ux_rent_voucher);
          }

          int rent_status = -1;   
          if (rent_item_js.isMember("status") && rent_item_js["status"].isInt())
            rent_status = rent_item_js["status"].asInt();
          
          rent_items_status.insert(std::make_pair(ux_rent_voucher, rent_status));
        }

        lock_rent_items_outpledge_list();
        _hire_volume.update_rent_items_onpledge(rent_items_status, _hire_rent_items_outpledge, rent_items_volume_partitioning);

        std::list<std::string>::iterator it;
        for (it = _hire_rent_items_outpledge.begin(); it != _hire_rent_items_outpledge.end(); it++)
          debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "rent item %s is outpledge", (*it).c_str());
        unlock_rent_items_outpledge_list();

        if (0 < _hire_rent_items_outpledge.size()) {
          bool bempty;
          _hirefs_commit_msg_queue.add_to_queue(HIRE_POST_POC_TYPE_RENT_RETRIEVE, bempty);
          _hirefs_commit_event.event_notify();
        }

        std::list<HIRE_RENT_ITEM>::iterator it_onpledge;
        for (it_onpledge = rent_items_volume_partitioning.begin(); it_onpledge != rent_items_volume_partitioning.end(); it_onpledge++)
          _notify_rent_volume_partitioning(*it_onpledge);
      }

      u_int64_t hire_volume_retrieve_block_number = _hire_volume_retrieve_block_number + _hire_one_day_block_number * 0.1;
      _hire_volume.update_rent_items(hire_lasted_block_number, hire_volume_retrieve_block_number);
      
      _hire_volume.update_poc_groups();
    } else _hire_volume.update_rent_items_onpledge(rent_items_status, _hire_rent_items_outpledge, rent_items_volume_partitioning);

  } while (false);

  if (_bhire_onpledge && !bhire_onpledge) _hire_volume.do_outpledg();

  if (bhire_onpledge != _bhire_onpledge) {
    _bhire_onpledge = bhire_onpledge;
    _sava_private_key();
  }

  return rtCode;
}

u_int64_t hireManager::_enquire_lasted_block_number()
{
  u_int64_t block_number = 0;

  lock_hire_chain_post_urls();
  for (size_t i = 0; i < _hire_chain_post_urls.size(); i++) {
    block_number = _enquire_lasted_block_number(_hire_chain_post_urls[i]);
    if (0 != block_number) break;
  }
  unlock_hire_chain_post_urls();  

  return block_number;
}

u_int64_t hireManager::_enquire_lasted_block_number(const std::string &hire_chain_post_url)
{
  u_int64_t block_number = 0;
  FJson::Value block_result_js;

  if (_get_block_from_hirechain(hire_chain_post_url, "latest", block_result_js)) 
    block_number = (u_int64_t)atoll(auxHelper::strToDecimalStr(block_result_js["number"].asString()).c_str());
  
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hire enquire lasted block number from %s : %llu", hire_chain_post_url.c_str(), block_number);

  return block_number;
}

std::string hireManager::_enquire_block_number(const std::string &hire_chain_post_url)
{
  std::string hire_block_number = "";

  int64_t hire_lasted_block_number = _enquire_lasted_block_number(hire_chain_post_url);
  if (0 == hire_lasted_block_number) return hire_block_number;

  if (0 < (hire_lasted_block_number - HIRE_GETBACK_BLOCK_NUMBER)) {
    char szbuffer[128] = {0};
    sprintf(szbuffer, "0x%x", hire_lasted_block_number - HIRE_GETBACK_BLOCK_NUMBER);
    hire_block_number = std::string(szbuffer);
  }

  return hire_block_number;
}

bool hireManager::_get_block_as_random_seed(const std::string &hire_chain_post_url, FJson::Value &root)
{
  bool rtCode = false;
  FJson::Value block_result_js;

  std::string hire_block_number = _enquire_block_number(hire_chain_post_url);
  if (hire_block_number.empty()) return rtCode;

  rtCode = _get_block_from_hirechain(hire_chain_post_url, hire_block_number, block_result_js);
  if (rtCode) {
    root["utg_block"] = block_result_js["number"].asString();
    root["utg_nonce"] = block_result_js["nonce"].asString();
    root["utg_block_hash"] = block_result_js["hash"].asString();
  }

  return rtCode;
}

bool hireManager::_do_post_volume_package_poc_to_hirechain()
{
  bool rtCode = false;

  lock_hire_chain_post_urls();
  for (size_t i = 0; i < _hire_chain_post_urls.size(); i++) {
    rtCode = _do_post_volume_package_poc_to_hirechain(_hire_chain_post_urls[i]);
    if (rtCode) break;
  }
  unlock_hire_chain_post_urls();

  return rtCode;
}

bool hireManager::_do_post_volume_package_poc_to_hirechain(const std::string &hire_chain_post_url)
{
  bool rtCode = false;
  FJson::Value root;
  std::string result = "", block_number = "";

  do {
    rtCode = _enquire_pledge(hire_chain_post_url);
    if (!rtCode) {
      result = HIRE_ENQUIRE_PLEDGE_FAIL;
      break;
    }

    if (!_bhire_onpledge) { 
      result = HIRE_OUT_PLEDGE;
      break;
    }

    rtCode = _get_block_as_random_seed(hire_chain_post_url, root);
    if (!rtCode) { 
      result = HIRE_ENQUIRE_BLOCK_FAIL;
      break;
    }

    block_number = root["utg_block"].asString();
    if (CE_SUCC != _hire_volume.do_volume_package_poc(root)) { 
      result = HIRE_CHECK_POC_FAIL;
      break;
    }

    rtCode = _do_post_poc_to_hirechain(hire_chain_post_url, root["utg_package_poc"].asString(), result);

  } while (false);

  return rtCode;
}

bool hireManager::_do_volume_rent_retrieve_poc_to_hirechain()
{
  bool rtCode = false;
  lock_hire_chain_post_urls();
  for (size_t i = 0; i < _hire_chain_post_urls.size(); i++) {
    rtCode = _do_volume_rent_retrieve_poc_to_hirechain(_hire_chain_post_urls[i]);
    if (rtCode) break;
  }
  unlock_hire_chain_post_urls();

  return rtCode;
}

bool hireManager::_do_volume_rent_retrieve_poc_to_hirechain(const std::string &hire_chain_post_url)
{
  bool rtCode = false;
  FJson::Value root;
  std::string result = "", rent_hashs = "", hire_block_number = "", data = "", poc_data_hexstring_out = "", block_hexstring_out = "";
  u_int64_t gasLimit = 800000, gasPrice = 0, nonce = 0, value = 0;

  do {
    if (0 == _hire_rent_items_outpledge.size()) break;
    
    std::list<std::string>::iterator it;
    lock_rent_items_outpledge_list();
    for (it = _hire_rent_items_outpledge.begin(); it != _hire_rent_items_outpledge.end(); it++) {
      if (!rent_hashs.empty()) rent_hashs += ",";
      rent_hashs += *it;
    }
    unlock_rent_items_outpledge_list();

    rtCode = _get_block_as_random_seed(hire_chain_post_url, root);
    if (!rtCode) {
      result = HIRE_ENQUIRE_BLOCK_FAIL;
      break;
    }
      
    hire_block_number = auxHelper::strToDecimalStr(root["utg_block"].asString());
    if (CE_SUCC != _hire_volume.do_volume_package_poc(root)) { 
      result = HIRE_CHECK_POC_FAIL;
      break;
    }

    data = std::string("UTG:1:stReValid:") + _hire_volume.get_hire_address() + std::string(":") + rent_hashs + ":" + root["utg_package_poc"].asString();
    poc_data_hexstring_out = auxHelper::byte2hexstring((const u_char *)data.c_str(), data.length());

    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hire rent retrieve poc data : %s", data.c_str());
    debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "hire rent retrieve poc data hexstring : %s", poc_data_hexstring_out.c_str());

    rtCode = _get_nonce_from_hirechain(hire_chain_post_url, _hire_volume.get_hire_address(), nonce);
    if (!rtCode) { 
      result = HIRE_ENQUIRE_NONCE_FAIL;
      break;
    }

    rtCode = _get_gasPrice_from_hirechain(hire_chain_post_url, gasPrice);
    if (!rtCode) { 
      result = HIRE_ENQUIRE_GASPRICE_FAIL;
      break;
    }

    rtCode = _get_gasLimit_from_hirechain(hire_chain_post_url, poc_data_hexstring_out, _hire_volume.get_hire_address(), value, gasPrice, gasLimit);
    if (!rtCode) { 
      result = HIRE_ENQUIRE_GAS_FAIL;
      break;
    }
    gasLimit = gasLimit * 1.2;

    _hirefs_data_to_block(block_hexstring_out, _hire_chain_id, nonce, gasPrice, gasLimit, _hire_volume.get_hire_address(), value, poc_data_hexstring_out, _hire_private_key);

    rtCode = _commit_block_to_hirechain(hire_chain_post_url, block_hexstring_out, result);
    if (!rtCode) break;

    lock_rent_items_outpledge_list();
    _hire_rent_items_outpledge.clear();
    unlock_rent_items_outpledge_list();

    hireLogManager::instance().add_volume_rent_log(HIRE_LOG_ACT_RETRIEVE_BY_DUE, rent_hashs, hire_block_number, std::string(""), auxHelper::intBytesToGBStr(_hire_volume.get_hire_package_volume_free()));

  } while (false);
  
  return rtCode;
}

bool hireManager::_do_post_poc_collection_to_hirechain_by_hire_poc_group_name(const std::string &hire_group_name, std::string &hire_poc_collection_post_result)
{
  bool rtCode = false;
  std::map<std::string, std::string> hire_poc_collection_post_results;

  rtCode = _do_post_poc_collection_to_hirechain_by_hire_poc_group_name(hire_poc_collection_post_results, hire_group_name);

  if (rtCode && 1 == hire_poc_collection_post_results.size()) 
    hire_poc_collection_post_result = hire_poc_collection_post_results.begin()->second;

  return rtCode;
}

bool hireManager::_do_post_poc_collection_to_hirechain_by_hire_poc_group_name(std::map<std::string, std::string> &hire_poc_collection_post_results, const std::string &hire_poc_group_name)
{
  bool rtCode = false;

  lock_hire_chain_post_urls();
  for (size_t i = 0; i < _hire_chain_post_urls.size(); i++) {
    rtCode = _do_post_poc_collection_to_hirechain_by_hire_poc_group_name(_hire_chain_post_urls[i], hire_poc_collection_post_results, hire_poc_group_name);
    if (rtCode) break;
  }
  unlock_hire_chain_post_urls();

  return rtCode;
}

bool hireManager::_do_post_poc_collection_to_hirechain_by_hire_poc_group_name(const std::string &hire_chain_post_url, std::map<std::string, std::string> &hire_poc_collection_post_results, const std::string &hire_group_name)
{
  bool rtCode = false;
  FJson::Value root;
  std::string result = "", hire_block_number = "";

  do {
    
    hire_poc_collection_post_results.clear(); 

    rtCode = _enquire_pledge(hire_chain_post_url);
    if (!rtCode) {
      result = HIRE_ENQUIRE_PLEDGE_FAIL;
      break;
    }

    if (!_bhire_onpledge) { 
      result = HIRE_OUT_PLEDGE;
      break;
    }

    rtCode = _get_block_as_random_seed(hire_chain_post_url, root);
    if (!rtCode) { 
      result = HIRE_ENQUIRE_BLOCK_FAIL;
      break;
    }

    hire_block_number = auxHelper::strToDecimalStr(root["utg_block"].asString());
    std::map<std::string, std::string> poc_collection_groups;
    _hire_volume.get_volume_poc_collection_by_group_name(root, hire_group_name, poc_collection_groups);
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "get volume poc collection by group name : %s(%u) ", hire_group_name.c_str(), poc_collection_groups.size());
    if (0 == poc_collection_groups.size()) {
      result = HIRE_CHECK_POC_FAIL;
      rtCode = false; 
      break;
    }

    std::map<std::string, std::string>::iterator it, it_temp;
    for (it = poc_collection_groups.begin(); it != poc_collection_groups.end() && !_stop_flag; it++) {
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "do post poc collection(group name : %s) ...", it->first.c_str());
      result = "";
      rtCode = _do_post_poc_to_hirechain(hire_chain_post_url, it->second, result);
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "do post poc collection(group name : %s, result : %s).", it->first.c_str(), result.c_str());
      if (!result.empty()) hire_poc_collection_post_results.insert(std::make_pair(it->first, result));

      //keep poc post record
      std::vector<std::string> hire_poc_group_voucher_items;
      _hire_volume.get_volume_poc_group_voucher_by_group_name(it->first, hire_poc_group_voucher_items);
      if (0 < hire_poc_group_voucher_items.size())
        hireLogManager::instance().add_poc_post_record(hire_poc_group_voucher_items[0], hire_block_number, hire_chain_post_url, result);
      
      it_temp = it;
      it_temp++;
      if (it_temp != poc_collection_groups.end()) {
        debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "do post poc collection wait begin");
        auxHelper::sleep_time_with_cancle(HIRE_POC_POST_RESULT_CHECK_TIMES * HIRE_ONE_BLOCK_NUMBER_TIME, &_stop_flag);
        debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "do post poc collection wait end");
      }
    }

    rtCode = (poc_collection_groups.size() == hire_poc_collection_post_results.size());
    if (rtCode) _save_poc_commit_history(HIREFS_VOLUME_PACKAGE_POC_COMMIT_HISTORY, hire_block_number);

  } while (false);

  if (!rtCode && !result.empty()) {
    std::vector<std::string> hire_poc_group_voucher_items;
    _hire_volume.get_volume_poc_group_voucher_by_group_name(hire_group_name, hire_poc_group_voucher_items);
    for (size_t i = 0; i < hire_poc_group_voucher_items.size(); i++) {
      hireLogManager::instance().add_poc_post_record(hire_poc_group_voucher_items[i], hire_block_number, hire_chain_post_url, result);
    }
  }

  return rtCode;
}

bool hireManager::_do_post_poc_collection_to_hirechain_every_day(std::map<std::string, std::string> &hire_poc_collection_post_results)
{
  return _do_post_poc_collection_to_hirechain_by_hire_poc_group_name(hire_poc_collection_post_results, "all");
}

bool hireManager::_check_poc_collection_post_result_from_hirechain(std::map<std::string, std::string> &hire_poc_collection_post_results, std::vector<std::string> &hire_poc_group_name_of_poc_collection_post_fail)
{
  std::map<std::string, std::string>::iterator it;

  hire_poc_group_name_of_poc_collection_post_fail.clear();
  for (it = hire_poc_collection_post_results.begin(); it != hire_poc_collection_post_results.end(); it++) {
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "check poc collection status : %s(group name : %s)...", it->second.c_str(), it->first.c_str());
    std::vector<HIRE_RENT_ITEM> hire_poc_items;
    _hire_volume.get_volume_poc_group_items_by_group_name(it->first, hire_poc_items);
    bool rtCode = _check_transaction_status_from_hirechain(it->second, hire_poc_items);
    if (!rtCode) hire_poc_group_name_of_poc_collection_post_fail.push_back(it->first);
  }

  return (0 == hire_poc_group_name_of_poc_collection_post_fail.size());
}

bool hireManager::_do_post_poc_to_hirechain(const std::string &hire_chain_post_url, const std::string &hire_poc, std::string &result)
{
  bool rtCode = false;
  std::string poc_data_hexstring_out = "", block_hexstring_out = "";
  u_int64_t gasLimit = 800000, gasPrice = 0, nonce = 0, value = 0;

  do {
    _hirefs_poc_to_block_data(poc_data_hexstring_out, _hire_volume.get_hire_address(), _hire_volume.get_hire_package_volume_expect(), hire_poc);
    
    rtCode = _get_nonce_from_hirechain(hire_chain_post_url, _hire_volume.get_hire_address(), nonce);
    if (!rtCode) {
      result = HIRE_ENQUIRE_NONCE_FAIL; 
      break;
    }

    rtCode = _get_gasPrice_from_hirechain(hire_chain_post_url, gasPrice);
    if (!rtCode) {
      result = HIRE_ENQUIRE_GASPRICE_FAIL; 
      break;
    }

    rtCode = _get_gasLimit_from_hirechain(hire_chain_post_url, poc_data_hexstring_out, _hire_volume.get_hire_address(), value, gasPrice, gasLimit);
    if (!rtCode) { 
      result = HIRE_ENQUIRE_GAS_FAIL;
      break;
    }
    gasLimit = gasLimit * 1.2;

    _hirefs_data_to_block(block_hexstring_out, _hire_chain_id, nonce, gasPrice, gasLimit, _hire_volume.get_hire_address(), value, poc_data_hexstring_out, _hire_private_key);
    rtCode = _commit_block_to_hirechain(hire_chain_post_url, block_hexstring_out, result);

  } while (false);
  
  return rtCode;
}

bool hireManager::_commit_block_to_hirechain(const std::string &hire_chain_post_url, const std::string &hire_block_data, std::string &result)
{
  /*
  request : 
    {
      "id":1,
      "jsonrpc":"2.0",
      "method":"eth_sendRawTransaction",
      "params":[
        "0xf9013301852905c54f9e8303"
      ]
    }
  response:
    { 
      "jsonrpc": "2.0",
      "id": 1,
      "error": {   
          "code": -32000,
          "message": "nonce too low"
      }
    }
    { 
      "id":1,
      "jsonrpc": "2.0",
      "result": "0xe670ec64341771606e55d6b4ca35a1a6b75ee3d5145a99d05921026d1527331"
    }
   */
  bool rtCode = false;
  FJson::Value request_js, response_js, params_js, error_js;
  FJson::Parser parser;
  std::string request_message = "", response_message = "";

  do {
    params_js.push_back(hire_block_data);
    _web3jrpc_request(request_js, 1, "eth_sendRawTransaction", params_js);

    if (hire_chain_post_url.empty()) break;
    response_message = _httpsession_post_msg(hire_chain_post_url, request_js.toCompatString());

    if (response_message.empty()) {
      result = HIRE_POST_TO_CHAIN_RPC_FAIL;
      break;
    }

    if (!parser.load_string(response_message, response_js)) break;
    
    if (response_js.isMember("error")) {
      error_js = response_js["error"];
      result = error_js["message"].asString();
      break;
    }
    
    if (!response_js.isMember("result")) break;

    if (!response_js["result"].isString() || response_js["result"].isNull()) break;

    result = response_js["result"].asString();
    if (result.empty()) break;

    rtCode = true;

  } while (false);
  
  return rtCode;
}

bool hireManager::_check_transaction_status_from_hirechain(const std::string &block_hash, const std::vector<HIRE_RENT_ITEM> &hire_poc_items)
{
  bool rtCode = false;

  if (!hireHelper::isBlockHash(block_hash)) return false;

  lock_hire_chain_post_urls();
  for (size_t i = 0; i < _hire_chain_post_urls.size(); i++) {
    rtCode = _check_transaction_status_from_hirechain(_hire_chain_post_urls[i], block_hash, hire_poc_items);
    if (rtCode) break;
  }
  unlock_hire_chain_post_urls();

  return rtCode;
}

bool hireManager::_check_transaction_status_from_hirechain(const std::string &hire_chain_post_url, const std::string &block_hash, const std::vector<HIRE_RENT_ITEM> &hire_poc_items)
{
  /*
  request : 
  {
    "id":1,
    "jsonrpc":"2.0",
    "method":"eth_getTransactionReceipt",
    "params":[
      "ux32c4fce07076bb94ac2c2d32ac3698e38b40606ce790d108a32a664cbaca0284"
    ]
  }
  response:
  { 
    "jsonrpc": "2.0",
    "id": 1,
    "result": null
  }
  { 
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "blockHash": "uxa54e36e13742ae6d05a5dc29fc34806d3cc855fae0f8eb7b574c71299b8fdbc3",
        "blockNumber": "0x84a6",
        "contractAddress": null,
        "cumulativeGasUsed": "0xfdc8",
        "effectiveGasPrice": "0x2905c54f9e",
        "from": "uxe98ef7645b716746e5505bf901ff7241d19f4ee0",
        "gasUsed": "0xfdc8",
        "logs": [
            {
                "address": "ux0000000000000000000000000000000000000000",
                "topics": [
                    "uxb259d26eb65071ded303add129ecef7af12cf17a8ea9d41f7ff0cfa5af3123f8",
                    "ux000000000000000000000000e98ef7645b716746e5505bf901ff7241d19f4ee0",
                    "ux0000000000000000000000000000000000000000000000000000003333393537"
                ],
                "data": "0x",
                "blockNumber": "0x84a6",
                "transactionHash": "ux32c4fce07076bb94ac2c2d32ac3698e38b40606ce790d108a32a664cbaca0184",
                "transactionIndex": "0x0",
                "blockHash": "uxa54e36e13742ae6d05a5dc29fc34806d3cc855fae0f8eb7b574c71299b8fdbc3",
                "logIndex": "0x0",
                "removed": false
            }
        ],
        "logsBloom": "0x00000008000000000080000000000000000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000020000000000000000000000040000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000002000000000000000000000000000000100000000000000000000000000000000000000000000000000000020000000008000000000000000000000000000000000000000000000000000200000000000000040000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "status": "0x1",
        "to": "uxe98ef7645b716746e5505bf901ff7241d19f4ee0",
        "transactionHash": "ux32c4fce07076bb94ac2c2d32ac3698e38b40606ce790d108a32a664cbaca0184",
        "transactionIndex": "0x0",
        "type": "0x0"
    }
  }
  */

  bool rtCode = false;
  FJson::Value request_js, response_js, params_js, result_js, logs_js, topics_js;
  FJson::Parser parser;
  std::string request_message = "", response_message = "", block_status = "";

  do {
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Check Transaction Receipt : %s.", block_hash.c_str());

    params_js.push_back(block_hash);
    _web3jrpc_request(request_js, 1, "eth_getTransactionReceipt", params_js);

    if (hire_chain_post_url.empty()) break;
    response_message = _httpsession_post_msg(hire_chain_post_url, request_js.toCompatString());

    if (response_message.empty()) break;

    if (!parser.load_string(response_message, response_js)) break;
    
    if (response_js.isMember("error")) break;
    
    if (!response_js.isMember("result") || response_js["result"].isNull()) break;

    result_js = response_js["result"];
    if (!result_js.isMember("status")) break;

    if (!result_js["status"].isString()) break;
    block_status = result_js["status"].asString();
    
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "CHECK Transaction Receipt : %s, status :  %s", block_hash.c_str(), block_status.c_str());
   
    rtCode = true;

    if ("0x1" != block_status) break;

    if (!result_js.isMember("logs")) break;
    logs_js = result_js["logs"];
    if (!logs_js.isArray() || 0 == logs_js.size()) break;
    for (size_t i = 0; i < logs_js.size(); i++) {
      do {
      
        FJson::Value log_item = logs_js[i];
        debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "CHECK Transaction Receipt : %s : log item : %s", block_hash.c_str(), log_item.toCompatString().c_str());
        
        //check topics
        if (!log_item.isMember("topics")) break;
        topics_js = log_item["topics"];
        if (!topics_js.isArray() || 0 == topics_js.size()) break;

        //check data
        if (!log_item.isMember("data") || !log_item["data"].isString()) break;
        std::string data_hexstring = log_item["data"].asString();
        data_hexstring = data_hexstring.substr(2, data_hexstring.length());
        debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "CHECK Transaction Receipt : %s : data_hexstring : %s", block_hash.c_str(), data_hexstring.c_str());

        std::string data = auxHelper::der_to_pem(data_hexstring);
        std::transform(data.begin(), data.end(), data.begin(), ::tolower);
        debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "CHECK Transaction Receipt : %s : data : %s", block_hash.c_str(), data.c_str());

        hireLogManager::instance().set_poc_post_commit_confirm(block_hash, data);
        
        for (size_t i = 0; i < hire_poc_items.size(); i++) {
          if (std::string::npos == data.find(hire_poc_items[i].hire_rent_voucher)) {
            debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "CHECK Transaction Receipt data find voucher(%s) fail.", hire_poc_items[i].hire_rent_voucher.c_str());
          } else debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "CHECK Transaction Receipt data find voucher(%s) succ.", hire_poc_items[i].hire_rent_voucher.c_str());
        }

      } while (false);
      
    }

  } while (false);
  
  return rtCode;
}

bool hireManager::_get_block_from_hirechain(const std::string &hire_chain_post_url, const std::string &numer, FJson::Value &result_js)
{
  /*
  request : 
    {
      "id":1,
      "jsonrpc":"2.0",
      "method":"eth_getBlockByNumber",
      "params":[
        "0x514",
        true
      ]
    }
  response:
    { 
      "jsonrpc": "2.0",
      "id": 1,
      "error": {
          "code": -32602,
          "message": "invalid argument 0: hex string without 0x prefix"
      }
    }
    { 
      "jsonrpc": "2.0",
      "id": 1,
      "result": {
        "difficulty": "0x1",
        "extraData": "0xd883010a048375746789676f312e31362e3130856c696e757800000000000000f90292c0c0c0c0c0846166e61df901b9943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66bc0820506c0c0c0c0c0c0c0c0c08080c0c0c0c08806f05b59d3b20000e0df943efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b8806f05b59d3b2000003a00000000000000000000000000000000000000000000000000000000000000000c0c0c0c0c0c0c0c0c0c0c0a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347780825549a36840df13b0928bf6ca12f245d7528ec89ceb70fa0aa6f80ae29610c6a9816132ce010d5df9f51f078fa80a6f16db67d4725e282f3bec851a8d61601",
        "gasLimit": "0x7a1200",
        "gasUsed": "0x0",
        "hash": "uxa69bf68170b2c9beeda78a8674105fc4901babffec99ac00d5a2c8644d671c7f",
        "logsBloom": "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
        "miner": "ux3efbaa018f81f9d0ef8b3a3d0d0b767d3482b66b",
        "mixHash": "ux0000000000000000000000000000000000000000000000000000000000000000",
        "nonce": "0x0000000000000000",
        "number": "0x514",
        "parentHash": "ux134ccba6c3be7a9503f7b29e3c7147ad2eec1d453e70898253c1c8cd9d3b7b8f",
        "receiptsRoot": "ux56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
        "sha3Uncles": "ux1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347",
        "size": "0x4f7",
        "stateRoot": "uxb3626cd8f09fefb681fb0663771fc0ac2899bb059df07b9b0c94f0fb5aa382e1",
        "timestamp": "0x624eab91",
        "totalDifficulty": "0x515",
        "transactions": [],
        "transactionsRoot": "ux56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
        "uncles": []
      }
    }
   */
  bool rtCode = false;
  FJson::Value request_js, response_js, params_js;
  FJson::Parser parser;
  std::string request_message = "", response_message = "";

  do {
    params_js.push_back(numer);
    params_js.push_back(true);
    _web3jrpc_request(request_js, 1, "eth_getBlockByNumber", params_js);

    if (hire_chain_post_url.empty()) break;
    response_message = _httpsession_post_msg(hire_chain_post_url, request_js.toCompatString());

    if (response_message.empty()) break;

    if (!parser.load_string(response_message, response_js)) break;
    
    if (response_js.isMember("error")) break;
    
    if (!response_js.isMember("result")) break;
    result_js = response_js["result"];

    rtCode = true;

  } while (false);
  
  return rtCode;
}

bool hireManager::_get_gasPrice_from_hirechain(const std::string &hire_chain_post_url, u_int64_t &gasPrice)
{
  /*
  request : 
    {
      "id":1,
      "jsonrpc":"2.0",
      "method":"eth_gasPrice",
      "params":null
    }
  response:
    {
      "jsonrpc": "2.0",
      "id": 1,
      "result": "0x2905c54f9e"
    }
   */
  bool rtCode = false;
  FJson::Value request_js, response_js, params_js;
  FJson::Parser parser;
  std::string request_message = "", response_message = "";

  do {
    _web3jrpc_request(request_js, 1, "eth_gasPrice", params_js);

    if (hire_chain_post_url.empty()) break;
    response_message = _httpsession_post_msg(hire_chain_post_url, request_js.toCompatString());

    if (response_message.empty()) break;

    if (!parser.load_string(response_message, response_js)) break;
    
    if (response_js.isMember("error")) break;
    
    if (!response_js.isMember("result")) break;

    if (!response_js["result"].isString()) break;

    gasPrice = strtoll(response_js["result"].asString().c_str(), NULL, 16);

    rtCode = true;

  } while (false);
  
  return rtCode;
}

bool hireManager::_get_gasLimit_from_hirechain(const std::string &hire_chain_post_url, const std::string &data_hexstring_out, const std::string &hire_address, u_int64_t value, u_int64_t gasPrice, u_int64_t &gasLimit)
{
  /*
  request : 
    {
      "id":1,
      "jsonrpc":"2.0",
      "method":"eth_estimateGas",
      "params":[
        {
          "data":"0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675",
          "from":"uxadae399384e44dae5552dab9964b59ee7cc7806a",
          "gas":"",
          "gasPrice":"0x2905c54f9e",
          "to":"uxadae399384e44dae5552dab9964b59ee7cc7806a",
          "value":"0x0"
        }
      ]
    }
  response:
    {
        "jsonrpc": "2.0",
        "id": 1,
        "result": "0xc6f8"
    }
   */
  bool rtCode = false;
  FJson::Value request_js, response_js, params_js, transaction_js;
  FJson::Parser parser;
  std::string request_message = "", response_message = "", ux_hire_address = "", ux_data_hexstring_out = "";
  char szbuffer[128] = {0};

  do {
    //gaslimit
    transaction_js["gas"] = std::string("");

    //from and to
    ux_hire_address = std::string("ux") + hire_address;
    transaction_js["from"] = ux_hire_address;
    transaction_js["to"] = ux_hire_address;

    //data
    ux_data_hexstring_out = std::string("0x") + data_hexstring_out;
    transaction_js["data"] = ux_data_hexstring_out;

    //gasPrice
    bzero(szbuffer, 128);
    sprintf(szbuffer, "0x%x", gasPrice);
    transaction_js["gasPrice"] = std::string(szbuffer);

    //value
    bzero(szbuffer, 128);
    sprintf(szbuffer, "0x%x", value);
    transaction_js["value"] = std::string(szbuffer);

    params_js.push_back(transaction_js);
    _web3jrpc_request(request_js, 1, "eth_estimateGas", params_js);

    if (hire_chain_post_url.empty()) break;
    response_message = _httpsession_post_msg(hire_chain_post_url, request_js.toCompatString());

    if (response_message.empty()) break;

    if (!parser.load_string(response_message, response_js)) break;
    
    if (response_js.isMember("error")) break;
    
    if (!response_js.isMember("result")) break;

    if (!response_js["result"].isString()) break;

    gasLimit = strtoll(response_js["result"].asString().c_str(), NULL, 16);

    rtCode = true;

  } while (false);
  
  return rtCode;
}

bool hireManager::_get_nonce_from_hirechain(const std::string &hire_chain_post_url, const std::string &hire_address, u_int64_t &nonce)
{
  /*
  request : 
    {
      "id":1,
      "jsonrpc":"2.0",
      "method":"eth_getTransactionCount",
      "params":null
    }
  response:
    {
        "jsonrpc": "2.0",
        "id": 1,
        "result": "0x0"
    }
   */
  bool rtCode = false;
  FJson::Value request_js, response_js, params_js;
  FJson::Parser parser;
  std::string request_message = "", response_message = "";

  do {
    params_js.push_back(std::string("0x") + hire_address);
    params_js.push_back("latest");
    _web3jrpc_request(request_js, 1, "eth_getTransactionCount", params_js);

    if (hire_chain_post_url.empty()) break;
    response_message = _httpsession_post_msg(hire_chain_post_url, request_js.toCompatString());

    if (response_message.empty()) break;

    if (!parser.load_string(response_message, response_js)) break;
    
    if (response_js.isMember("error")) break;
    
    if (!response_js.isMember("result")) break;

    if (!response_js["result"].isString()) break;

    nonce = strtoll(response_js["result"].asString().c_str(), NULL, 16);

    rtCode = true;

  } while (false);
  
  return rtCode;
}

 bool hireManager::_get_chainid_from_hirechain(const std::string &hire_chain_post_url, u_int64_t &chain_id)
 {
  /*
  request : 
  {
    "id":1,
    "jsonrpc":"2.0",
    "method":"eth_chainId",
    "params":[]
  }
  response:
  {
    "id":1,
    "jsonrpc":"2.0",
    "result":"0xbc"
  }
   */

  bool rtCode = false;
  FJson::Value request_js, response_js, params_js;
  FJson::Parser parser;
  std::string request_message = "", response_message = "";

  do {
    _web3jrpc_request(request_js, 1, "eth_chainId", params_js);

    if (hire_chain_post_url.empty()) break;
    response_message = _httpsession_post_msg(hire_chain_post_url, request_js.toCompatString());

    if (response_message.empty()) break;

    if (!parser.load_string(response_message, response_js)) break;
    
    if (response_js.isMember("error")) break;
    
    if (!response_js.isMember("result")) break;

    if (!response_js["result"].isString()) break;

    chain_id = strtoll(response_js["result"].asString().c_str(), NULL, 16);

    rtCode = true;

  } while (false);
  
  return rtCode;
 }

void hireManager::_hirefs_poc_to_block_data(std::string &data_hexstring_out, const std::string &hire_address, u_int64_t hire_volume_Bytes, const std::string &hirefs_poc)
{
  std::string data = std::string("UTG:1:stProof:") + hire_address + std::string("::") + auxHelper::intBytesToGBStr(hire_volume_Bytes) + std::string(":") + hirefs_poc;
  data_hexstring_out = auxHelper::byte2hexstring((const u_char *)data.c_str(), data.length());

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hire poc data : %s", data.c_str());
  debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "hire poc data hexstring : %s", data_hexstring_out.c_str());
}

void hireManager::_hirefs_data_to_block(std::string &block_hexstring_out, int chain_id, u_int64_t nonce, u_int64_t gasPrice, u_int64_t gasLimit, 
                                  const std::string &hire_der_address, u_int64_t value, const std::string &data_hexstring, const std::string &hire_der_prikey)
{
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "chain id        : %d", chain_id);
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "nonce           : %llu", nonce);
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "gasPrice        : %llu", gasPrice);
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "gasLimit        : %llu", gasLimit);
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "toAddress       : %s", hire_der_address.c_str());
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "value           : %llu", value);

  RLPValue block_rlp;
  block_rlp.setArray();

  block_rlp.push_back(RLPValue(encode_bignum(nonce)));                       //0 nonce        : u_int64_t
  block_rlp.push_back(RLPValue(encode_bignum(gasPrice)));                    //1 gasPrice     : u_int64_t
  block_rlp.push_back(RLPValue(encode_bignum(gasLimit)));                    //2 gasLimit     : u_int64_t
  block_rlp.push_back(RLPValue(auxHelper::der_to_pem(hire_der_address)));     //3 toAddress    : u_char[20] hire Address 20Bytes
  block_rlp.push_back(RLPValue(encode_bignum(value)));                       //4 value        : u_int64_t
  block_rlp.push_back(RLPValue(auxHelper::der_to_pem(data_hexstring)));      //5 data         : u_char[]

  u_char sha3[32] = {0};
  std::string block_rlp_out;
  block_rlp_out.append(block_rlp.write());
  _hirefs_rlp_Eip155hash(sha3, block_rlp, chain_id);
  
  int v = 0;
  std::string r, s;
  blockChainTools::bct_signature(hire_der_prikey, sha3, v, r, s);
  hire_append_signature_to_rlp(chain_id, block_rlp, v, r, s);
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "v : %u", v);
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "r : %s", auxHelper::pem_to_der(r).c_str());
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "s : %s", auxHelper::pem_to_der(s).c_str());
  debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "block signature %s", blockChainTools::is_bct_signature_valid(v, r, s) ? "succ" : "fail");

  block_rlp_out.clear();
  block_rlp_out.append(block_rlp.write());
  block_hexstring_out = "0x";
  block_hexstring_out += auxHelper::pem_to_der(block_rlp_out);

  debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "HIRE BLOCK HEXSTRING : \n%s", block_hexstring_out.c_str());
}

void hireManager::_hirefs_rlp_Eip155hash(u_char *hash, const RLPValue &block_rlp, int chain_id)
{
  RLPValue sig_rlp = block_rlp;
  sig_rlp.push_back(RLPValue(encode_bignum(chain_id)));
  sig_rlp.push_back(RLPValue(encode_bignum(0)));
  sig_rlp.push_back(RLPValue(encode_bignum(0)));

  std::string sig_rlp_out;
  sig_rlp_out.append(sig_rlp.write());
  SHA3_256((u_int8_t *)hash, (const u_int8_t *)sig_rlp_out.c_str(), sig_rlp_out.size());

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hire block hash : %s", auxHelper::byte2hexstring(hash, 32).c_str());
}

void hireManager::_web3jrpc_request(FJson::Value &request_js, u_int request_id, const std::string &method, FJson::Value &params_js)
{
  /*
  {
      "id":1,
      "jsonrpc":"2.0",
      "method":"xxxxxx",
      "params":[
        "0xf9013301852905c54f9e8303"
      ]
    }
  */
  request_js["id"] = request_id;
  request_js["jsonrpc"] = std::string("2.0");
  request_js["method"] = method;
  request_js["params"] = params_js;
}

u_int hireManager::_get_mem_used() 
{ 
  /*
  MemTotal:       32737880 kB
  MemFree:          220512 kB
  MemAvailable:   30524120 kB
  Buffers:           91040 kB
  Cached:         29756316 kB
  */
  MEM_OCCUPY m;
  u_int mem_used = 0;
  FILE *fd = NULL;  
  char buff[256] = {0};  
  
  fd = fopen("/proc/meminfo", "r");
  if (NULL == fd) return 0;

  fgets(buff, sizeof(buff), fd);  
  sscanf(buff, "%s %lu ", m.name1, &(m.MemTotal));      //MemTotal

  fgets(buff, sizeof(buff), fd);  
  sscanf(buff, "%s %lu ", m.name2, &(m.MemFree));       //MemFree

  fgets(buff, sizeof(buff), fd);  
  sscanf(buff, "%s %lu ", m.name3, &(m.MemAvailable));  //MemAvailable

  if (NULL != fd)  fclose(fd);

  return ((1 - (m.MemAvailable * 1.0) / ((m.MemTotal + 1) * 1.0)) * 100);
}

int hireManager::_get_cpuoccupy(CPU_OCCUPY *cpust) 
{  
  FILE *fd = NULL;  
  char buff[256] = {0};  
  CPU_OCCUPY *cpu_occupy = NULL;  
  cpu_occupy = cpust;  
    
  fd = fopen("/proc/stat", "r");
  if (NULL == fd) return 0;

  fgets(buff, sizeof(buff), fd);  
  sscanf(buff, "%s %u %u %u %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice, &cpu_occupy->system, &cpu_occupy->idle, &cpu_occupy->lowait, &cpu_occupy->irq, &cpu_occupy->softirq);  
  
  if (NULL != fd) fclose(fd);  
    
  return 0;  
}
  
double hireManager::_cal_cpuoccupy(CPU_OCCUPY *o, CPU_OCCUPY *n)  
{  
  unsigned long old_total, new_total, old_used, new_used;  
  double cpu_use = 0;
    
  old_total = (unsigned long)(o->user + o->nice + o->system + o->idle + o->lowait + o->irq + o->softirq);
  old_used = (unsigned long)(o->user + o->nice + o->system);

  new_total = (unsigned long)(n->user + n->nice + n->system + n->idle + n->lowait + n->irq + n->softirq);
  new_used = (unsigned long)(n->user + n->nice + n->system);
  
  return ((new_used - old_used) * 1.0 / (new_total - old_total + 1) * 1.0) * 100;
}
