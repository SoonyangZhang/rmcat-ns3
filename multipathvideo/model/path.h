/*
 * path.h
 *
 *  Created on: 2018Äê5ÔÂ13ÈÕ
 *      Author: zsy
 */

#ifndef PATH_H_
#define PATH_H_
#include "ns3/simulator.h"
#include "ns3/event-id.h"
#include "mpcommon.h"
#include <deque>
#include <map>
namespace ns3
{
class MpSender;
//should generate some fake Packet(ftype=0x80) or fec(for later) for bw probe
class Path
{
public:
	const static double RATE_GAIN_CYCLE[];
	Path(PathAddressInfo info,uint32_t pathIndex,MpSender *sender);//create socket
	~Path();//for delete
	void InitRate(uint32_t minRate,uint32_t maxRate);
	void Connect();
	void StartPace();
	void AddPacePacket(uint32_t packet_id, int8_t retrans,uint32_t size);//including header length
	void OnFeedBack(uint8_t *feedback,int len);//feed back rate
	void ProcessMessage(sim_header_t &sHeader,RazorHeader&header);
	void ProcessPing(sim_header_t sheader,sim_ping_t ping);
	void ProcessPong(sim_header_t sheader,sim_pong_t pong);
	void CalculateRtt(uint32_t keep_rtt);
	int64_t GetPaceQueueMs();//queue milleseconds
	uint8_t GetPathState();
	void PaceCallback();
	uint16_t GetTransSeq(){return m_transSeq++;}
private:
	void IncreaseBudget(uint32_t delta_ts);
	void UseBudget(uint32_t bytes);
	uint8_t m_cycleIndex{0};
	double m_currentGain;
	EventId m_paceTimer;
	uint32_t m_paceGap{5};
	EventId m_heartBeat;
	EventId m_rxConEvent;
	uint16_t m_transSeq{0};
	std::map<uint32_t,packet_history_t> m_history;// mark if the packet in bwprobe state
	std::deque<pace_packet_t> m_paceQueue;
	uint64_t m_total{0};
	PathControlBlock m_pcb;
	uint8_t m_pathState{path_idle};
	bool m_bwProbe{false};
	PathAddressInfo m_info;
	MpSender *m_sender;
	int64_t m_lastUpdate;//for pacing to increase budget
	int64_t m_cycleTs;//for cycle update
	int32_t m_byteRemaining;
	bool m_firstPacket{false};
	uint32_t m_minRate;//bps
	uint32_t m_maxRate;
	bool m_firstRtt{true};
	uint32_t m_rtt;
	uint32_t m_rtt_var;
	uint32_t m_pathId{0};
	//cf_unwrapper_t m_wrap;//init_unwrapper16(&m_wrap)
};
}
#endif /* PATH_H_ */
