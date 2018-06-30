#include "gcc_sender.h"
#include<stdio.h>
#include<string>
#include<sstream>
#include"ns3/log.h"
namespace ns3
{
NS_LOG_COMPONENT_DEFINE ("GCCSENDER");
/*12000bps=1500*8bit/s*/
GccSender::GccSender():m_minPacingRate("12000bps"),
m_curentPacingRate(m_minPacingRate)
{
	uint32_t m_paceQueueLen=300;
	m_paceSender=pace_create((void*)this,(pace_send_func)&GccSender::DataSend,m_paceQueueLen);
	m_pacingTimer.SetFunction(&GccSender::NotifyPacingPerformed,this);
	m_paceBitRate=12000;
	m_sequence=0;
	m_pacingTime=5;
}
GccSender::~GccSender()
{
	pace_destroy(m_paceSender);
}
void GccSender::ChangePacingRate(uint64_t bitrate)
{
    std::stringstream ss;
    ss<<bitrate<<"bps";
    std::string s=ss.str();
	DataRate newval(s);
NS_LOG_INFO("change pacing rate from"<<m_curentPacingRate<<"to"<<newval);
	m_curentPacingRate=newval;
	m_paceBitRate=bitrate;
}
void GccSender::NotifyPacingPerformed()
{
	pace_try_transmit(this->m_paceSender,Simulator::Now().GetMilliSeconds()); 
         //NS_LOG_INFO("NotifyPacingPerformed"<<Simulator::Now().GetMilliSeconds()) ;
	//if(m_pacingTimer.IsExpired ())
	{
	m_pacingTimer.Schedule(MilliSeconds(m_pacingTime));	
	}
	
}	
void GccSender::Start()
{
  NS_LOG_INFO("gcc Sender start"<<m_paceSender->last_update_ts<<""<<Simulator::Now().GetMilliSeconds());
  pace_set_estimate_bitrate(m_paceSender,m_paceBitRate);
  m_pacingTimer.Schedule();
}
void GccSender::InsertPacket(int num,uint32_t length)
{
	//TODO 
	for(int i=0;i<num;i++)
	{
		pace_insert_packet(m_paceSender,m_sequence,0,length,Simulator::Now().GetMilliSeconds());
		m_sequence++;
	}
	
}
void GccSender::DataSend(void* handler, uint32_t packet_id, int retrans, size_t size)
{
	GccSender *obj=static_cast<GccSender*>(handler);
	if(obj->m_pacingTimer.IsExpired ())
	{
		NS_LOG_DEBUG(Simulator::Now().GetSeconds()<<" "<<Simulator::Now().GetMilliSeconds()<<"pacer timer in expire state"<<packet_id<<" "<<size);
		//obj->m_pacingTimer.Schedule(obj->m_curentPacingRate.CalculateBytesTxTime(size));
	}
	
}
}
