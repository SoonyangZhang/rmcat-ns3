/*
 * sim_sender.h
 *
 *  Created on: 2018.4.26
 *      Author: zsy
 */

#ifndef SIM_SENDER_H
#define SIM_SENDER_H
#include "razor_header.h"
#include "razor_api.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/simulator.h"
#include<map>
namespace ns3
{
class SimSender
{
public:
	SimSender();
	~SimSender();
	void Reset();
	void SetHeader(sim_header_t header);
	void UpdateRtt(uint32_t rtt,uint32_t rtt_var);
	void ProcessFeedBack(sim_feedback_t feedback);
	void SetBitRates(uint32_t min_bitrate, uint32_t start_bitrate, uint32_t max_bitrate);
	uint16_t FrameSplit(uint16_t splits[],uint32_t size);
	void PutFakeData(uint8_t ftype,uint32_t size);
	void SetSendSegmentCallback(Callback<void,Ptr<Packet>>sendSegemnt);
	void SetVideoBitChangeCallback(Callback<void,uint32_t> changeBitRate);
	static void ChangeBitRate(void* trigger, uint32_t bitrate, uint8_t fraction_loss, uint32_t rtt);
	static void PacePacketSend(void* handler, uint32_t packet_id, int retrans, size_t size);
	void PaceStart();
	void PaceHeartBeat();
	void SetSegmentSize(uint32_t segmentSize){m_segmentSize=segmentSize;}
	void SetPaceQueueLen(uint32_t queueLen){m_paceQueueLen=queueLen;}
private:
	sim_header_t m_header;
	uint32_t m_frame_id;
	uint32_t m_packet_id;
	uint16_t m_transport_seq;
	bool m_active;
	int64_t m_first_ts;
	uint8_t m_remb{0};
	uint32_t m_paceTime;// in 5 milliseconds;
	EventId m_paceEvent;
	uint32_t m_paceQueueLen;//in 300 milliseconds;
	uint32_t m_maxSplitPackets;
	uint32_t m_segmentSize;
	uint8_t m_lossFraction;
	razor_sender_t *m_cc;//sender side congestion control
	std::map<uint32_t,sim_segment_t> m_cache;
private:
	Callback<void,Ptr<Packet>> m_sendPacket;
	Callback<void,uint32_t>m_changeBitRate;
};
}
#endif /* SIM_SENDER_H */
