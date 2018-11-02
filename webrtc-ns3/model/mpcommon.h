#ifndef MP_COMMON_H
#define MP_COMMON_H
//#include "razor_header.h"
#include "ns3/node.h"
#include "ns3/ipv4-address.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include"ns3/nstime.h"
#include <vector>
namespace ns3
{
enum PathSession
{
	path_idle=0x00,
	path_connecting,
	path_connected,
	path_disconnceting,
	path_disconnected,

};
struct PathAddressInfo
{
Ipv4Address srcIp;
Ipv4Address destIp;
uint16_t srcPort;
uint16_t destPort;
};
typedef struct
{
	int64_t			create_ts;
	int64_t			arrival_ts;
	int64_t			send_ts;
	uint16_t		sequence_number;
	uint8_t         be_probe;
	uint32_t		payload_size;
}packet_history_t;
typedef struct
{
uint32_t seq;
int8_t retrans;
uint32_t len;
int64_t que_ts;
}pace_packet_t;
/*
typedef struct
{
sim_segment_t seg;
uint8_t ref;
}segment_ref_t;*/
class PathControlBlock
{
public:
	uint32_t m_paceRate;
	uint32_t m_averageRate;
	uint32_t m_probeRate;
	Time m_minRtt{Time::Max()};
	int64_t m_minRttTs;
	Time m_maxRtt{Time::Min()};
	int64_t m_maxRttTs;
	Time m_queueDrain;//feedback by receiver; m_maxRttTs-m_minRttTs;no use for now
};
#define SU_MAX(a, b)		((a) > (b) ? (a) : (b))
#define SU_MIN(a, b)		((a) < (b) ? (a) : (b))
#define SU_ABS(a, b)		((a) > (b) ? ((a) - (b)) : ((b) - (a)))
void GetNodeAddress(Ptr<Node>node,std::vector<Ipv4Address>&addresses);
bool IsExistPath (Ptr<Node> node,Ipv4Address src, Ipv4Address dst);
struct comp_socket
{
       typedef Ipv4Address value_type;
       bool operator () (const value_type & ls, const value_type &rs) const
       {
    	   return ls<rs;
       }
};
struct comp_path
{
       typedef PathAddressInfo value_type;
       bool operator () (const value_type & ls, const value_type &rs) const
       {
    	   return ls.srcIp<rs.srcIp||(ls.srcIp==rs.srcIp&&ls.destIp<rs.destIp)||
		   (ls.srcIp==rs.srcIp&&ls.destIp==rs.destIp&&ls.srcPort<rs.srcPort)||
		   (ls.srcIp==rs.srcIp&&ls.destIp==rs.destIp&&ls.srcPort==rs.srcPort&&ls.srcPort<rs.destPort);
       }
};
}
#endif
