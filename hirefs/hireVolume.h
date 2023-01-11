#ifndef HIRE_POC_MANAGER_H
#define HIRE_POC_MANAGER_H

#include "local_defs.h"
#include "messageHelp.h"
#include "FJson.h"
#include <map>
#include <vector>
#include <list>

#define DATA_NODE_SIZE                       20
#define DATA_MERKLE_TREE_DEPTH_INDEX         20 // 1M = 1024 * 1024 = 2^20

#define HIRE_VOLUME_PACKAGE_FILE_WRITE_TIMES    256
#define HIRE_VOLUME_PACKAGE_FILE_NODE_NUM       (HIRE_VOLUME_PACKAGE_FILE_WRITE_TIMES * 1024 * 1024LL)
#define HIRE_VOLUME_PACKAGE_FILE_SIZE           (20 * HIRE_VOLUME_PACKAGE_FILE_NODE_NUM)

#define HIRE_VOLUME_EVENT_TIMEOUT  60

#define HIRE_VOLUME_PACKAGE_POC_VERSION  1
#define HIRE_VOLUME_RENT_POC_VERSION  1

#define HIRE_VOLUME_POC_NUMBER_MAX_IN_ONE_CHAIN_BLOCK     10

struct data_node {
  u_char data[DATA_NODE_SIZE];
};

typedef std::vector<data_node *> DEPTH_LEAFS_VECTOR;
typedef DEPTH_LEAFS_VECTOR* PDEPTH_LEAFS_VECTOR;

class MerkleTree
{
public:
  MerkleTree();
  ~MerkleTree();
public:
  static u_int getLevelBufferSize(u_char depth_index);
  static u_int getLevelNodeSize(u_char depth_index);
  static u_char volumeToDepth(u_int64_t volume_bytes);

  bool initMerkleTree(u_char depth);
  void unInitMerkleTree();

  data_node* getNode(u_char depth_index, u_int level_index);
  data_node* getDataLevelNode(u_int index);
  data_node* getDataLevelFirstNode();
  data_node* getDataLevelLastNode();
  data_node* getMerkleTreeRootNode();

  u_char getDepth() { return _tree.size(); }

  u_int getBufferSize() { return  _buffer_size; }
  u_char *getBuffer() { return _buffer; }
  
  u_int64_t getCurDataBlcokIndex() { return _data_node_index; }
  u_int64_t setCurDataBlcokIndex(u_int64_t data_node_index) { return _data_node_index = data_node_index; }

  void setZeroNode(u_char *pdata, u_int data_len);
  data_node* getZeroNode() { return (&_zero_node); }

  bool addDataLevelNode(data_node* pdata_node);
  void produceDataLevel();
  //返回生成最顶层使用的是 HASH or ACC : true hash, false acc
  bool produceMerkleLevles(bool bstart_by_hash = true); //true hash, false acc, return the top level hash bool
  
  void produceDataLevelNode(u_int64_t data_node_index, data_node * pzero_node, data_node * pn_1_node, data_node * pn_node_out);
  bool dataLevelNodeCheck(u_int64_t data_node_index, data_node * pzero_node, data_node * pn_1_node, data_node * pn_node);

  void produceMerkleLevelNode(data_node * pchild_node_a, data_node * pchild_node_b, data_node * pfather_node_out, bool bhash);
  bool merkleLevelNodeCheck(data_node * pchild_node_a, data_node * pchild_node_b, data_node * pfather_node, bool bhash);
private:
  void _produceMerkleLevles(std::vector<data_node *> &low, std::vector<data_node *> &high, bool bhash);

  void _accumulation(u_char *a, u_char *b, u_char *out, u_int64_t data_node_index = 0);
private:
  std::vector<PDEPTH_LEAFS_VECTOR> _tree;
  u_int _buffer_size;
  u_char *_buffer;
  data_node _zero_node;
  u_int64_t _data_node_index;
};

typedef enum _WRITE_ACTION_TYPE {
  HIREWAY_NONE = 0, //none
  HIREWAY_CW = 1,   //create and write
  HIREWAY_W = 2,    //just write
  HIREWAY_WC =3,    //write and close
}WRITE_ACTION_TYPE;

typedef struct _WRITE_MESSAGE {
  u_char *pbuffer_write;
  WRITE_ACTION_TYPE write_action_type;
  std::string filename;
}WRITE_MESSAGE;

#define DATA_BLOCK_SIZE (DATA_NODE_SIZE * 1024 * 1024LL)
struct data_block{
  u_char data[DATA_BLOCK_SIZE];
};

typedef struct _HIRE_RENT_ITEM
{
  _HIRE_RENT_ITEM() {
    hire_block_number = "";
    hire_rent_voucher = "";
    hire_rent_volume = "";
    hire_rent_index = "";
    hire_rent_addr = "";
    hire_rent_time = 0;
    bhire_rent_onpledge = false;
    bneed_retrieve_by_manual = false;
    balready_volume_partitioning = false;
    phire_rent_merkletree_topmost = NULL;
  }
  u_int64_t hire_rent_time;
  std::string hire_rent_addr;
  std::string hire_block_number;
  std::string hire_rent_voucher;
  std::string hire_rent_volume;
  std::string hire_rent_index;
  bool bhire_rent_onpledge; //true rent on pledge, false rent out pledge
  bool bneed_retrieve_by_manual;
  bool balready_volume_partitioning; //true already partitioning, false not partitioning
  MerkleTree *phire_rent_merkletree_topmost;
} HIRE_RENT_ITEM;

class hireVolume
{
public:
  hireVolume();
  ~hireVolume();

public:
  bool start();
  void stop();
  void wait_stop();
  
public:
  static std::string getMD5(const std::string &message, u_char * pbuffer = NULL);

  //thread function
  static void *thread_hirevolume_handle_worker(void *param);
  void hirevolume_worker();
  
  static void *thread_hirevolume_write_handle_worker(void *param);
  void hirevolume_write_worker();

  void notify_hireVolume(const std::string &message);

public:
  PUBLIC_V(HIRE_STATUS, _hire_status);
  PUBLIC_V(u_int64_t, _hire_package_volume_expect);
  PUBLIC_V(u_int64_t, _hire_package_volume_actual);
  //PUBLIC_V(u_int64_t, _hire_package_volume_free);
  PUBLIC_V(u_int, _remaining_time);
  PUBLIC_V(std::string, _volume_package_filename_prefix);
  PUBLIC_V(bool, _stop_volume_package_flag);
  PUBLIC_V(std::string, _hire_address);

public:
  static bool is_volume_packaged(HIRE_STATUS hire_status);

  //
  CN_ERR do_volume_package_poc(FJson::Value &root);
  //
  CN_ERR do_volume_rent(FJson::Value &root);
  CN_ERR do_volume_rent_poc(FJson::Value &root);
  void get_all_rent_items(FJson::Value &rent_items, bool bvolumeToGB = false);
  void do_volume_rent_volume_partitioning(const std::string &hire_rent_index);
  
  //
  CN_ERR do_volume_retrieve(const std::string &hire_rent_voucher, const std::string &hire_block_number);
  u_int64_t get_hire_package_volume_free();
  void update_rent_items_onpledge(std::map<std::string, int> &rent_items_status, std::list<std::string> &rent_items_outpledge, std::list<HIRE_RENT_ITEM> &rent_items_volume_partitioning);
  void update_rent_items(u_int64_t hire_lasted_block_number, u_int64_t hire_volume_retrieve_block_number);
  void update_poc_groups();
  void update_rent_item_volume_partitioning(const std::string &hire_rent_voucher);

  //group_name为 all 表示获取所有组
  void get_volume_poc_collection_by_group_name(FJson::Value &root, const std::string &group_name, std::map<std::string, std::string> &hire_poc_collection_groups);
  void get_volume_poc_group_items_by_group_name(const std::string &group_name, std::vector<HIRE_RENT_ITEM> &hire_poc_group_items);
  void get_volume_poc_group_voucher_by_group_name(const std::string &group_name, std::vector<std::string> &hire_poc_group_voucher_items);

  //
  void do_outpledg();
public:
  LOCK_V(hire_rent);
  LOCK_V(hire_poc_group);
private:
  void _process_volume_message(const std::string &message);
  void _do_volume_package(FJson::Value &root);
  void _do_volume_package_continue();
  
  void _do_volume_reset();
  std::string _get_volume_package_file_name(u_int index);
  bool _read_volume_package_file_to_merkletree_datalevel(const std::string &file_name, MerkleTree &merkletree, ssize_t seek);
  bool _read_volume_package_file(const std::string &file_name, u_char *pbuffer, u_int64_t buffer_len, ssize_t seek);
  
  bool _save_merkletree_to_file(const std::string &file_name, MerkleTree &merkletree);
  bool _load_merkletree_from_file(const std::string &file_name, MerkleTree &merkletree);
  
  CN_ERR _do_volume_rent_poc(HIRE_RENT_ITEM &hire_rent_item, FJson::Value &root);

  CN_ERR _do_volume_package_poc(u_int64_t hire_package_volume_free, FJson::Value &root);
  void _volume_package(MerkleTree &merkletree_data);
  
  bool _load_volume_status();
  bool _save_volume_status();

  bool _init_topmost_merkletree(u_int64_t volume_bytes, MerkleTree &merkletree);
  bool _load_topmost_merkletree();
  //加载原封装的顶层默克尔树
  bool _load_original_topmost_merkletree();
  //计算出租后剩余空间的顶层默克尔树
  bool _load_free_topmost_merkletree(u_int64_t hire_package_volume_free, MerkleTree &merkletree);
  //计算出租空间的顶层默克尔树
  bool _load_rent_topmost_merkletree();

  void _check_volume_package_status_when_start();

  void _notify_hireVolume_write(u_char *pbuffer_write, WRITE_ACTION_TYPE write_action_type, std::string filename);
  
  //POC FUNTION
  std::string _make_poc_random_node_index_hash(const std::string &hire_block, const std::string &hire_nonce, const std::string &hire_block_hash);
  bool _load_B0_Bn1_Bn(u_int64_t random_node_index, data_node &data_node_zero, MerkleTree &merkletree_cur, MerkleTree &merkletree_pre);
  bool _add_B0_Bn1_Bn_to_poc(u_int64_t random_node_index, data_node &data_node_zero, MerkleTree &merkletree_cur, MerkleTree &merkletree_pre, std::string &hire_volume_poc);
  void _get_merkletree_node(u_int64_t level_index, u_int64_t &node_index_a, u_int64_t &node_index_b, u_int64_t &node_index_f);
  bool _new_merkletree_from_topmost_merkletree(u_int64_t node_index_begin, u_int64_t node_index_end, MerkleTree &merkletree_topmost, MerkleTree &new_merkletree);
  bool _make_volume_poc_merkletree_path(u_int64_t node_index_begin, MerkleTree &merkletree_topmost, u_int64_t random_node_index, MerkleTree &merkletree_cur, std::string &hire_volume_poc);
  bool _get_node_index_from_str(const std::string &hire_rent_index, u_int64_t &node_index_begin, u_int64_t &node_index_end);

  //VOLUME PACKAGE POC
  u_int64_t _make_volume_package_poc_random_node_index(u_int64_t hire_package_volume_free, const std::string &hire_block, const std::string &hire_nonce, const std::string &hire_block_hash);
  void _make_volume_package_poc_head(u_int64_t hire_package_volume_free, u_int64_t random_node_index, const std::string &hire_block, const std::string &hire_nonce, const std::string &hire_block_hash, std::string &hire_package_volume_poc_head);
  bool _get_volume_package_poc(u_int64_t random_node_index, MerkleTree &merkletree_topmost, std::string &hire_volume_package_poc, bool need_true);

  //VOLUME RENT POC
  u_int64_t _make_volume_rent_poc_random_node_index(const std::string &hire_rent_index, const std::string &hire_block, const std::string &hire_nonce, const std::string &hire_block_hash);
  void _make_volume_rent_poc_head(HIRE_RENT_ITEM &hire_rent_item, u_int64_t random_node_index, const std::string &hire_block, const std::string &hire_nonce, const std::string &hire_block_hash, std::string &hire_volume_rent_poc_head);
  bool _get_volume_rent_poc(HIRE_RENT_ITEM &hire_rent_item, u_int64_t random_node_index, std::string &hire_volume_rent_poc);
  bool _check_hire_rent_merkletree(MerkleTree &merkletree_topmost, MerkleTree &rent_merkletree_topmost, const std::string &hire_rent_volume, const std::string &hire_rent_index);
  bool _add_hire_rent_item(const std::string &hire_block_number, const std::string &hire_rent_voucher, const std::string &hire_rent_volume, const std::string &hire_rent_index, const std::string &hire_rent_addr, u_int64_t hire_rent_time, bool bhire_rent_onpledge, bool check_merkletree, bool balready_volume_partitioning);
  bool _del_hire_rent_item(const std::string &hire_rent_voucher);
  void _clear_hire_rent_items();
  bool _has_rent_item_outpledge();

  //提交POC的空间项分组
  void _poc_group_items_to_poc_collection(FJson::Value &root, std::vector<HIRE_RENT_ITEM> &hire_poc_group_items, std::string &hire_poc_collection);
  void _clear_hire_poc_groups();
  std::string _get_volume_poc_group_items_by_item_voucher_in_group(const std::string &item_voucher, std::vector<HIRE_RENT_ITEM> &hire_poc_group_items);
private:
  bool _stop_flag;

  MerkleTree _merkletree_topmost;
  HIRE_STATUS _hire_status;
  std::string _hire_address;
  
  std::string _volume_package_hire_blockchain_number_hash;
  std::string _volume_package_hire_blockchain_number;
  std::string _volume_package_hire_blockchain_nonce;
  std::string _volume_package_filename_prefix;
  u_int64_t _hire_package_volume_expect;
  u_int64_t _hire_package_volume_actual;
  u_int64_t _hire_package_volume_actual_write;
  
  u_int _remaining_time;
  bool _stop_volume_package_flag;

  thread_worker _hirevolume_handle_thread;
  cond_event _hire_volume_event;
  msg_queue<std::string> _hirevolume_msg_queue;

  thread_worker _hirevolume_write_handle_thread;
  cond_event _hirevolume_write_event;
  cond_event _hirevolume_write_free_event;
  msg_queue<WRITE_MESSAGE> _hirevolume_write_msg_queue;
  memery_pool<data_block, DATA_BLOCK_SIZE * 4> _hirevolume_write_msg_pool;
  u_int _queue_size;
  
  //租用空间条目
  std::map<std::string, HIRE_RENT_ITEM> _hire_rent_items;

  //需要提交poc的空间分组条目
  std::map<std::string, std::vector<HIRE_RENT_ITEM>> _hire_poc_groups;

  std::string _volume_poc;
  std::string _volume_poc_bak;
};
#endif //HIRE_POC_MANAGER_H
