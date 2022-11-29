#include "hireVolume.h"
#include "loggerLocal.h"
#include "auxHelper.h"
#include "rlpvalue/InfInt.h"
#include "hireLogManager.h"
#include <math.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

MerkleTree::MerkleTree()
{
  _buffer_size = 0 ;
  _buffer = NULL;
  _data_node_index = 0;
  bzero(_zero_node.data, DATA_NODE_SIZE);
}

MerkleTree::~MerkleTree()
{
  unInitMerkleTree();
}

bool MerkleTree::initMerkleTree(u_char depth)
{ 
  bool rtCode = false;

  do
  {
    for (size_t i = 0; i < depth; i++) {
      PDEPTH_LEAFS_VECTOR pdepth_leafs_vector = new DEPTH_LEAFS_VECTOR;
      _tree.push_back(pdepth_leafs_vector);
    }
  
    u_int all_leafs = 0;
    for (size_t j = 0; j < _tree.size(); j++)
      all_leafs += std::pow(2, j);
    
    _buffer_size = all_leafs * DATA_NODE_SIZE;
    _buffer = new u_char[_buffer_size];
    if (NULL == _buffer) break;
    bzero(_buffer, _buffer_size);

    //printf("all leafs %u, buffer size : %u\n", all_leafs, _buffer_size);
    
    u_char *pdata_node = _buffer;
    for (size_t j = 0; j < _tree.size(); j++) {
      u_int level_leafs = std::pow(2, j);
      for (size_t k = 0; k < level_leafs; k++) {
        _tree[j]->push_back((data_node *)pdata_node);
        pdata_node += DATA_NODE_SIZE;
      }
    }
    
    rtCode = true;

    //for (size_t i = 0; i < _tree.size(); i++)
      //debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "L%02u, leafs %u", i, _tree[i]->size());
  } while (false);

  return rtCode;
}

void MerkleTree::unInitMerkleTree()
{
  if (NULL != _buffer) { 
    delete[] _buffer;
    _buffer = NULL;
  }

  for (size_t i = 0; i < _tree.size(); i++) {
    PDEPTH_LEAFS_VECTOR pdepth_leafs_vector = _tree[i];
    if (NULL != pdepth_leafs_vector) delete pdepth_leafs_vector;
  }
  _tree.clear();
  _data_node_index = 0;
  _buffer_size = 0;
  bzero(_zero_node.data, DATA_NODE_SIZE);
}

data_node* MerkleTree::getNode(u_char depth_index, u_int level_index)
{
  data_node* pnode = NULL;

  if (0 < _tree.size() && depth_index < _tree.size())
    if (0 < (*_tree[depth_index]).size() && level_index < (*_tree[depth_index]).size()) 
      pnode = (*_tree[depth_index])[level_index];

  return pnode;
}

data_node* MerkleTree::getDataLevelNode(u_int index)
{
  return getNode(_tree.size() - 1, index);
}

data_node* MerkleTree::getDataLevelFirstNode()
{
  return getNode(_tree.size() - 1, 0);
}

data_node* MerkleTree::getDataLevelLastNode()
{
  return getNode(_tree.size() - 1, getLevelNodeSize(_tree.size() - 1) - 1);
}

data_node* MerkleTree::getMerkleTreeRootNode()
{
  return getNode(0, 0);
}

u_int MerkleTree::getLevelBufferSize(u_char depth_index) 
{
  return (DATA_NODE_SIZE * getLevelNodeSize(depth_index));
}

u_int MerkleTree::getLevelNodeSize(u_char depth_index)
{
  return std::pow(2, depth_index);
}

u_char MerkleTree::volumeToDepth(u_int64_t volume_bytes)
{
  u_int64_t data_level_block_bytes = MerkleTree::getLevelBufferSize(DATA_MERKLE_TREE_DEPTH_INDEX);
  u_int64_t number_of_block = volume_bytes / data_level_block_bytes;
  double topmost_depth_index_double1 = std::log2(number_of_block);
  u_char topmost_depth_index = (u_char)std::log2(number_of_block);
  if (topmost_depth_index_double1 > (topmost_depth_index * 1.0)) topmost_depth_index += 1;

  return topmost_depth_index;
}

void MerkleTree::setZeroNode(u_char *pdata, u_int data_len)
{ 
  SHA_CTX ctx;
  //B0 = Hash(Data)
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, pdata, data_len);
  SHA1_Final(_zero_node.data, &ctx);
}

bool MerkleTree::addDataLevelNode(data_node* pdata_node)
{
  bool rtCode = false;

  if (0 < _tree.size() && NULL != pdata_node) {
    u_int data_depth = _tree.size() - 1;
    PDEPTH_LEAFS_VECTOR pdepth_leafs_vector = _tree[data_depth];
    u_int depth_leafs = (*pdepth_leafs_vector).size();
    if (0 < depth_leafs && _data_node_index < depth_leafs)
      memcpy((*pdepth_leafs_vector)[_data_node_index++]->data, pdata_node->data, DATA_NODE_SIZE);
    rtCode = true;
  }

  return rtCode;
}

void MerkleTree::produceDataLevel()
{
  if (0 < _tree.size()) {
    u_int data_depth = _tree.size() - 1;
    PDEPTH_LEAFS_VECTOR pdepth_leafs_vector = _tree[data_depth];
    u_int64_t data_level_node_size = getLevelNodeSize(getDepth() - 1);
    
    u_int depth_leafs = (*pdepth_leafs_vector).size();
    if (0 < depth_leafs) {
      if (0 == _data_node_index) {
        memcpy((*pdepth_leafs_vector)[0]->data, _zero_node.data, DATA_NODE_SIZE);
        _data_node_index++;
      } else {
        if (0 == _data_node_index % data_level_node_size) {
          SHA_CTX ctx;
          SHA1_Init(&ctx);
        
          //B0
          SHA1_Update(&ctx, _zero_node.data, DATA_NODE_SIZE);
          //Bn-1 
          SHA1_Update(&ctx, (*pdepth_leafs_vector)[depth_leafs - 1]->data, DATA_NODE_SIZE);
          //data block index
          SHA1_Update(&ctx, &_data_node_index, sizeof(u_int64_t));
          
          SHA1_Final((*pdepth_leafs_vector)[0]->data, &ctx);
          
          //printf("B%llu : %s\n\n", _data_node_index, auxHelper::byte2hexstring((*pdepth_leafs_vector)[0]->data, DATA_NODE_SIZE).c_str());

          _data_node_index++;  
        }
      }
      
      //Bn = Hash( (B0 + Bn-1 + n)
      for (size_t i = 1; i < depth_leafs; i++) {
        if (0 == (1 & _data_node_index)) {
          SHA_CTX ctx;
          SHA1_Init(&ctx);
        
          //B0
          SHA1_Update(&ctx, _zero_node.data, DATA_NODE_SIZE);
          SHA1_Update(&ctx, (*pdepth_leafs_vector)[i - 1]->data, DATA_NODE_SIZE);
          SHA1_Update(&ctx, &_data_node_index, sizeof(u_int64_t));
          
          SHA1_Final((*pdepth_leafs_vector)[i]->data, &ctx);

          //printf("B%llu : %s\n\n", _data_node_index, auxHelper::byte2hexstring((*pdepth_leafs_vector)[i]->data, DATA_NODE_SIZE).c_str());

          _data_node_index++;
        } else {
          _accumulation(_zero_node.data, (*pdepth_leafs_vector)[i - 1]->data, (*pdepth_leafs_vector)[i]->data, _data_node_index);
          _data_node_index++;
        } 
      }
    }
  }
}

bool MerkleTree::produceMerkleLevles(bool bstart_by_hash)
{
  bool bnext_hash = bstart_by_hash;

  if (0 < _tree.size()) {
    u_int depth_index = _tree.size() - 1;
    for (size_t i = depth_index; i > 0; i--) {
      _produceMerkleLevles(*_tree[i], *_tree[i - 1], bnext_hash);
      bnext_hash = (!bnext_hash);
    }
  }

  return bnext_hash;
}

void MerkleTree::produceDataLevelNode(u_int64_t data_node_index, data_node * pzero_node, data_node * pn_1_node, data_node * pn_node_out)
{
  if (NULL == pzero_node || NULL == pn_1_node || NULL == pn_node_out) return;

  if (0 == data_node_index) 
    memcpy(pn_node_out->data, pzero_node->data, DATA_NODE_SIZE);
  else {
    if (0 == (1 & data_node_index)) {
      SHA_CTX ctx;
      SHA1_Init(&ctx);

      SHA1_Update(&ctx, pzero_node->data, DATA_NODE_SIZE);
      SHA1_Update(&ctx, pn_1_node->data, DATA_NODE_SIZE);
      SHA1_Update(&ctx, &data_node_index, sizeof(u_int64_t));
      
      SHA1_Final(pn_node_out->data, &ctx);

    } else _accumulation(pzero_node->data, pn_1_node->data, pn_node_out->data, data_node_index);
  }
  
  //printf("produceDataLevelNode : %s [%llu] %s\n", 
  //    auxHelper::byte2hexstring(pn_node_out->data, DATA_NODE_SIZE).c_str(), data_node_index, (0 == (1 & data_node_index)) ? "hash ": "acc");
}

bool MerkleTree::dataLevelNodeCheck(u_int64_t data_node_index, data_node * pzero_node, data_node * pn_1_node, data_node * pn_node)
{
  data_node test_node;
  bool rtCode = false;
  
  if (NULL == pzero_node || NULL == pn_1_node || NULL == pn_node) return rtCode;

  produceDataLevelNode(data_node_index, pzero_node, pn_1_node, &test_node);
  //printf("dataLevelNodeCheck : %s [%llu]\n", auxHelper::byte2hexstring(test_node.data, DATA_NODE_SIZE).c_str(), data_node_index);
  rtCode = (0 == memcmp(pn_node->data, test_node.data, DATA_NODE_SIZE));

  return rtCode;  
}

void MerkleTree::produceMerkleLevelNode(data_node * pchild_node_a, data_node * pchild_node_b, data_node * pfather_node_out, bool bhash)
{
  if (NULL == pchild_node_a || NULL == pchild_node_b || NULL == pfather_node_out) return;

  if (bhash) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, pchild_node_a->data, DATA_NODE_SIZE);
    SHA1_Update(&ctx, pchild_node_b->data, DATA_NODE_SIZE);
    SHA1_Final(pfather_node_out->data, &ctx);
  } else _accumulation(pchild_node_a->data, pchild_node_b->data, pfather_node_out->data);
    
  //printf("produceMerkleLevelNode : %s\n", auxHelper::byte2hexstring(pfather_node_out->data, DATA_NODE_SIZE).c_str());
}

bool MerkleTree::merkleLevelNodeCheck(data_node * pchild_node_a, data_node * pchild_node_b, data_node * pfather_node, bool bhash)
{
  data_node test_node;
  bool rtCode = false;
  
  if (NULL == pchild_node_a || NULL == pchild_node_b || NULL == pfather_node) return rtCode;

  produceMerkleLevelNode(pchild_node_a, pchild_node_b, &test_node, bhash);
  rtCode = (0 == memcmp(test_node.data, pfather_node->data, DATA_NODE_SIZE));

  return rtCode;
}

void MerkleTree::_produceMerkleLevles(std::vector<data_node *> &low, std::vector<data_node *> &high, bool bhash)
{
  for (size_t i = 0, j = 0; i < low.size(); i += 2, j++)
    produceMerkleLevelNode(low[i], low[i + 1], high[j], bhash);
}

void MerkleTree::_accumulation(u_char *a, u_char *b, u_char *out, u_int64_t data_block_index)
{
  u_int64_t * p1 = (u_int64_t *)a, * p2 = (u_int64_t *)b, * p3 = (u_int64_t *)out;
  
  size_t number_of_sizeof_u_int64_t = (DATA_NODE_SIZE / sizeof(u_int64_t));
  for (size_t i = 0; i < number_of_sizeof_u_int64_t; i++) {
    p3[i] = p1[i] + p2[i] + data_block_index;
    //printf("%llu = %llu + %llu + %llu\n", p3[i], p1[i], p2[i], data_block_index);
  }
  
  for (size_t j = (number_of_sizeof_u_int64_t * sizeof(u_int64_t)); j < DATA_NODE_SIZE; j++) {
    out[j] = a[j] + b[j];
  }

  //printf("B%llu : %s\n\n", data_block_index, auxHelper::byte2hexstring(out, DATA_NODE_SIZE).c_str());
}

hireVolume::hireVolume()
{
  _stop_flag = true;
  _hire_status = HIREST_VOLUME_UNPACKAGE;
  _hire_package_volume_expect = 0;
  _hire_package_volume_actual = 0;
  _hire_package_volume_actual_write = 0;
  _remaining_time = 0;
  _volume_package_filename_prefix = "";
  _volume_package_hire_blockchain_number_hash = "";
  _volume_package_hire_blockchain_number = "";
  _volume_package_hire_blockchain_nonce = "";
  _stop_volume_package_flag = true;
  _hirevolume_write_msg_pool.setMaxBlockCount(3);
  _queue_size = 0;
  _hire_address = "";
  _volume_poc = "";

  _init_locker_hire_rent();
  _init_locker_hire_poc_group();
}

hireVolume::~hireVolume()
{
  _clear_hire_rent_items();

  _destroy_locker_hire_rent();
  _destroy_locker_hire_poc_group();
}

bool hireVolume::start()
{
  _stop_flag = false;
  _stop_volume_package_flag = false;

  _hire_volume_event.event_init("hire_volume_event");
  _hirevolume_write_event.event_init("hirevolume_write_event");
  _hirevolume_write_free_event.event_init("hirevolume_write_free_event");

  _load_volume_status();

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "create hire volume handle worker thread.");
  if (false == _hirevolume_handle_thread.thread_worker_start(hireVolume::thread_hirevolume_handle_worker, this)) {
    _stop_flag = true;
    debugEntry(LL_ERROR, LOG_MODULE_INDEX_HIRE, "create hire volume handle worker thread fail.");
    return false;
  }

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "create hire volume write handle worker thread.");
  if (false == _hirevolume_write_handle_thread.thread_worker_start(hireVolume::thread_hirevolume_write_handle_worker, this)) {
    _stop_flag = true;
    debugEntry(LL_ERROR, LOG_MODULE_INDEX_HIRE, "create hire volume write handle worker thread fail.");
    return false;
  }

  return true;
}

void hireVolume::stop()
{
  _stop_flag = true;

  _hire_volume_event.event_notify();
  _hirevolume_write_event.event_notify();
  _hirevolume_write_free_event.event_notify();
}

void hireVolume::wait_stop()
{  
  _hirevolume_handle_thread.wait_thread_stop();
  _hirevolume_write_handle_thread.wait_thread_stop();

  _hire_volume_event.event_close();
  _hirevolume_write_event.event_close();
  _hirevolume_write_free_event.event_close();
}

std::string hireVolume::getMD5(const std::string &data, u_char * pbuffer)
{
  u_char *pmd5_buffer = NULL;
  u_char md5_buffer[16] = {0};

  if (NULL == pbuffer) pmd5_buffer = md5_buffer;
  else pmd5_buffer = pbuffer;

  MD5_CTX ctx;
  //B0 = Hash(Data)
  MD5_Init(&ctx);
  MD5_Update(&ctx, data.c_str(), data.length());
  MD5_Final(pmd5_buffer, &ctx);

  return auxHelper::byte2hexstring(pmd5_buffer, 16);
}

void *hireVolume::thread_hirevolume_handle_worker(void *param)
{
  hireVolume *_this = (hireVolume *) param;
  _this->hirevolume_worker();
  return NULL;
}

void *hireVolume::thread_hirevolume_write_handle_worker(void *param)
{
  hireVolume *_this = (hireVolume *) param;
  _this->hirevolume_write_worker();
  return NULL;
}

void hireVolume::notify_hireVolume(const std::string &message)
{
  bool bempty;
  _hirevolume_msg_queue.add_to_queue(message, bempty);
  _hire_volume_event.event_notify();
}

void hireVolume::_notify_hireVolume_write(u_char *pbuffer_write, WRITE_ACTION_TYPE write_action_type, std::string filename)
{
  bool bempty;
  WRITE_MESSAGE write_message;

  write_message.pbuffer_write = pbuffer_write;
  write_message.write_action_type = write_action_type;
  write_message.filename = filename;
  
  if (_queue_size < _hirevolume_write_msg_queue.size()) {
    _queue_size = _hirevolume_write_msg_queue.size();
    printf("_queue_size : %u\n", _queue_size);
  }

  _hirevolume_write_msg_queue.add_to_queue(write_message, bempty);

  if (bempty) _hirevolume_write_event.event_notify();
}

bool hireVolume::is_volume_packaged(HIRE_STATUS hire_status)
{
  if (HIREST_VOLUME_PACKAGED == hire_status || HIREST_VOLUME_IN_SERVICE == hire_status) return true;
  else return false;
}

CN_ERR hireVolume::do_volume_package_poc(FJson::Value &root)
{
  return _do_volume_package_poc(get_hire_package_volume_free(), root);
}

CN_ERR hireVolume::_do_volume_package_poc(u_int64_t hire_package_volume_free, FJson::Value &root)
{
  bool rtCode = false;
  u_int64_t random_node_index = 0;
  std::string hire_volume_package_poc;

  if (0 == hire_package_volume_free) {
    root.clear();
    root["utg_block"] = _volume_package_hire_blockchain_number;
    root["utg_nonce"] = _volume_package_hire_blockchain_nonce;
    root["utg_block_hash"] = _volume_package_hire_blockchain_number_hash;
    root["utg_package_poc"] = hire_volume_package_poc;
    root["utg_volume"] = auxHelper::intBytesToGBStr(_hire_package_volume_expect);
    root["utg_volume_free"] = auxHelper::intBytesToGBStr(hire_package_volume_free);
    return CE_SUCC;
  }

  std::string hire_block = auxHelper::strToDecimalStr(root["utg_block"].asString());
  std::string hire_nonce = auxHelper::strToDecimalStr(root["utg_nonce"].asString());
  std::string hire_block_hash = root["utg_block_hash"].asString();
  std::transform(hire_block_hash.begin(), hire_block_hash.end(), hire_block_hash.begin(), ::tolower);

  random_node_index = _make_volume_package_poc_random_node_index(hire_package_volume_free, hire_block, hire_nonce, hire_block_hash);
  _make_volume_package_poc_head(hire_package_volume_free, random_node_index, hire_block, hire_nonce, hire_block_hash, hire_volume_package_poc);

  MerkleTree free_merkletree_topmost;
  if (hire_package_volume_free < _hire_package_volume_expect) {
    rtCode = _load_free_topmost_merkletree(hire_package_volume_free, free_merkletree_topmost);
    if (!rtCode) return CE_MEMERY_OUT;
  }
  MerkleTree &merkletree_topmost = (hire_package_volume_free < _hire_package_volume_expect) ? free_merkletree_topmost : _merkletree_topmost;
  rtCode = _get_volume_package_poc(random_node_index, merkletree_topmost, hire_volume_package_poc);
  if (rtCode) {
    root.clear();
    root["utg_block"] = _volume_package_hire_blockchain_number;
    root["utg_nonce"] = _volume_package_hire_blockchain_nonce;
    root["utg_block_hash"] = _volume_package_hire_blockchain_number_hash;
    root["utg_package_poc"] = hire_volume_package_poc;
    root["utg_volume"] = auxHelper::intBytesToGBStr(_hire_package_volume_expect);
    root["utg_volume_free"] = auxHelper::intBytesToGBStr(hire_package_volume_free);
  } else return CE_FAIL;
  
  return CE_SUCC;
}

CN_ERR hireVolume::do_volume_rent(FJson::Value &root)
{
  std::string hire_rent_voucher = root["voucher"].asString();
  std::string hire_block_number = auxHelper::strToDecimalStr(root["utg_block"].asString());
  std::string hire_rent_addr = root["rentAddr"].asString();
  u_int64_t hire_rent_time = root["rentTime"].asUInt();

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hire rent items size : %u", _hire_rent_items.size());

  std::map<std::string, HIRE_RENT_ITEM>::iterator it;
  lock_hire_rent();
  it = _hire_rent_items.find(hire_rent_voucher);
  unlock_hire_rent();

  if (it != _hire_rent_items.end()) {
    u_int64_t hire_pre_block_number = strtoll(it->second.hire_block_number.c_str(), NULL, 10);
    u_int64_t hire_cur_block_number = strtoll(hire_block_number.c_str(), NULL, 10); 
    if (false == it->second.bhire_rent_onpledge && hire_cur_block_number > hire_pre_block_number)
      _del_hire_rent_item(hire_rent_voucher);
    else return CE_OPERATION_DENIED;
  }

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "%s is not find in hire rent items.", hire_rent_voucher.c_str());

  std::string volume_GB_str = root["volume"].asString();
  u_int64_t volume_GB = atoll(volume_GB_str.c_str());
  u_int64_t rent_volume_bytes = volume_GB * 1024 * 1024 * 1024LL;
  u_int64_t hire_package_volume_free = get_hire_package_volume_free();
  if (0 == rent_volume_bytes || hire_package_volume_free < rent_volume_bytes) return CE_OPERATION_DENIED;
  
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "%s rent %llu Bytes from free %llu Bytes .",  hire_rent_voucher.c_str(), rent_volume_bytes, hire_package_volume_free);

  if (_has_rent_item_outpledge()) return CE_STATUS;

  char szbuffer[128] = {0};
  sprintf(szbuffer, "%llu", rent_volume_bytes);
  std::string hire_rent_volume = std::string(szbuffer);

  u_int64_t hire_rent_index_begin = (hire_package_volume_free - rent_volume_bytes) / DATA_NODE_SIZE;
  u_int64_t hire_rent_index_end = (hire_package_volume_free / DATA_NODE_SIZE) - 1;
  bzero(szbuffer, 128);
  sprintf(szbuffer, "%llu-%llu", hire_rent_index_begin, hire_rent_index_end);
  std::string hire_rent_index = std::string(szbuffer);

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "%s hire rent index : %s.", hire_rent_voucher.c_str(), hire_rent_index.c_str());

  bool rtCode = _add_hire_rent_item(hire_block_number, hire_rent_voucher, hire_rent_volume, hire_rent_index, hire_rent_addr, hire_rent_time, false, true, false);
  if (rtCode) {
    hire_package_volume_free -= rent_volume_bytes;

    //rent poc
    std::string hire_volume_rent_poc;
    FJson::Value root_temp = root;

    lock_hire_rent();
    it = _hire_rent_items.find(hire_rent_voucher);
    unlock_hire_rent();

    if (it != _hire_rent_items.end()) {
      _do_volume_rent_poc(it->second, root_temp);
      hire_volume_rent_poc = root_temp["utg_rent_poc"].asString();
    }

    //free 
    root_temp.clear();
    root_temp = root;
    _do_volume_package_poc(hire_package_volume_free, root_temp);
    root_temp["utg_rent_poc"] = hire_volume_rent_poc;

    root.clear();
    root = root_temp;

    _save_volume_status();

    hireLogManager::instance().add_volume_rent_log(HIRE_LOG_ACT_RENT, hire_rent_voucher, hire_block_number, volume_GB_str, auxHelper::intBytesToGBStr(hire_package_volume_free));

  } else return CE_OPERATION_DENIED;

  return CE_SUCC;
}

CN_ERR hireVolume::do_volume_rent_poc(FJson::Value &root)
{
  CN_ERR rtCode = CE_SUCC;
  std::string voucher = root["voucher"].asString();

  do
  {
    std::map<std::string, HIRE_RENT_ITEM>::iterator it;
    lock_hire_rent();
    it = _hire_rent_items.find(voucher);
    unlock_hire_rent();
    if (it == _hire_rent_items.end()) {
      rtCode = CE_DEVICE_NOTEXISTS;
      break;
    }

    if (false == it->second.bhire_rent_onpledge) {
      rtCode = CE_STATUS;
      break;
    }

    FJson::Value root_temp = root;
    rtCode = _do_volume_rent_poc(it->second, root_temp);
    if (CE_SUCC != rtCode) break;
    std::string hire_volume_rent_poc = root_temp["utg_rent_poc"].asString();

    //free 
    root_temp.clear();
    root_temp = root;
    rtCode = _do_volume_package_poc(get_hire_package_volume_free(), root_temp);
    root_temp["utg_rent_poc"] = hire_volume_rent_poc;

    root.clear();
    root = root_temp;

  } while (false);

  return rtCode;
}

void hireVolume::do_volume_rent_volume_partitioning(const std::string &hire_rent_index)
{
  u_int64_t rent_index_begin = 0, rent_index_end = 0;
  _get_node_index_from_str(hire_rent_index, rent_index_begin, rent_index_end);

  u_int64_t data_node_number = rent_index_end - rent_index_begin + 1;
  u_int number_of_files_begin = rent_index_begin / HIRE_VOLUME_PACKAGE_FILE_NODE_NUM;
  u_int number_of_files = data_node_number / HIRE_VOLUME_PACKAGE_FILE_NODE_NUM;
  
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "do_volume_rent_volume_partitioning delete (index %s) files [%u, %u)", hire_rent_index.c_str(), number_of_files_begin, (number_of_files_begin + number_of_files));
  for (u_int64_t i = number_of_files_begin; i < (number_of_files_begin + number_of_files); i++) {
    std::string file_name = _get_volume_package_file_name(i);
    if (0 == access(file_name.c_str(), R_OK)) unlink(file_name.c_str());
     debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "do_volume_rent_volume_partitioning delete file : %s", file_name.c_str());
  }
}

CN_ERR hireVolume::_do_volume_rent_poc(HIRE_RENT_ITEM &hire_rent_item, FJson::Value &root)
{
  bool rtCode = false;

  std::string hire_block = auxHelper::strToDecimalStr(root["utg_block"].asString());
  std::string hire_nonce = auxHelper::strToDecimalStr(root["utg_nonce"].asString());
  std::string hire_block_hash = root["utg_block_hash"].asString();
  std::transform(hire_block_hash.begin(), hire_block_hash.end(), hire_block_hash.begin(), ::tolower);
  
  std::string hire_volume_rent_poc;
  u_int64_t random_node_index = _make_volume_rent_poc_random_node_index(hire_rent_item.hire_rent_index, hire_block, hire_nonce, hire_block_hash);
  _make_volume_rent_poc_head(hire_rent_item, random_node_index, hire_block, hire_nonce, hire_block_hash, hire_volume_rent_poc);
  rtCode = _get_volume_rent_poc(hire_rent_item, random_node_index, hire_volume_rent_poc);

  if (rtCode) {
    root.clear();
    root["utg_rent_poc"] = hire_volume_rent_poc;
  } else return CE_FAIL;
  
  return CE_SUCC;
}

void hireVolume::get_all_rent_items(FJson::Value &rent_items, bool bvolumeToGB)
{
  std::map<std::string, HIRE_RENT_ITEM>::iterator it;

  lock_hire_rent();
  for (it = _hire_rent_items.begin(); it != _hire_rent_items.end(); it++) {
    FJson::Value hire_rent_item;

    hire_rent_item["utg_rent_addr"] = it->second.hire_rent_addr;
    hire_rent_item["utg_rent_time"] = it->second.hire_rent_time;
    hire_rent_item["utg_rent_voucher"] = it->second.hire_rent_voucher;
    if (bvolumeToGB) {
      u_int64_t rent_volume_Bytes = strtoll(it->second.hire_rent_volume.c_str(), NULL, 10);
      u_int64_t rent_volume_GB = rent_volume_Bytes / ( 1024 * 1024 * 1024LL );

      char szbuffer[128] = {0};
      sprintf(szbuffer, "%llu", rent_volume_GB);
      hire_rent_item["utg_rent_volume"] = std::string(szbuffer);
    } else hire_rent_item["utg_rent_volume"] = it->second.hire_rent_volume;
    hire_rent_item["utg_rent_index"] = it->second.hire_rent_index;
    hire_rent_item["utg_block_number"] = it->second.hire_block_number;
    hire_rent_item["utg_rent_onpledge"] = it->second.bhire_rent_onpledge;
    hire_rent_item["utg_need_retrieve_by_manual"] = it->second.bneed_retrieve_by_manual;
    hire_rent_item["utg_volume_partitioning"] = it->second.balready_volume_partitioning;
    
    rent_items.push_back(hire_rent_item);
    
  }
  unlock_hire_rent();
}

CN_ERR hireVolume::do_volume_retrieve(const std::string &hire_rent_voucher, const std::string &hire_block_number)
{
  bool rtCode = false;
  u_int64_t rent_volume_bytes = 0;

  std::map<std::string, HIRE_RENT_ITEM>::iterator it;
  lock_hire_rent();
  it = _hire_rent_items.find(hire_rent_voucher);
  unlock_hire_rent();
  if (it == _hire_rent_items.end()) return CE_DEVICE_NOTEXISTS;

  u_int64_t hire_rent_block_number = strtoll(it->second.hire_block_number.c_str(), NULL, 10);
  u_int64_t hire_retrieve_block_number = strtoll(hire_block_number.c_str(), NULL, 10);
  if (false == it->second.bhire_rent_onpledge && it->second.bneed_retrieve_by_manual && hire_retrieve_block_number > hire_rent_block_number) {
    rtCode = _del_hire_rent_item(hire_rent_voucher);
    if (rtCode) _save_volume_status();
  } else return CE_OPERATION_DENIED;

  return CE_SUCC;
}

void hireVolume::update_rent_item_volume_partitioning(const std::string &hire_rent_voucher)
{
  std::map<std::string, HIRE_RENT_ITEM>::iterator it;

  lock_hire_rent();
  it = _hire_rent_items.find(hire_rent_voucher);
  if (it != _hire_rent_items.end()) it->second.balready_volume_partitioning = true;
  unlock_hire_rent();
}

u_int64_t hireVolume::get_hire_package_volume_free()
{
  u_int64_t hire_package_volume_free = _hire_package_volume_expect;

  lock_hire_rent();
  std::map<std::string, HIRE_RENT_ITEM>::iterator it;
  for (it = _hire_rent_items.begin(); it != _hire_rent_items.end(); it++) {
    if (true == it->second.bhire_rent_onpledge) {
      u_int64_t rent_volume_Bytes = strtoll(it->second.hire_rent_volume.c_str(), NULL, 10);
      hire_package_volume_free -= rent_volume_Bytes;
    }
  }
  unlock_hire_rent();

  return hire_package_volume_free;
}


void hireVolume::hirevolume_worker()
{
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hirevolume_worker begin.");
  
  std::string hire_volume_message = "";

  _check_volume_package_status_when_start();

  while (!_stop_flag) {
    hire_volume_message = _hirevolume_msg_queue.get_from_queue();
    if (!hire_volume_message.empty()) {
      _process_volume_message(hire_volume_message);
    } else {
      int event_rtCode = _hire_volume_event.event_timewait(HIRE_VOLUME_EVENT_TIMEOUT);
      if (ETIMEDOUT == event_rtCode) {
        
      }
    }
  }

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hirevolume_worker end.");
}

void hireVolume::hirevolume_write_worker()
{
  WRITE_MESSAGE write_message;
  u_int64_t data_level_block_bytes = MerkleTree::getLevelBufferSize(DATA_MERKLE_TREE_DEPTH_INDEX);
  int fd = -1;

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hirevolume_write_worker begin.");

  while (!_stop_flag) {
    write_message.pbuffer_write = NULL;
    write_message.write_action_type = HIREWAY_NONE;
    write_message.filename = "";
    
    write_message = _hirevolume_write_msg_queue.get_from_queue();
    if (NULL !=  write_message.pbuffer_write) {
      if (HIREWAY_CW == write_message.write_action_type) {
        fd = open(write_message.filename.c_str(), O_RDWR | O_CREAT);
        if (-1 == fd) {
          auxHelper::sleep_time_with_cancle(30, &_stop_flag);
          fd = open(write_message.filename.c_str(), O_RDWR | O_CREAT);
          if (-1 == fd) break;
        }
      }

      ssize_t bytes_write = 0;
      while (bytes_write != data_level_block_bytes && !_stop_flag && !_stop_volume_package_flag) {
        ssize_t once_bytes_write = write(fd, write_message.pbuffer_write + bytes_write, data_level_block_bytes - bytes_write);
        if (0 < once_bytes_write) bytes_write += once_bytes_write;
      }
      _hire_package_volume_actual_write += data_level_block_bytes;
      _hirevolume_write_msg_pool.msg_free(write_message.pbuffer_write);
      _hirevolume_write_free_event.event_notify();

      if (HIREWAY_WC == write_message.write_action_type) {
        //fsync(fd);
        close(fd);
        //_notify_hireVolume_fsync(fd);
      }
    } else {
      int event_rtCode = _hirevolume_write_event.event_timewait(HIRE_VOLUME_EVENT_TIMEOUT);
      if (ETIMEDOUT == event_rtCode) {
        
      } 
    }
  }

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hirevolume_write_worker end.");
}

void hireVolume::_process_volume_message(const std::string &message)
{
  FJson::Parser parser;
  FJson::Value root;

  if (!parser.load_string(message, root)) return;

  switch (root["cmd"].asUInt()) {
  case HIRECMD_VOLUME_PACKAGE:
    _do_volume_package(root);
    break;
  case HIRECMD_VOLUME_RESET:
    _do_volume_reset();
    break;
  case  HIRECMD_VOLUME_PACKAGE_CONTINUE:
    _do_volume_package_continue();
    break;
  default:
    break;
  }
}

void hireVolume::_do_volume_package(FJson::Value &root)
{  
  std::string volume_GB_str = root["volume"].asString();
  u_int64_t volume_GB = atoll(volume_GB_str.c_str());
  
  _hire_package_volume_expect = volume_GB * 1024 * 1024 * 1024LL;
  _hire_package_volume_actual = 0;
  if (0 == volume_GB || _hire_package_volume_expect < HIRE_VOLUME_PACKAGE_FILE_SIZE) return;
  
  MerkleTree merkletree_data;
  merkletree_data.initMerkleTree(DATA_MERKLE_TREE_DEPTH_INDEX + 1);
  
  _volume_package_hire_blockchain_number = auxHelper::strToDecimalStr(root["utg_block"].asString());
  _volume_package_hire_blockchain_nonce = auxHelper::strToDecimalStr(root["utg_nonce"].asString());
  std::string volume_package_hire_blockchain_number_hash = root["utg_block_hash"].asString();
  std::transform(volume_package_hire_blockchain_number_hash.begin(), volume_package_hire_blockchain_number_hash.end(), volume_package_hire_blockchain_number_hash.begin(), ::tolower);
  _volume_package_hire_blockchain_number_hash = volume_package_hire_blockchain_number_hash;

  std::string hire_address = root["utg_address"].asString();
  std::string hire_B0_data = _volume_package_hire_blockchain_number + _volume_package_hire_blockchain_nonce + _volume_package_hire_blockchain_number_hash + hire_address;
  merkletree_data.setZeroNode((u_char *)hire_B0_data.c_str(), hire_B0_data.length());
  
  hireLogManager::instance().add_volume_package_log(HIRE_LOG_ACT_BEGIN_PACKAGE, _volume_package_hire_blockchain_number, volume_GB_str);

  _volume_package(merkletree_data);
}

void hireVolume::_do_volume_package_continue()
{
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume package CONTINUE");

  if (_hire_package_volume_actual == _hire_package_volume_expect) return;

  MerkleTree merkletree_data;
  merkletree_data.initMerkleTree(DATA_MERKLE_TREE_DEPTH_INDEX + 1);

  _read_volume_package_file(_get_volume_package_file_name(0), merkletree_data.getZeroNode()->data, DATA_NODE_SIZE, 0);
  
  if (HIRE_VOLUME_PACKAGE_FILE_SIZE <= _hire_package_volume_actual) {
    u_int64_t data_level_block_bytes = MerkleTree::getLevelBufferSize(DATA_MERKLE_TREE_DEPTH_INDEX); 
    u_int64_t number_of_data_level_block_need_to_produce_complete = _hire_package_volume_actual / data_level_block_bytes; 
    u_int number_of_files_complete = number_of_data_level_block_need_to_produce_complete / HIRE_VOLUME_PACKAGE_FILE_WRITE_TIMES;
    _read_volume_package_file_to_merkletree_datalevel(_get_volume_package_file_name(number_of_files_complete - 1), merkletree_data, ((HIRE_VOLUME_PACKAGE_FILE_WRITE_TIMES - 1) * data_level_block_bytes));
    u_int64_t data_node_number_complete = _hire_package_volume_actual / DATA_NODE_SIZE;
    merkletree_data.setCurDataBlcokIndex(data_node_number_complete);

    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume package continue from node %llu", data_node_number_complete);
  } else { 
    _hire_package_volume_actual = 0;
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume package continue from node 0");
  }
  
  hireLogManager::instance().add_volume_package_log(HIRE_LOG_ACT_CONTINUE_PACKAGE, _volume_package_hire_blockchain_number, auxHelper::intBytesToGBStr(_hire_package_volume_actual));

  _volume_package(merkletree_data);
}

void hireVolume::_volume_package(MerkleTree &merkletree_data)
{
  u_int64_t data_level_block_bytes = MerkleTree::getLevelBufferSize(DATA_MERKLE_TREE_DEPTH_INDEX); 
  u_int64_t number_of_data_level_block_need_to_produce = _hire_package_volume_expect / data_level_block_bytes; 
  u_int64_t number_of_data_level_block_need_to_produce_complete = _hire_package_volume_actual / data_level_block_bytes;
  
  u_int64_t volume_GB = _hire_package_volume_expect / (1024 * 1024 * 1024LL);
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume package : %llu(%u GB), data_level_block_bytes %llu", _hire_package_volume_expect, volume_GB, data_level_block_bytes);
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume package bytes %llu/%llu", _hire_package_volume_actual, _hire_package_volume_expect);

  _load_topmost_merkletree();
  _merkletree_topmost.setCurDataBlcokIndex(number_of_data_level_block_need_to_produce_complete);
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume package from block %llu", number_of_data_level_block_need_to_produce_complete);

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume package begin.");
  _hire_status = HIREST_VOLUME_PACKAGING;
  
  data_block *pdata_block_buffer = NULL;
  u_char *pdata_buffer = (u_char *)merkletree_data.getDataLevelFirstNode();
  u_int number_of_files = number_of_data_level_block_need_to_produce / HIRE_VOLUME_PACKAGE_FILE_WRITE_TIMES;
  u_int number_of_files_begin = number_of_data_level_block_need_to_produce_complete / HIRE_VOLUME_PACKAGE_FILE_WRITE_TIMES;
  pdata_block_buffer = _hirevolume_write_msg_pool.msg_alloc();
  
  _remaining_time = (number_of_files - number_of_files_begin) * 40;
  _stop_volume_package_flag = false;
  _hire_package_volume_actual_write = _hire_package_volume_actual;
  u_int64_t index_of_blocks = 0;
  bool topmost_start_hash = false;
  for (size_t i = number_of_files_begin; i < number_of_files && !_stop_flag && !_stop_volume_package_flag; i++) {
    std::string file_name = _get_volume_package_file_name(i);
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "produce volume package file (%u/%u):%s.", i, number_of_files, file_name.c_str());
    
    time_t ts_start = time(NULL);
    for (size_t j = 0; j < HIRE_VOLUME_PACKAGE_FILE_WRITE_TIMES && !_stop_flag && !_stop_volume_package_flag; j++) {
      //
      WRITE_ACTION_TYPE write_action_type = HIREWAY_NONE;
      if (0 == j) write_action_type = HIREWAY_CW;
      else if ((HIRE_VOLUME_PACKAGE_FILE_WRITE_TIMES - 1) == j) write_action_type = HIREWAY_WC;
      else write_action_type = HIREWAY_W;

      merkletree_data.produceDataLevel();
      topmost_start_hash = merkletree_data.produceMerkleLevles();
      while (!_stop_flag && !_stop_volume_package_flag) {
        pdata_block_buffer = _hirevolume_write_msg_pool.msg_alloc();
        if (NULL != pdata_block_buffer) {
          memcpy(pdata_block_buffer->data, pdata_buffer, DATA_BLOCK_SIZE);
          _notify_hireVolume_write((u_char *)pdata_block_buffer, write_action_type, file_name);
          break;
        } else  {
          _hirevolume_write_free_event.event_timewait(1); 
          //debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "hirevolume write pool block count : %u/%u", _hirevolume_write_msg_pool.getCurBolckCount(), _hirevolume_write_msg_pool.getMaxBlockCount());
        }
      }

      _merkletree_topmost.addDataLevelNode(merkletree_data.getMerkleTreeRootNode());
      //debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "    merkle root node : %s     %llu", auxHelper::byte2hexstring((u_char *)merkletree_data.getMerkleTreeRootNode(), DATA_NODE_SIZE).c_str(), index_of_blocks);
      index_of_blocks++;
    }

    if (_stop_flag || _stop_volume_package_flag) break;

    if (0 == i % 20) {
      _save_volume_status();
      _save_merkletree_to_file(HIREFS_VOLUME_PACKAGE_TOPMOST_MERKLETREE_FILE_NAME, _merkletree_topmost);
    }

    _remaining_time = ((time(NULL) - ts_start) * (number_of_files - i - 1));
    _hire_package_volume_actual = ((i + 1) * HIRE_VOLUME_PACKAGE_FILE_SIZE);
    
    u_int completion = 100 * ((_hire_package_volume_actual * 1.0) / (_hire_package_volume_expect * 1.0));
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "POC completion %u% remaining_time %u sec.", completion, _remaining_time);
  }

  while (!_stop_flag && !_stop_volume_package_flag && _hire_package_volume_actual_write != _hire_package_volume_actual) {
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "wait write package volume file complete ...");
    auxHelper::sleep_time_with_cancle(3, &_stop_flag);
  }
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "write package volume file completed.");
  
  if (_hire_package_volume_actual == _hire_package_volume_expect) {
    _merkletree_topmost.produceMerkleLevles(topmost_start_hash);
    //debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Topmost merkle data-frist node : %s", auxHelper::byte2hexstring((u_char *)_merkletree_topmost.getDataLevelFirstNode(), DATA_NODE_SIZE).c_str());
    //debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Topmost merkle data-last node : %s", auxHelper::byte2hexstring((u_char *)_merkletree_topmost.getDataLevelLastNode(), DATA_NODE_SIZE).c_str());
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Topmost merkle root node : %s", auxHelper::byte2hexstring((u_char *)_merkletree_topmost.getMerkleTreeRootNode(), DATA_NODE_SIZE).c_str());
    _save_merkletree_to_file(HIREFS_VOLUME_PACKAGE_TOPMOST_MERKLETREE_FILE_NAME, _merkletree_topmost);
    
    sync();
    
    hireLogManager::instance().add_volume_package_log(HIRE_LOG_ACT_END_PACKAGE, _volume_package_hire_blockchain_number, auxHelper::intBytesToGBStr(_hire_package_volume_expect));

    _hire_status = HIREST_VOLUME_PACKAGED;
  } else _hire_status = HIREST_VOLUME_UNPACKAGE;

  _save_volume_status();

  _hirevolume_write_msg_pool.clean_msg_pool();
  
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume package end(%s).", (_hire_status == HIREST_VOLUME_PACKAGED) ? "succ" : "fail");
}

void hireVolume::_do_volume_reset()
{
  hireLogManager::instance().add_volume_package_log(HIRE_LOG_ACT_RESET_PACKAGE, _volume_package_hire_blockchain_number, auxHelper::intBytesToGBStr(_hire_package_volume_expect));

  _stop_volume_package_flag = true;
  _hire_status = HIREST_VOLUME_RESETING;

  if (!_volume_package_filename_prefix.empty()) {
    char dir_name[256] = {0};
    sprintf(dir_name, "%s/%s", HIREFS_VOLUME_DIRECTORY, _volume_package_filename_prefix.c_str());
    struct stat st;
    if (0 == stat(dir_name, &st)) auxHelper::System("rm -rf %s", dir_name);
  }

  if (0 == access(HIREFS_VOLUME_STATUS_FILE_NAME, R_OK))
    unlink(HIREFS_VOLUME_STATUS_FILE_NAME);

  if (0 == access(HIREFS_VOLUME_PACKAGE_TOPMOST_MERKLETREE_FILE_NAME, R_OK))
    unlink(HIREFS_VOLUME_PACKAGE_TOPMOST_MERKLETREE_FILE_NAME);

  if (0 == access(HIREFS_VOLUME_PACKAGE_POC_COMMIT_HISTORY, R_OK))
    unlink(HIREFS_VOLUME_PACKAGE_POC_COMMIT_HISTORY);
    
  _merkletree_topmost.unInitMerkleTree();

  _hire_status = HIREST_VOLUME_UNPACKAGE;
  _volume_package_hire_blockchain_number = "";
  _volume_package_hire_blockchain_nonce = "";
  _volume_package_hire_blockchain_number_hash = "";
  _hire_package_volume_expect = 0;
  _hire_package_volume_actual = 0;
  _volume_poc = "";
  
  _clear_hire_poc_groups();
  _clear_hire_rent_items();
  _save_volume_status();
}

std::string hireVolume::_get_volume_package_file_name(u_int index)
{
  char dir_name[256] = {0};
  char file_name[256] = {0};

  sprintf(dir_name, "%s/%s", HIREFS_VOLUME_DIRECTORY, _volume_package_filename_prefix.c_str());
  struct stat st;
  if (0 != stat(dir_name, &st)) mkdir(dir_name, S_IWUSR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

  sprintf(file_name, "%s/%s_%05lu", dir_name, _volume_package_filename_prefix.c_str(), index);

  return std::string(file_name);
}

bool hireVolume::_read_volume_package_file_to_merkletree_datalevel(const std::string &file_name, MerkleTree &merkletree, ssize_t seek)
{
  u_int data_level_block_bytes = MerkleTree::getLevelBufferSize(merkletree.getDepth() - 1);
  u_char *pdata_buffer = (u_char *)merkletree.getDataLevelFirstNode();

  return _read_volume_package_file(file_name, pdata_buffer, data_level_block_bytes, seek);
}

bool hireVolume::_read_volume_package_file(const std::string &file_name, u_char *pbuffer, u_int64_t buffer_len, ssize_t seek)
{
  if (NULL == pbuffer || 0 == buffer_len) return false;

  FILE *fp = fopen(file_name.c_str(), "rb");
  if (NULL == fp) return false;

  if (0 != seek) fseek(fp, seek, SEEK_SET);

  ssize_t bytes_read = 0;
  while (bytes_read != buffer_len) {
    bytes_read += fread(pbuffer + bytes_read, 1, buffer_len - bytes_read, fp);
    if (0 != feof(fp)) break;
  }

  if (NULL != fp) fclose(fp);

  return true;
}

bool hireVolume::_save_merkletree_to_file(const std::string &file_name, MerkleTree &merkletree)
{
  u_int data_level_block_bytes = merkletree.getBufferSize();
  u_char *pdata_buffer = (u_char *)merkletree.getBuffer();

  if (NULL == pdata_buffer || 0 == data_level_block_bytes) return false;

  FILE *fpwrite = fopen(file_name.c_str(), "wb");
  if (NULL == fpwrite) return false;

  ssize_t bytes_write = 0;
  while (bytes_write != data_level_block_bytes) {
    bytes_write += fwrite(pdata_buffer + bytes_write, 1, data_level_block_bytes - bytes_write, fpwrite);
  }

  if (NULL != fpwrite) fclose(fpwrite);

  return true;
}

bool hireVolume::_load_merkletree_from_file(const std::string &file_name, MerkleTree &merkletree)
{
  u_int buffer_size = merkletree.getBufferSize();
  u_char *pdata_buffer = (u_char *)merkletree.getBuffer();

  return _read_volume_package_file(file_name, pdata_buffer, buffer_size, 0);
}

bool hireVolume::_load_volume_status()
{
  /*
  {
    "utg_block":"111111",
    "utg_block_hash":"uxb7cef0165d6f99bb368931a525421a052706c0c5d5502cf8ee1b161cf6f4df02",
    "utg_nonce":"222222",
    "utg_package_volume_actual":"16106127360",
    "utg_package_volume_expect":"16106127360",
    "utg_rent_items":[
      {
        "utg_rent_index":"536870912-805306367",
        "utg_rent_volume":"5368709120",
        "utg_rent_voucher":"ux56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
        "utg_block_number": "12324",
        "utg_rent_addr":"ux56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
        "utg_rent_time": 30,
        "utg_rent_onpledge": true
      },
      {
        "utg_rent_index":"268435456-536870911",
        "utg_rent_volume":"5368709120",
        "utg_rent_voucher":"ux57754379cebe357bd58e67739a1248c9c295432f926f7f2e77bb87d73eb1476e",
        "utg_block_number": "3214",
        "utg_rent_addr":"ux56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
        "utg_rent_time": 30,
        "utg_rent_onpledge": false
      }
    ],
    "utg_status":3
  }
  */
  std::string hire_status = "";
  FJson::Value root, hire_rent_items;
  FJson::Parser parser;

  if (false == auxHelper::read_from_file(HIREFS_VOLUME_STATUS_FILE_NAME, hire_status)) {
    debugEntry (LL_WARN, LOG_MODULE_INDEX_HIRE, "read hire volume stauts file(%s) fail.", HIREFS_VOLUME_STATUS_FILE_NAME);
    return false;
  }

  if (!hire_status.empty()) {
    if (parser.load_string(hire_status, root)) {
      debugEntry (LL_DEBUG, LOG_MODULE_INDEX_HIRE, "load hire volume status %s.", root.toCompatString().c_str());

      _hire_status = (HIRE_STATUS)root["utg_status"].asUInt();
      _volume_package_hire_blockchain_number = root["utg_block"].asString();
      _volume_package_hire_blockchain_nonce = root["utg_nonce"].asString();
      _volume_package_hire_blockchain_number_hash = root["utg_block_hash"].asString();

      _hire_package_volume_expect = (u_int64_t)atoll(root["utg_package_volume_expect"].asString().c_str());
      _hire_package_volume_actual = (u_int64_t)atoll(root["utg_package_volume_actual"].asString().c_str());
      
      if (root.isMember("utg_volume_poc")) _volume_poc = root["utg_volume_poc"].asString();

      if (0 != _hire_package_volume_expect && _hire_package_volume_expect == _hire_package_volume_actual) {
        hire_rent_items = root["utg_rent_items"];
        if(hire_rent_items.isArray() && 0 < hire_rent_items.size()) {
          for (size_t i = 0; i < hire_rent_items.size(); i++) {
            std::string hire_rent_voucher = hire_rent_items[i]["utg_rent_voucher"].asString();
            std::string hire_rent_volume = hire_rent_items[i]["utg_rent_volume"].asString();
            std::string hire_rent_index = hire_rent_items[i]["utg_rent_index"].asString();
            std::string hire_block_number = hire_rent_items[i]["utg_block_number"].asString();
            std::string hire_rent_addr = hire_rent_items[i]["utg_rent_addr"].asString();
            u_int64_t hire_rent_time = hire_rent_items[i]["utg_rent_time"].asUInt();
            bool bhire_rent_onpledge = hire_rent_items[i]["utg_rent_onpledge"].asBool();
            bool balready_volume_partitioning = hire_rent_items[i]["utg_volume_partitioning"].asBool();
            
            debugEntry (LL_DEBUG, LOG_MODULE_INDEX_HIRE, "load hire package volume rent : %s, %s Bytes, %s.", hire_rent_voucher.c_str(), hire_rent_volume.c_str(), hire_rent_index.c_str());
            
            _add_hire_rent_item(hire_block_number, hire_rent_voucher, hire_rent_volume, hire_rent_index, hire_rent_addr, hire_rent_time, bhire_rent_onpledge, false, balready_volume_partitioning);
          }
        }

        debugEntry (LL_DEBUG, LOG_MODULE_INDEX_HIRE, "load hire package volume free  %llu Bytes.", get_hire_package_volume_free());

      }
    }
  }

  return true;
}

bool hireVolume::_save_volume_status()
{
  bool rtCode = true;
  FJson::Value root, hire_rent_items;
  char szBytes[128] = {0};
  
  root["utg_status"] = _hire_status;

  bzero(szBytes, 128);
  sprintf(szBytes, "%llu", _hire_package_volume_expect);
  root["utg_package_volume_expect"] = std::string(szBytes);

  bzero(szBytes, 128);
  sprintf(szBytes, "%llu", _hire_package_volume_actual);
  root["utg_package_volume_actual"] = std::string(szBytes);

  //B0 DATA
  root["utg_block"] = _volume_package_hire_blockchain_number;
  root["utg_nonce"] = _volume_package_hire_blockchain_nonce;
  root["utg_block_hash"] = _volume_package_hire_blockchain_number_hash;
  if (!_volume_poc.empty()) root["utg_volume_poc"] = _volume_poc;
  
  //HIRE RENT ITEMS
  get_all_rent_items(hire_rent_items);

  if (0 < hire_rent_items.size()) root["utg_rent_items"] = hire_rent_items;
  
  //
  std::string data_content = root.toCompatString();
  rtCode = auxHelper::save_to_file(HIREFS_VOLUME_STATUS_FILE_NAME, data_content);

  debugEntry (LL_DEBUG, LOG_MODULE_INDEX_HIRE, "save hire volume status info : %s.", data_content.c_str());
  
  return rtCode;
}

bool hireVolume::_init_topmost_merkletree(u_int64_t volume_bytes, MerkleTree &merkletree)
{
  bool rtCode = false;

  if (0 < volume_bytes) {
    u_char topmost_depth_index = MerkleTree::volumeToDepth(volume_bytes);

    merkletree.unInitMerkleTree();
    rtCode = merkletree.initMerkleTree(topmost_depth_index + 1);
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "%llu bytes volume need MerkleTree depth %u", volume_bytes, topmost_depth_index + 1);
  }

  return rtCode;
}

bool hireVolume::_load_original_topmost_merkletree()
{
  bool rtCode = false;
  
  if (0 < _hire_package_volume_expect) {
    rtCode = _init_topmost_merkletree(_hire_package_volume_expect, _merkletree_topmost);
      
    if (rtCode && 0 < _hire_package_volume_actual)
      rtCode = _load_merkletree_from_file(HIREFS_VOLUME_PACKAGE_TOPMOST_MERKLETREE_FILE_NAME, _merkletree_topmost);
    
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "topmost MerkleTree root : %s", auxHelper::byte2hexstring(_merkletree_topmost.getMerkleTreeRootNode()->data, DATA_NODE_SIZE).c_str());
  }

  return rtCode;
}

bool hireVolume::_load_free_topmost_merkletree(u_int64_t hire_package_volume_free, MerkleTree &merkletree)
{
  bool rtCode = false;

  if (0 < _hire_package_volume_expect) {
    if (hire_package_volume_free < _hire_package_volume_expect) {
      rtCode = _init_topmost_merkletree(hire_package_volume_free, merkletree);
      
      if (rtCode) {
        u_int64_t node_index_beign = 0;
        u_int64_t node_index_end = hire_package_volume_free / DATA_NODE_SIZE;
        _new_merkletree_from_topmost_merkletree(node_index_beign, node_index_end, _merkletree_topmost, merkletree);
      }

      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "topmost MerkleTree root after rent : %s", auxHelper::byte2hexstring(merkletree.getMerkleTreeRootNode()->data, DATA_NODE_SIZE).c_str());
    }
  }

  return rtCode;
}

bool hireVolume::_load_rent_topmost_merkletree()
{
  bool rtCode = false;

  if (0 < _hire_package_volume_expect) {
    std::map<std::string, HIRE_RENT_ITEM>::iterator it;
    lock_hire_rent();
    for (it = _hire_rent_items.begin(); it != _hire_rent_items.end(); it++) {
      if (NULL == it->second.phire_rent_merkletree_topmost) continue;
      rtCode = _check_hire_rent_merkletree(_merkletree_topmost, *(it->second.phire_rent_merkletree_topmost), it->second.hire_rent_volume, it->second.hire_rent_index);
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "rent voucher %s topmost MerkleTree root : %s", it->second.hire_rent_voucher.c_str(), auxHelper::byte2hexstring(it->second.phire_rent_merkletree_topmost->getMerkleTreeRootNode()->data, DATA_NODE_SIZE).c_str());
    }
    unlock_hire_rent();
  } 

  return rtCode;
}

bool hireVolume::_load_topmost_merkletree()
{
  bool rtCode = false;
  
  do
  {
    rtCode = _load_original_topmost_merkletree();
    if (!rtCode) break;

    _load_rent_topmost_merkletree();
   
  } while (false);
  
  return rtCode;
}

void hireVolume::_check_volume_package_status_when_start()
{
  if (0 != _hire_package_volume_expect && _hire_package_volume_expect == _hire_package_volume_actual) {
    _load_topmost_merkletree();
  }

  if (_hire_package_volume_actual < _hire_package_volume_expect) {
    FJson::Value root;
    root["cmd"] = HIRECMD_VOLUME_PACKAGE_CONTINUE;
    notify_hireVolume(root.toCompatString());
  }
}

std::string hireVolume::_make_poc_random_node_index_hash(const std::string &hire_block, const std::string &hire_nonce, const std::string &hire_block_hash)
{
  u_char hash_buffer[DATA_NODE_SIZE] = {0};
  u_int hash_buffer_len = DATA_NODE_SIZE;
  std::string hire_random_node_index_data = hire_block + hire_nonce + hire_block_hash;

  SHA_CTX ctx;
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, hire_random_node_index_data.c_str(), hire_random_node_index_data.length());
  SHA1_Final(hash_buffer, &ctx);

  std::string hire_random_node_index_big_num_str;
  hire_random_node_index_big_num_str.append((const char *)hash_buffer, hash_buffer_len);

  return hire_random_node_index_big_num_str;
}

bool hireVolume::_load_B0_Bn1_Bn(u_int64_t random_node_index, data_node &data_node_zero, MerkleTree &merkletree_cur, MerkleTree &merkletree_pre)
{
  bool rtCode = false;

  do
  {
    merkletree_cur.unInitMerkleTree();
    rtCode = merkletree_cur.initMerkleTree(DATA_MERKLE_TREE_DEPTH_INDEX + 1);
    if (!rtCode) break;
    
    merkletree_pre.unInitMerkleTree();
    rtCode = merkletree_pre.initMerkleTree(DATA_MERKLE_TREE_DEPTH_INDEX + 1);
    if (!rtCode) break;

    rtCode = _read_volume_package_file(_get_volume_package_file_name(0), data_node_zero.data, DATA_NODE_SIZE, 0);
    if (!rtCode) break;

    ssize_t data_level_block_bytes = MerkleTree::getLevelBufferSize(DATA_MERKLE_TREE_DEPTH_INDEX); 
    u_int64_t random_node_data_bytes = random_node_index * DATA_NODE_SIZE;
    u_int64_t random_node_block_index = random_node_data_bytes / data_level_block_bytes;
    u_int random_node_block_filename_index = random_node_block_index / HIRE_VOLUME_PACKAGE_FILE_WRITE_TIMES;
    u_int64_t random_node_block_in_file_index = random_node_block_index % HIRE_VOLUME_PACKAGE_FILE_WRITE_TIMES;
    ssize_t random_block_file_seek = random_node_block_in_file_index * data_level_block_bytes;
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "read Cur Block from : %s", _get_volume_package_file_name(random_node_block_filename_index).c_str());
    rtCode = _read_volume_package_file_to_merkletree_datalevel(_get_volume_package_file_name(random_node_block_filename_index), merkletree_cur, random_block_file_seek);
    if (!rtCode) break;

    merkletree_cur.produceMerkleLevles();
    data_node *pnode_1 = merkletree_cur.getMerkleTreeRootNode();
    data_node *pnode_2 = _merkletree_topmost.getDataLevelNode(random_node_block_index);
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Cur merkle root node : %s", auxHelper::byte2hexstring(pnode_1->data, DATA_NODE_SIZE).c_str());
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Cur merkle root node in TopMerkle : %s", auxHelper::byte2hexstring(pnode_2->data, DATA_NODE_SIZE).c_str());
    rtCode = (0 == memcmp(pnode_1->data, pnode_2->data, DATA_NODE_SIZE));
    if (!rtCode) break;

    if (0 == (1 & random_node_index) && 0 != random_node_index) {
      u_int64_t node_index_in_cur_data_level = random_node_index % MerkleTree::getLevelNodeSize(DATA_MERKLE_TREE_DEPTH_INDEX);
      if (0 == node_index_in_cur_data_level) { 
        if (0 == random_node_block_in_file_index && 0 != random_node_block_filename_index) { 
          debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "read Pre Block from : %s", _get_volume_package_file_name(random_node_block_filename_index - 1).c_str());
          ssize_t pre_block_file_seek = (HIRE_VOLUME_PACKAGE_FILE_WRITE_TIMES - 1) * data_level_block_bytes;
          rtCode = _read_volume_package_file_to_merkletree_datalevel(_get_volume_package_file_name(random_node_block_filename_index - 1), merkletree_pre, pre_block_file_seek);
          if (!rtCode) break;
        } else {
          debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "read Pre Block from : %s", _get_volume_package_file_name(random_node_block_filename_index).c_str());
          ssize_t pre_block_file_seek = random_block_file_seek - data_level_block_bytes;
          rtCode = _read_volume_package_file_to_merkletree_datalevel(_get_volume_package_file_name(random_node_block_filename_index), merkletree_pre, pre_block_file_seek);
          if (!rtCode) break;
        }

        merkletree_pre.produceMerkleLevles();
        pnode_1 = merkletree_pre.getMerkleTreeRootNode();
        pnode_2 = _merkletree_topmost.getDataLevelNode(random_node_block_index - 1);
        debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Pre merkle root node : %s", auxHelper::byte2hexstring(pnode_1->data, DATA_NODE_SIZE).c_str());
        debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Pre merkle root node in TopMerkle : %s", auxHelper::byte2hexstring(pnode_2->data, DATA_NODE_SIZE).c_str());
        rtCode = (0 == memcmp(pnode_1->data, pnode_2->data, DATA_NODE_SIZE));
        if (!rtCode) break;
      }
    }

  } while (false);

  return rtCode;
}

bool hireVolume::_add_B0_Bn1_Bn_to_poc(u_int64_t random_node_index, data_node &data_node_zero, MerkleTree &merkletree_cur, MerkleTree &merkletree_pre, std::string &hire_volume_poc)
{
  bool rtCode = false;

  do
  {
    data_node *pBn_1 = NULL, *pBn = NULL, *pBn1 = NULL;
    u_int64_t node_index_in_cur_data_level = random_node_index % MerkleTree::getLevelNodeSize(DATA_MERKLE_TREE_DEPTH_INDEX);
    
    //Make the POC
    std::string temp_str;
    if (0 == (1 & random_node_index)) {
      //B0,Bn-1,Bn,Bn+1
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "B0,Bn-1,Bn,Bn+1");

      if (0 == node_index_in_cur_data_level) pBn_1 = merkletree_pre.getDataLevelLastNode();
      else pBn_1 = merkletree_cur.getDataLevelNode(node_index_in_cur_data_level - 1);
      
      pBn = merkletree_cur.getDataLevelNode(node_index_in_cur_data_level);
      pBn1 = merkletree_cur.getDataLevelNode(node_index_in_cur_data_level + 1);

      temp_str = auxHelper::byte2hexstring(data_node_zero.data, DATA_NODE_SIZE);
      hire_volume_poc += std::string(",") + temp_str;
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "B0     : %s [0]", temp_str.c_str());

      temp_str = auxHelper::byte2hexstring(pBn_1->data, DATA_NODE_SIZE);
      hire_volume_poc += std::string(",") + temp_str;
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Bn-1   : %s [%llu]", temp_str.c_str(), random_node_index - 1);

      temp_str = auxHelper::byte2hexstring(pBn->data, DATA_NODE_SIZE);
      hire_volume_poc += std::string(",") + temp_str;
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Bn     : %s [%llu]", temp_str.c_str(), random_node_index);

      temp_str = auxHelper::byte2hexstring(pBn1->data, DATA_NODE_SIZE);
      hire_volume_poc += std::string(",") + temp_str;
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Bn+1   : %s [%llu]", temp_str.c_str(), random_node_index + 1);
    } else {
      //B0,Bn-1,Bn
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "B0,Bn-1,Bn");
      
      pBn_1 = merkletree_cur.getDataLevelNode(node_index_in_cur_data_level - 1);
      pBn = merkletree_cur.getDataLevelNode(node_index_in_cur_data_level);

      temp_str = auxHelper::byte2hexstring(data_node_zero.data, DATA_NODE_SIZE);
      hire_volume_poc += std::string(",") + temp_str;
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "B0     : %s [0]", temp_str.c_str());

      temp_str = auxHelper::byte2hexstring(pBn_1->data, DATA_NODE_SIZE);
      hire_volume_poc += std::string(",") + temp_str;
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Bn-1   : %s [%llu]", temp_str.c_str(), random_node_index - 1);

      temp_str = auxHelper::byte2hexstring(pBn->data, DATA_NODE_SIZE);
      hire_volume_poc += std::string(",") + temp_str;
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "Bn     : %s [%llu]", temp_str.c_str(), random_node_index);
    }

    rtCode = merkletree_cur.dataLevelNodeCheck(random_node_index, &data_node_zero, pBn_1, pBn);
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "B%llu node check : %s", random_node_index, rtCode ? "succ" : "fail");

  } while (false);

  return rtCode;
}

void hireVolume::_get_merkletree_node(u_int64_t level_index, u_int64_t &node_index_a, u_int64_t &node_index_b, u_int64_t &node_index_f)
{
  if (0 == (1 & level_index)) {
    //N,N+1
    node_index_a = level_index;
    node_index_b = level_index + 1;
  } else {
    //N-1,N
    node_index_a = level_index - 1;
    node_index_b = level_index;
  }
  node_index_f = (level_index / 2);
}

bool hireVolume::_new_merkletree_from_topmost_merkletree(u_int64_t node_index_begin, u_int64_t node_index_end, MerkleTree &merkletree_topmost, MerkleTree &new_merkletree)
{
  bool rtCode = false;

  u_int merkletree_topmost_index_begin = node_index_begin / MerkleTree::getLevelNodeSize(DATA_MERKLE_TREE_DEPTH_INDEX);
  u_int merkletree_topmost_index_end = (node_index_end + 1) / MerkleTree::getLevelNodeSize(DATA_MERKLE_TREE_DEPTH_INDEX);
  
  for (size_t i = merkletree_topmost_index_begin; i < merkletree_topmost_index_end; i++) 
    rtCode = new_merkletree.addDataLevelNode(merkletree_topmost.getDataLevelNode(i));

  new_merkletree.produceMerkleLevles();

  return rtCode;
}

bool hireVolume::_make_volume_poc_merkletree_path(u_int64_t node_index_begin, MerkleTree &merkletree_topmost, u_int64_t random_node_index, MerkleTree &merkletree_cur, std::string &hire_volume_poc)
{
  bool rtCode = false, bhash = true;
  data_node *pchild_node_a = NULL, *pchild_node_b = NULL, *pfather_node = NULL;
  u_int64_t level_index = random_node_index, node_index_a = 0, node_index_b = 0, node_index_f = 0;
  std::string temp_str;
  data_node father_node;

  size_t i = 0;
  for (i = 0; i < DATA_MERKLE_TREE_DEPTH_INDEX; i++) {

    _get_merkletree_node(level_index, node_index_a, node_index_b, node_index_f);

    pchild_node_a = merkletree_cur.getNode(DATA_MERKLE_TREE_DEPTH_INDEX - i, node_index_a % MerkleTree::getLevelNodeSize(DATA_MERKLE_TREE_DEPTH_INDEX - i));
    pchild_node_b = merkletree_cur.getNode(DATA_MERKLE_TREE_DEPTH_INDEX - i, node_index_b % MerkleTree::getLevelNodeSize(DATA_MERKLE_TREE_DEPTH_INDEX - i));
    pfather_node = merkletree_cur.getNode(DATA_MERKLE_TREE_DEPTH_INDEX - i - 1, node_index_f % MerkleTree::getLevelNodeSize(DATA_MERKLE_TREE_DEPTH_INDEX - i - 1));
    if (NULL == pchild_node_a || NULL == pchild_node_b || NULL == pfather_node) break;
    
    merkletree_cur.produceMerkleLevelNode(pchild_node_a, pchild_node_b, &father_node, bhash);

    rtCode = (0 == memcmp(father_node.data, pfather_node->data, DATA_NODE_SIZE));
    if (!rtCode) break;
    
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "%s  %s => %s [L%02u(%u %u %s)]",
            auxHelper::byte2hexstring(pchild_node_a->data, DATA_NODE_SIZE).c_str(),
            auxHelper::byte2hexstring(pchild_node_b->data, DATA_NODE_SIZE).c_str(),
            auxHelper::byte2hexstring(father_node.data, DATA_NODE_SIZE).c_str(),
            (DATA_MERKLE_TREE_DEPTH_INDEX - i), node_index_a, node_index_b, (bhash ? "hash" : " acc"));

    if (0 < i) {
      if (0 == (1 & level_index)) {
        temp_str = auxHelper::byte2hexstring(pchild_node_b->data, DATA_NODE_SIZE);
        hire_volume_poc += std::string(",") + temp_str;
      } else {
        temp_str = auxHelper::byte2hexstring(pchild_node_a->data, DATA_NODE_SIZE);
        hire_volume_poc += std::string(",") + temp_str;
      }
    }

    bhash = !bhash;
    level_index = (level_index / 2);
  }
  
  if (!rtCode) return rtCode;
  rtCode = (i == DATA_MERKLE_TREE_DEPTH_INDEX);
  if (!rtCode) return rtCode;

  u_int merkletree_topmost_index_begin = node_index_begin / MerkleTree::getLevelNodeSize(DATA_MERKLE_TREE_DEPTH_INDEX);
  level_index -= merkletree_topmost_index_begin;

  u_char mdepth = merkletree_topmost.getDepth() - 1;
  for (i = 0; i < mdepth; i++) {

    _get_merkletree_node(level_index, node_index_a, node_index_b, node_index_f);
    
    pchild_node_a = merkletree_topmost.getNode(mdepth - i, node_index_a);
    pchild_node_b = merkletree_topmost.getNode(mdepth - i, node_index_b);
    pfather_node = merkletree_topmost.getNode(mdepth - i - 1, node_index_f);
    
    merkletree_topmost.produceMerkleLevelNode(pchild_node_a, pchild_node_b, &father_node, bhash);
    rtCode = (0 == memcmp(father_node.data, pfather_node->data, DATA_NODE_SIZE));
    if (!rtCode) break;

    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "%s  %s => %s [M%02u(%u %u %s)]",
            auxHelper::byte2hexstring(pchild_node_a->data, DATA_NODE_SIZE).c_str(),
            auxHelper::byte2hexstring(pchild_node_b->data, DATA_NODE_SIZE).c_str(),
            auxHelper::byte2hexstring(father_node.data, DATA_NODE_SIZE).c_str(),
            (mdepth - i), node_index_a, node_index_b, (bhash ? "hash" : " acc"));
    
    if (0 == (1 & level_index)) {
      temp_str = auxHelper::byte2hexstring(pchild_node_b->data, DATA_NODE_SIZE);
      hire_volume_poc += std::string(",") + temp_str;
    } else {
      temp_str = auxHelper::byte2hexstring(pchild_node_a->data, DATA_NODE_SIZE);
      hire_volume_poc += std::string(",") + temp_str;
    }

    bhash = !bhash;
    level_index = (level_index / 2);
  }
  
  // root 
  if (NULL != pfather_node) {
    temp_str = auxHelper::byte2hexstring(pfather_node->data, DATA_NODE_SIZE);
    hire_volume_poc += std::string(",") + temp_str;
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "%s                                                                                       [M%02u(0 0 root)]", temp_str.c_str(), (mdepth - i));
  }
    
  if (!rtCode) return rtCode;
  rtCode = (i == mdepth);

  return rtCode;
}

bool hireVolume::_get_node_index_from_str(const std::string &hire_rent_index, u_int64_t &node_index_begin, u_int64_t &node_index_end)
{
  size_t ipos = 0;
  std::string rent_index = hire_rent_index;

  ipos = rent_index.find("-");
  if (ipos != std::string::npos) {
    std::string temp_id = "";

    temp_id.append(rent_index.begin(), rent_index.begin() + ipos);
    node_index_begin = strtoll(temp_id.c_str(), NULL, 10);

    temp_id = "";
    temp_id.append(rent_index.begin() + ipos + 1, rent_index.end());
    node_index_end = strtoll(temp_id.c_str(), NULL, 10);

    return true;
  } else return false;
}

void hireVolume::_make_volume_package_poc_head(u_int64_t hire_package_volume_free, u_int64_t random_node_index, const std::string &hire_block, const std::string &hire_nonce, const std::string &hire_block_hash, std::string &hire_package_volume_poc_head)
{
  if (0 == hire_package_volume_free) return;
  u_int64_t data_node_number = hire_package_volume_free / DATA_NODE_SIZE;

  char szBytes[256] = {0};
  sprintf(szBytes, "v%u,%s,%s,%s,%llu,%llu,%llu,0-%llu", HIRE_VOLUME_PACKAGE_POC_VERSION, hire_block.c_str(), hire_nonce.c_str(), hire_block_hash.c_str(), random_node_index, DATA_NODE_SIZE, data_node_number, data_node_number - 1);
  hire_package_volume_poc_head = std::string(szBytes);
}

u_int64_t hireVolume::_make_volume_package_poc_random_node_index(u_int64_t hire_package_volume_free, const std::string &hire_block, const std::string &hire_nonce, const std::string &hire_block_hash)
{
  if (0 == hire_package_volume_free) return 0;

  u_int64_t data_node_number = hire_package_volume_free / DATA_NODE_SIZE;

  std::string hire_random_node_index_big_num_str = _make_poc_random_node_index_hash(hire_block, hire_nonce, hire_block_hash);

  InfInt hire_random_node_index_data_num = decode_bignum(hire_random_node_index_big_num_str);
  InfInt random_node_index_bignum = hire_random_node_index_data_num % data_node_number;
  debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "volume package => random_node_index_data_num : %s, random_node_index_bignum : %s", hire_random_node_index_data_num.toString().c_str(), random_node_index_bignum.toString().c_str());
  u_int64_t random_node_index = random_node_index_bignum.toUnsignedLongLong();
  if (0 == random_node_index) random_node_index = data_node_number - 1; 
  
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume package poc random node index %llu.", random_node_index);

  return random_node_index;
}

bool hireVolume::_get_volume_package_poc(u_int64_t random_node_index, MerkleTree &merkletree_topmost, std::string &hire_volume_package_poc)
{
  bool rtCode = false;
  MerkleTree merkletree_cur, merkletree_pre;
  data_node data_node_zero;

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume package poc Node Index : %llu.", random_node_index);

  do
  {
    if (!_volume_poc.empty()) {
      hire_volume_package_poc += _volume_poc;
      rtCode = true;
      break;
    }

    rtCode = _load_B0_Bn1_Bn(random_node_index, data_node_zero, merkletree_cur, merkletree_pre);
    if (!rtCode) break;

    rtCode = _add_B0_Bn1_Bn_to_poc(random_node_index, data_node_zero, merkletree_cur, merkletree_pre, _volume_poc);
    if (!rtCode) break;

    rtCode = _make_volume_poc_merkletree_path(0, merkletree_topmost, random_node_index, merkletree_cur, _volume_poc);
    if (!rtCode) break;
    
    hire_volume_package_poc += _volume_poc;

    _save_volume_status();

  } while (false);

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume package poc :\n%s", hire_volume_package_poc.c_str());

  return rtCode;
}

void hireVolume::_make_volume_rent_poc_head(HIRE_RENT_ITEM &hire_rent_item, u_int64_t random_node_index, const std::string &hire_block, const std::string &hire_nonce, const std::string &hire_block_hash, std::string &hire_volume_rent_poc_head)
{
  u_int64_t rent_index_begin = 0, rent_index_end = 0;
  _get_node_index_from_str(hire_rent_item.hire_rent_index, rent_index_begin, rent_index_end);
  u_int64_t data_node_number = rent_index_end - rent_index_begin + 1;

  char szBytes[256] = {0};
  sprintf(szBytes, "v%u,%s,%s,%s,%llu,%llu,%llu,%s", HIRE_VOLUME_RENT_POC_VERSION, hire_block.c_str(), hire_nonce.c_str(), hire_block_hash.c_str(), random_node_index, DATA_NODE_SIZE, data_node_number, hire_rent_item.hire_rent_index.c_str());
  hire_volume_rent_poc_head = std::string(szBytes);
}

u_int64_t hireVolume::_make_volume_rent_poc_random_node_index(const std::string &hire_rent_index, const std::string &hire_block, const std::string &hire_nonce, const std::string &hire_block_hash)
{
  std::string hire_random_node_index_big_num_str = _make_poc_random_node_index_hash(hire_block, hire_nonce, hire_block_hash);

  u_int64_t rent_index_begin = 0, rent_index_end = 0, rent_index_num = 0;
  _get_node_index_from_str(hire_rent_index, rent_index_begin, rent_index_end);
  rent_index_num = rent_index_end - rent_index_begin + 1;
  
  InfInt hire_random_node_index_data_num = decode_bignum(hire_random_node_index_big_num_str);
  InfInt random_node_index_bignum = hire_random_node_index_data_num % rent_index_num; //[0, rent_index_num)
  debugEntry(LL_OBSERVE, LOG_MODULE_INDEX_HIRE, "volume rent => random_node_index_data_num : %s, random_node_index_bignum : %s", hire_random_node_index_data_num.toString().c_str(), random_node_index_bignum.toString().c_str());
  u_int64_t random_node_index = random_node_index_bignum.toUnsignedLongLong() + rent_index_begin; //[rent_index_begin, rent_index_end]
  if (0 == random_node_index) random_node_index = rent_index_end;
  
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume rent poc random node index %llu.", random_node_index);

  return random_node_index;
}

bool hireVolume::_get_volume_rent_poc(HIRE_RENT_ITEM &hire_rent_item, u_int64_t random_node_index, std::string &hire_volume_rent_poc)
{
  bool rtCode = false;
  MerkleTree merkletree_cur, merkletree_pre;
  data_node data_node_zero;
  
  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "volume rent poc  : %llu.", random_node_index);

  do
  {
    if (!_volume_poc.empty()) {
      hire_volume_rent_poc += _volume_poc;
      rtCode = true;
      break;
    }

    if (NULL == hire_rent_item.phire_rent_merkletree_topmost) break;
    MerkleTree &merkletree_topmost = *(hire_rent_item.phire_rent_merkletree_topmost);

    rtCode = _load_B0_Bn1_Bn(random_node_index, data_node_zero, merkletree_cur, merkletree_pre);
    if (!rtCode) break;

    rtCode = _add_B0_Bn1_Bn_to_poc(random_node_index, data_node_zero, merkletree_cur, merkletree_pre, _volume_poc);
    if (!rtCode) break;
    
    u_int64_t rent_index_begin = 0, rent_index_end = 0;
    _get_node_index_from_str(hire_rent_item.hire_rent_index, rent_index_begin, rent_index_end);
    rtCode = _make_volume_poc_merkletree_path(rent_index_begin, merkletree_topmost, random_node_index, merkletree_cur, _volume_poc);
    if (!rtCode) break;
    
    hire_volume_rent_poc += _volume_poc;
  } while (false);

  debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "%s volume rent poc :\n%s", hire_rent_item.hire_rent_voucher.c_str(), hire_volume_rent_poc.c_str());

  return rtCode;
}

bool hireVolume::_check_hire_rent_merkletree(MerkleTree &merkletree_topmost, MerkleTree &rent_merkletree_topmost, const std::string &hire_rent_volume, const std::string &hire_rent_index)
{
  bool rtCode = false;

  do
  {
    u_int64_t hire_rent_volume_bytes = strtoll(hire_rent_volume.c_str(), NULL, 10);
    rtCode = _init_topmost_merkletree(hire_rent_volume_bytes, rent_merkletree_topmost);
    if (!rtCode) break;

    u_int64_t node_index_begin = 0, node_index_end = 0;
    rtCode = _get_node_index_from_str(hire_rent_index, node_index_begin, node_index_end);
    if (!rtCode) break;
    
    rtCode = _new_merkletree_from_topmost_merkletree(node_index_begin, node_index_end, merkletree_topmost, rent_merkletree_topmost);
    if (!rtCode) break;

  } while (false);

  return rtCode;
}

bool hireVolume::_add_hire_rent_item(const std::string &hire_block_number, const std::string &hire_rent_voucher, const std::string &hire_rent_volume, const std::string &hire_rent_index, const std::string &hire_rent_addr, u_int64_t hire_rent_time, bool bhire_rent_onpledge, bool check_merkletree, bool balready_volume_partitioning)
{
  bool rtCode = false;

  do
  {
    HIRE_RENT_ITEM hire_rent_item;
    hire_rent_item.hire_block_number = hire_block_number;
    hire_rent_item.hire_rent_voucher = hire_rent_voucher;
    hire_rent_item.hire_rent_volume = hire_rent_volume;
    hire_rent_item.hire_rent_index = hire_rent_index;
    hire_rent_item.hire_rent_addr = hire_rent_addr;
    hire_rent_item.hire_rent_time = hire_rent_time;
    hire_rent_item.bhire_rent_onpledge = bhire_rent_onpledge;
    hire_rent_item.balready_volume_partitioning = balready_volume_partitioning;
    
    MerkleTree *pmerkletree_topmost = new MerkleTree;
    if (NULL == pmerkletree_topmost) break;
    hire_rent_item.phire_rent_merkletree_topmost = pmerkletree_topmost;

    if (check_merkletree) {
      rtCode = _check_hire_rent_merkletree(_merkletree_topmost, *pmerkletree_topmost, hire_rent_volume, hire_rent_index);
      if (!rtCode) break;
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "%s's topmost root : %s.", hire_rent_voucher.c_str(), auxHelper::byte2hexstring((u_char *)((*pmerkletree_topmost).getMerkleTreeRootNode()), DATA_NODE_SIZE).c_str());
    }
    
    std::pair<std::map<std::string, HIRE_RENT_ITEM>::iterator, bool> ret;
    lock_hire_rent();
    ret = _hire_rent_items.insert(std::make_pair(hire_rent_item.hire_rent_voucher, hire_rent_item));
    unlock_hire_rent();

    rtCode = ret.second;

  } while (false);

  return rtCode;
}

void hireVolume::update_rent_items_onpledge(std::map<std::string, int> &rent_items_status, std::list<std::string> &rent_items_outpledge, std::list<HIRE_RENT_ITEM> &rent_items_volume_partitioning)
{
  rent_items_outpledge.clear();
  std::map<std::string, int>::iterator it_status;
  for (it_status = rent_items_status.begin(); it_status != rent_items_status.end(); it_status++) {
    if (6 == it_status->second) rent_items_outpledge.push_back(it_status->first);
  }

  lock_hire_rent();
  std::map<std::string, HIRE_RENT_ITEM>::iterator it;
  for (it = _hire_rent_items.begin(); it != _hire_rent_items.end(); it++) {
    it_status = rent_items_status.find(it->first);
    if (it_status != rent_items_status.end()) {
      int rent_status = it_status->second;
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "rent item %s's status %u.", it->first.c_str(), rent_status);
      bool bhire_rent_onpledge = (1 == rent_status || 4 == rent_status ) ? true : false;
      if (bhire_rent_onpledge != it->second.bhire_rent_onpledge) {
        it->second.bhire_rent_onpledge = bhire_rent_onpledge;
        if (it->second.bhire_rent_onpledge) { 
          debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "rent item %s is onpledge", it->first.c_str());
        } else debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "rent item %s is outpledge", it->first.c_str());
      }
      if (it->second.bhire_rent_onpledge) it->second.bneed_retrieve_by_manual = false;
      if (it->second.bhire_rent_onpledge && !(it->second.balready_volume_partitioning)) rent_items_volume_partitioning.push_back(it->second);
    } else { 
      it->second.bhire_rent_onpledge = false;
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "rent item %s is outpledge", it->first.c_str());
    }
  }
  unlock_hire_rent();
}

void hireVolume::update_rent_items(u_int64_t hire_lasted_block_number, u_int64_t hire_volume_retrieve_block_number)
{
  u_int64_t hire_package_volume_free = get_hire_package_volume_free();

  std::map<std::string, HIRE_RENT_ITEM>::iterator it;
  lock_hire_rent();
  for (it = _hire_rent_items.begin(); it != _hire_rent_items.end(); it++) {
    u_int64_t hire_rent_block_number = strtoll(it->second.hire_block_number.c_str(), NULL, 10);
    u_int64_t after_hire_block_number = (hire_lasted_block_number > hire_rent_block_number) ? (hire_lasted_block_number - hire_rent_block_number) : 0;
    if (false == it->second.bhire_rent_onpledge && hire_volume_retrieve_block_number < after_hire_block_number) {
      //rent_items_need_del.push_back(it->first);
      it->second.bneed_retrieve_by_manual = true;
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "rent item %s need to retrieve by manual.", it->first.c_str());

      char szbuffer[128] = {0};
      sprintf(szbuffer, "%llu", hire_lasted_block_number);
      hireLogManager::instance().add_volume_rent_log(HIRE_LOG_ACT_RETRIEVE_BY_ENQUIRE_TIMEOUT, it->first, std::string(szbuffer), std::string(""), auxHelper::intBytesToGBStr(hire_package_volume_free));
    } else it->second.bneed_retrieve_by_manual = false;
  }
  unlock_hire_rent();

  _save_volume_status();
}

void hireVolume::get_volume_poc_collection_by_group_name(FJson::Value &root, const std::string &group_name, std::map<std::string, std::string> &hire_poc_collection_groups)
{
  std::map<std::string, std::vector<HIRE_RENT_ITEM>>::iterator it;

  if ("all" == group_name) {
    lock_hire_poc_group();
    for (it = _hire_poc_groups.begin(); it !=  _hire_poc_groups.end(); it++) {
      std::string hire_poc;
      std::vector<HIRE_RENT_ITEM> &hire_poc_items = it->second;
      if (0 < hire_poc_items.size()) {
        _poc_group_items_to_poc_collection(root, hire_poc_items, hire_poc);
        hire_poc_collection_groups.insert(std::make_pair(it->first, hire_poc));
      }
    }
    unlock_hire_poc_group();
  } else {
    std::string hire_poc;
    std::vector<HIRE_RENT_ITEM> hire_poc_items;
    std::string group_name_key = _get_volume_poc_group_items_by_item_voucher_in_group(group_name, hire_poc_items);
    if (!group_name_key.empty() && 0 < hire_poc_items.size()) {
      _poc_group_items_to_poc_collection(root, hire_poc_items, hire_poc);
      hire_poc_collection_groups.insert(std::make_pair(group_name_key, hire_poc));
    }
  }
}

void hireVolume::get_volume_poc_group_items_by_group_name(const std::string &group_name, std::vector<HIRE_RENT_ITEM> &hire_poc_group_items)
{
  std::map<std::string, std::vector<HIRE_RENT_ITEM>>::iterator it;

  lock_hire_poc_group();
  it = _hire_poc_groups.find(group_name);
  if (it != _hire_poc_groups.end())
    hire_poc_group_items = it->second;
  unlock_hire_poc_group();
}

void hireVolume::get_volume_poc_group_voucher_by_group_name(const std::string &group_name, std::vector<std::string> &hire_poc_group_voucher_items)
{
  std::map<std::string, std::vector<HIRE_RENT_ITEM>>::iterator it;

  if ("all" == group_name) {
    lock_hire_poc_group();
    for (it = _hire_poc_groups.begin(); it !=  _hire_poc_groups.end(); it++) {
      std::string hire_voucher;
      std::vector<HIRE_RENT_ITEM> &hire_poc_items = it->second;
      for (size_t i = 0; i < hire_poc_items.size(); i++) {
        if (0 != i) hire_voucher += ",";
        hire_voucher += hire_poc_items[i].hire_rent_voucher;
      }
      hire_poc_group_voucher_items.push_back(hire_voucher);
    }
    unlock_hire_poc_group();
  } else {
    lock_hire_poc_group();
    it = _hire_poc_groups.find(group_name);
    if (it != _hire_poc_groups.end()) {
      std::string hire_voucher;
      std::vector<HIRE_RENT_ITEM> &hire_poc_items = it->second;
      for (size_t i = 0; i < hire_poc_items.size(); i++) {
        if (0 != i) hire_voucher += ",";
        hire_voucher += hire_poc_items[i].hire_rent_voucher;
      }
      hire_poc_group_voucher_items.push_back(hire_voucher);
    }
    unlock_hire_poc_group();
  }
}

std::string hireVolume::_get_volume_poc_group_items_by_item_voucher_in_group(const std::string &item_voucher, std::vector<HIRE_RENT_ITEM> &hire_poc_group_items)
{
  std::string group_name = "";

  get_volume_poc_group_items_by_group_name(item_voucher, hire_poc_group_items);
  if (0 == hire_poc_group_items.size()) {
    std::map<std::string, std::vector<HIRE_RENT_ITEM>>::iterator it;
    lock_hire_poc_group();
    
    for (it = _hire_poc_groups.begin(); it !=  _hire_poc_groups.end(); it++) {
      bool bfind = false;

      std::vector<HIRE_RENT_ITEM> &hire_poc_items = it->second;
      for (size_t i = 0; i < hire_poc_items.size(); i++) {
        if (item_voucher == hire_poc_items[i].hire_rent_voucher) {
          bfind = true;
          break;
        }
      }
      
      if (bfind) {
        group_name = it->first;
        hire_poc_group_items = it->second;
        break;
      }

    }

    unlock_hire_poc_group();
  } else group_name = item_voucher;

  return group_name;
}

bool hireVolume::_del_hire_rent_item(const std::string &hire_rent_voucher)
{
  bool rtCode = false;
  std::map<std::string, HIRE_RENT_ITEM>::iterator it;

  lock_hire_rent();
  it = _hire_rent_items.find(hire_rent_voucher);
  if (it != _hire_rent_items.end()) {
     MerkleTree *p = it->second.phire_rent_merkletree_topmost;
    debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "del rent item : %s(add at blcok number : %s)", it->first.c_str(), it->second.hire_block_number.c_str());
    _hire_rent_items.erase(it);
    if (NULL != p) delete p;
    rtCode = true;
  }
  unlock_hire_rent();

  return rtCode;
}

void hireVolume::_clear_hire_rent_items()
{
  lock_hire_rent();
  std::map<std::string, HIRE_RENT_ITEM>::iterator it;
  for (it = _hire_rent_items.begin(); it != _hire_rent_items.end(); it++) {
     MerkleTree *p = it->second.phire_rent_merkletree_topmost;
    if (NULL != p) delete p;
  }
  _hire_rent_items.clear();
  unlock_hire_rent();
}

bool hireVolume::_has_rent_item_outpledge()
{
  bool bfind = false;

  lock_hire_rent();
  std::map<std::string, HIRE_RENT_ITEM>::iterator it;
  for (it = _hire_rent_items.begin(); it != _hire_rent_items.end(); it++) {
    if (false == it->second.bhire_rent_onpledge) bfind = true;
  }
  unlock_hire_rent();

  return bfind;
}

void hireVolume::_poc_group_items_to_poc_collection(FJson::Value &root, std::vector<HIRE_RENT_ITEM> &hire_poc_group_items, std::string &hire_poc_collection)
{
  hire_poc_collection = "";

  for (size_t i = 0; i < hire_poc_group_items.size(); i++) {
    if (_hire_address == hire_poc_group_items[i].hire_rent_voucher) {
      FJson::Value root_temp = root;
      do_volume_package_poc(root_temp);
      hire_poc_collection += root_temp["utg_package_poc"].asString();
    } else {
      FJson::Value root_temp = root;
      _do_volume_rent_poc(hire_poc_group_items[i], root_temp);
      hire_poc_collection += "|";
      hire_poc_collection += hire_poc_group_items[i].hire_rent_voucher;
      hire_poc_collection += ",";
      hire_poc_collection += root_temp["utg_rent_poc"].asString();
    }
  }
}

void hireVolume::update_poc_groups()
{
  std::vector<HIRE_RENT_ITEM> hire_poc_group_items;
  
  HIRE_RENT_ITEM hire_rent_item;
  hire_rent_item.hire_rent_voucher = _hire_address;
  hire_poc_group_items.push_back(hire_rent_item);

  lock_hire_poc_group();
  _hire_poc_groups.clear();
  std::map<std::string, HIRE_RENT_ITEM>::iterator it;

  for (it = _hire_rent_items.begin(); it != _hire_rent_items.end(); it++) {
    if (false == it->second.bhire_rent_onpledge) continue;

    hire_poc_group_items.push_back(it->second);
    
    if (HIRE_VOLUME_POC_NUMBER_MAX_IN_ONE_CHAIN_BLOCK == hire_poc_group_items.size()) {
      _hire_poc_groups.insert(std::make_pair(hire_poc_group_items[0].hire_rent_voucher, hire_poc_group_items));
      hire_poc_group_items.clear();
    }
  }

  if (0 < hire_poc_group_items.size())
    _hire_poc_groups.insert(std::make_pair(hire_poc_group_items[0].hire_rent_voucher, hire_poc_group_items));

  std::map<std::string, std::vector<HIRE_RENT_ITEM>>::iterator it_group;
  for (it_group = _hire_poc_groups.begin(); it_group != _hire_poc_groups.end(); it_group++) {
    std::vector<HIRE_RENT_ITEM> &hire_poc_group_items_temp = it_group->second;
    for (size_t i = 0; i < hire_poc_group_items_temp.size(); i++) {
      debugEntry(LL_DEBUG, LOG_MODULE_INDEX_HIRE, "group name : %s, item[%u] : %s.", it_group->first.c_str(), i, hire_poc_group_items_temp[i].hire_rent_voucher.c_str());
    }
  }
  unlock_hire_poc_group();
}

void hireVolume::_clear_hire_poc_groups()
{
  lock_hire_poc_group();
  _hire_poc_groups.clear();
  unlock_hire_poc_group();
}
