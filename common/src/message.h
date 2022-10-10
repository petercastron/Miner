#pragma once

#include "messageHelp.h"
#include "protohdr.h"
#include "struct_sdvn.h"
#include "ip_addr.h"
#include "sa.h"

//define size of gateway header
#ifdef IPV6_SUPPORT
#define SIZE_GW_HDR SIZE_GW_IPV6_HDR
#else
#define SIZE_GW_HDR SIZE_GW_IPV4_HDR
#endif

#define MSG_DATA_LEN ETH_MAX_DATA_SIZE            //1500
#define MSG_LEN ETH_MAX_DATA_SIZE + SIZE_GW_HDR   //1500 + HEAD
#define MIN_UDP_MSG_LEN 22                        //udp packet should bigger than 64Byte :  ETH(14) + IPV4(20) + UDP(8) + DATA(22) = 64 Byte

#define MSG_BLOCK_SIZE  2048       //size of msg
#define MSG_POOL_SIZE   2048*50    //size of pool block

typedef enum _MSG_TYPE
{
  MSG_TYPE_RX = 1,
  MSG_TYPE_TX = 2,
  MSG_TYPE_DNS_PROXY_REQUEST = 3,
  MSG_TYPE_DNS_PROXY_RESPONSE = 4,
} MSG_TYPE;

typedef enum _SDVN_CARE_PROTO
{
  SCP_NULL = 0,
  SCP_ARP = 1,
  SCP_ICMPV4 = 2,
  SCP_ICMPV6 = 3,
  SCP_DNS = 4,
  SCP_TCP = 5,
  SCP_UDP = 6,
  SCP_MULTICAST = 7,
  SCP_BROADCAST = 8,
} SDVN_CARE_PROTO;

#pragma pack(1)
typedef struct _message
{
  u_char type; //rx 1 , tx 2, dns proxy 3,4
  bool access_internet;

  //msg ip and port
  IPADDR data_fromip;    //net order
  u_short data_fromport; //net order

  IPADDR payload_dip;
  u_short payload_dport;

  IPADDR payload_sip;
  u_short payload_sport;

  IPADDR dest_vip;
  IPADDR next_hop;

  AutoSa psa;  //sa

  //以太网帧(L2)
  eth_hdr *peth_hdr; //eth head
  u_short eth_proto; //arp, ipv4, ipv6

  //帧载荷(L3 ip层)
  u_char *ip_hdr;    //ip head
  u_char ip_proto;   //udp, tcp, icmp(for eth_proto = ipv4, ipv6)
  u_char ttl;

  //传输层(L4)
  u_char *udp_tcp_icmp_hdr; //ip data

  u_char todo_proto; //sdvn care proto
  u_char *todo_data; //sdvn care hdr
  
  GW_VEDA_HEADER *veda_hdr_ptr;
  GW_IKE_HEADER *ike_hdr_ptr;
  u_char *msg_data_ptr;

  u_int msg_len;
  u_int msg_data_len;
  u_char dst_mac_addr[ETH_ALEN];
  u_char msg_head[SIZE_GW_HDR];
  u_char msg_data[MSG_DATA_LEN];
} MESSAGE;

typedef struct _block_msg
{
  u_char msg_buffer[MSG_BLOCK_SIZE];
} BLOCK_MSG;
#pragma pack()

typedef memery_pool<BLOCK_MSG, MSG_POOL_SIZE> msg_memery_pool;

