/*
 * recvpath.h
 *
 *  Created on: 2018Äê5ÔÂ14ÈÕ
 *      Author: zsy
 */

#ifndef RECVPATH_H_
#define RECVPATH_H_
#include "mpcommon.h"
#include"ns3/event-id.h"
#include "ns3/data-rate.h"
namespace ns3
{
struct packet_record_t
{
uint16_t seq;
int64_t ts;
uint32_t len;
};
class MpReceiver;
class ReceivePath
{
public:
	ReceivePath(MpReceiver *receiver,uint32_t pathIndex,PathAddressInfo info);
	~ReceivePath();
	void ProcessMessage(sim_header_t &sHeader,RazorHeader&header);
private:
	void ProcessConnect(sim_connect_t &con);
	void ProcessSegment(sim_segment_t &seg);
	void Feedback();
	EventId m_feedback;
	EventId m_heartbeat;
	//std::map<uint32_t,packet_record_t>;//map uint16_t seq to uint32_t
	uint16_t m_lastValue{0};//for seq update
	DataRate m_rate;
	uint32_t m_peerCid;
	MpReceiver *m_receiver;
	PathAddressInfo m_info;
	uint8_t m_pathState{path_idle};
	uint32_t m_pathId{0};
};
}
#endif /* RECVPATH_H_ */

