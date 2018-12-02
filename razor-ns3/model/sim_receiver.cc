#include<utility>
#include<string.h>
#include"sim_receiver.h"
#include"razor_header.h"
#include"ns3/nstime.h"
#include "ns3/packet.h"
#include"ns3/log.h"
#include "sim_constants.h"
#include <string>
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("SimReceiver");
SimReceiver::SimReceiver()
{
	m_bufferSize=1024;
	m_frameBuffer.resize(m_bufferSize);
	m_minBitRate=SIM_MIN_BITRATE;
	m_maxBitRate=SIM_MAX_BITRATE;

	m_minfid=0;
	m_maxfid=0;
	m_framets=0;
	m_playts=0;
	m_maxts=100;
	m_frameTime=100;
	m_state=buffer_waiting;
	m_recvFirstPacket=true;
	m_statsDataLen=0;
	m_rateStatsInterval=1000;
	//m_lastStatsTime=0;
	m_heartbeat=5;
	uint8_t header_len=sizeof(sim_header_t)+sizeof(sim_segment_t);
	m_cc=razor_receiver_create(m_minBitRate,m_maxBitRate,
			header_len,(void*)this,&SimReceiver::SendFeedback);
}
SimReceiver::~SimReceiver()
{
	if(m_cc!=NULL)
	{
		razor_receiver_destroy(m_cc);
		m_cc=NULL;
	}
	if(!m_heartbeatEvent.IsExpired())
	{
		m_heartbeatEvent.Cancel();
	}
	if(!m_statsRateEvent.IsExpired())
	{
		m_statsRateEvent.Cancel();
	}
	
}
void SimReceiver::Reset()
{
	m_minfid=0;
	m_maxfid=0;
	m_framets=0;
	m_playts=0;
	m_maxts=100;
	m_frameTime=100;
	m_state=buffer_waiting;
	m_statsDataLen=0;
	//m_lastStatsTime=0;
	m_recvFirstPacket=true;
	if(m_cc!=NULL)
	{
		razor_receiver_destroy(m_cc);
		m_cc=NULL;
	}
	uint8_t header_len=sizeof(sim_header_t)+sizeof(sim_segment_t);
	m_cc=razor_receiver_create(m_minBitRate,m_maxBitRate,
			header_len,(void*)this,&SimReceiver::SendFeedback);
	if(!m_heartbeatEvent.IsExpired())
	{
		m_heartbeatEvent.Cancel();
	}
}
void SimReceiver::SetBitrate(uint32_t minBitRate,uint32_t maxBitRate)
{
	m_minBitRate=minBitRate;
	m_maxBitRate=maxBitRate;
	Reset();
}
void SimReceiver::SetHeader(sim_header_t sheader)
{
	m_sHeader=sheader;
	m_sHeader.mid=RazorProtoType::MIN_MSG_ID;
}
void SimReceiver::SetFeedBack(Callback<void,Ptr<Packet>>feedback)
{
	m_feedback=feedback;
}
void SimReceiver::SendFeedback(void*handler,const uint8_t* payload, int payload_size)
{
	//NS_LOG_FUNCTION(Simulator::Now().GetMilliSeconds());
	SimReceiver *obj=static_cast<SimReceiver*>(handler);
	sim_header_t sHeader=obj->m_sHeader;
	sHeader.mid=RazorProtoType::SIM_FEEDBACK;
	NS_ASSERT(payload_size<=SIM_FEEDBACK_SIZE);
	sim_feedback_t feedback;
	feedback.base_packet_id=0;
	feedback.feedback_size=payload_size;
	memcpy(&feedback.feedback,payload,payload_size);
	RazorHeader header;
	header.SetHeader(sHeader);
	header.SetMessagePayload((void*)&feedback,sizeof(sim_feedback_t));
	if(!obj->m_feedback.IsNull())
	{
		//NS_LOG_INFO("send feedback");
		Ptr<Packet> packet=Create<Packet>(0);
		packet->AddHeader(header);
		obj->m_feedback(packet);
	}
}
void SimReceiver::RecvFakeData(sim_segment_t seg)
{
	//NS_LOG_FUNCTION(Simulator::Now().GetMilliSeconds()<<"receive "<<seg.packet_id);
	if(seg.packet_id!=m_base_seq+1){
		uint32_t i=0;
		uint32_t from=m_base_seq+1;
		uint32_t to=seg.packet_id;		
		for(i=from;i<to;i++){
			if(!m_lossSeqCb.IsNull()){
				m_lossSeqCb(i);
			}
		}		
		}
	m_base_seq=seg.packet_id;
	/*if(!m_lossSeqCb.IsNull()){
		m_lossSeqCb(seg.packet_id);
	}*/
	uint8_t  header_len=sizeof(sim_header_t)+sizeof(sim_segment_t);
	if(m_cc!=NULL)
	{
		m_cc->on_received(m_cc,seg.transport_seq, seg.timestamp + seg.send_ts,
				header_len+seg.data_size,seg.remb);
	}
	m_statsDataLen+=(header_len+seg.data_size);
	if(m_recvFirstPacket==true)
	{
		Time interval=MilliSeconds((uint64_t)m_rateStatsInterval);
		m_statsRateEvent=Simulator::Schedule(interval,&SimReceiver::StatsRecvRate,this);
		m_heartbeatEvent=Simulator::ScheduleNow(&SimReceiver::HeartBeat,this);
	}
	m_recvFirstPacket=false;
	return;

}
uint32_t SimReceiver::GetFakeData()
{
	return m_statsDataLen;
}
void SimReceiver::HeartBeat()
{
	//NS_LOG_FUNCTION_NOARGS();
	if(m_heartbeatEvent.IsExpired())
	{
		if(m_cc!=NULL)
		{
			m_cc->heartbeat(m_cc);
		}
	m_heartbeatEvent.Cancel();	
	Time heartbeat=MilliSeconds((uint64_t)m_heartbeat);
		m_heartbeatEvent=Simulator::Schedule(heartbeat,&SimReceiver::HeartBeat,this);
	}
	return;
}
void SimReceiver::StatsRecvRate()
{
	double rate=0.0;
	if(m_statsRateEvent.IsExpired())
	{
		rate=m_statsDataLen*8/(m_rateStatsInterval*0.001);
		//NS_LOG_INFO("receive bitrate "<<rate);
		m_statsDataLen=0.0;
		Time interval=MilliSeconds((uint64_t)m_rateStatsInterval);
		m_statsRateEvent=Simulator::Schedule(interval,&SimReceiver::StatsRecvRate,this);

	}
}
void SimReceiver::UpdateRtt(uint32_t rtt,uint32_t rtt_var)
{
	m_cc->update_rtt(m_cc,rtt+rtt_var);
}
}
