/*
 * path.cc
 *
 *  Created on: 2018Äê5ÔÂ13ÈÕ
 *      Author: zsy
 */
#include "mpsender.h"
#include "path.h"
#include "ns3/log.h"
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("Path");
const double Path::RATE_GAIN_CYCLE [] = {5.0 / 4, 3.0 / 4, 1, 1, 1, 1, 1, 1};
Path::Path(PathAddressInfo info,uint32_t pathIndex,MpSender *sender)
{
	m_info=info;
	m_pathId=pathIndex;
	m_sender=sender;
}
Path::~Path()
{
	if(!m_heartBeat.IsExpired())
	{
		m_heartBeat.Cancel();
	}
	if(!m_paceTimer.IsExpired())
	{
		m_paceTimer.Cancel();
	}	

}
void Path::Connect()
{
	if(m_pathState==path_idle||m_pathState==path_disconnected
			||m_pathState==path_connecting)
	{
		RazorHeader header;
		sim_header_t sHeader;
		uint32_t uid=m_sender->GetUid();
		uint32_t cid=m_sender->GetCid();
		INIT_SIM_HEADER(sHeader,RazorProtoType::SIM_CONNECT,uid);
		m_pathState=path_connecting;
		header.SetHeader(sHeader);
		sim_connect_t con;
		con.cid=cid;
		con.token_size=1;
		con.token[0]=123;
		header.SetMessagePayload((void*)&con,sizeof(sim_connect_t));
		Ptr<Packet> p=Create<Packet>(0);
		p->AddHeader(header);
		m_sender->OnPacket(m_info,p);
		Time next=MilliSeconds((uint64_t)100);//100ms
		m_rxConEvent=Simulator::Schedule(next,&Path::Connect,this);
	}
	if(m_pathState==path_connected)
	{
		m_rxConEvent.Cancel();
	}
}
void Path::InitRate(uint32_t minRate,uint32_t maxRate)
{
	m_minRate=minRate;
	m_maxRate=maxRate;
}
void Path::AddPacePacket(uint32_t packet_id, int8_t retrans,uint32_t size)
{
	pace_packet_t que;
	que.seq=packet_id;
	que.retrans=retrans;
	que.que_ts=Simulator::Now().GetMilliSeconds();
	que.len=size;
	m_paceQueue.push_back(que);
	m_total+=size;
}
void Path::StartPace()
{
	NS_LOG_INFO("start pace");
	m_lastUpdate=Simulator::Now().GetMilliSeconds();
	m_paceTimer=Simulator::ScheduleNow(&Path::PaceCallback,this);	
}
void Path::ProcessMessage(sim_header_t &sHeader,RazorHeader&header)
{
	switch(sHeader.mid)
	{
	case SIM_PING:
	{
    	sim_ping_t ping;
    	header.GetMessage((void*)&ping,sizeof(ping));
    	ProcessPing(sHeader,ping);
	}
	break;
	case SIM_PONG:
	{

	}
	break;
	case SIM_CONNECT_ACK:
	{
		m_pathState=path_connected;
		NS_LOG_INFO(m_pathId<<"connected");
		m_rxConEvent.Cancel();

	}
	break;
	default:
		NS_LOG_WARN("message is unrecognizable"<<sHeader.mid);
		break;
	}
}
void Path::ProcessPing(sim_header_t sheader,sim_ping_t ping)
{
	NS_LOG_FUNCTION(this<<" "<<Simulator::Now().GetMilliSeconds());
	RazorHeader header;
	sim_header_t sHeader;
	uint32_t uid=m_sender->GetUid();
	INIT_SIM_HEADER(sHeader,RazorProtoType::SIM_PONG,uid);
	header.SetHeader(sHeader);
	header.SetMessagePayload((void*)&ping,sizeof(ping));
	Ptr<Packet> p=Create<Packet>(0);
	p->AddHeader(header);
	m_sender->OnPacket(m_info,p);
}
void Path::ProcessPong(sim_header_t sheader,sim_pong_t pong)
{
	NS_LOG_FUNCTION(this<<" "<<Simulator::Now().GetMilliSeconds());
	int64_t now_ts=Simulator::Now().GetMilliSeconds();
	uint32_t keep_rtt=5;
	if (now_ts > pong.ts + 5)
		keep_rtt = (uint32_t)(now_ts - pong.ts);
	if(m_firstRtt==false)
	{
	CalculateRtt(keep_rtt);
	}
	if(m_firstRtt==true)
	{
	m_firstRtt=false;
	m_rtt=keep_rtt;
	}
}
void Path::CalculateRtt(uint32_t keep_rtt)
{
	if (keep_rtt < 5)
		keep_rtt = 5;
	m_rtt_var=(3*m_rtt_var+SU_ABS(m_rtt, keep_rtt)) / 4;
	if (m_rtt_var < 10)
		m_rtt_var = 10;
	m_rtt = (7 * m_rtt + keep_rtt) / 8;
	if (m_rtt < 10)
		m_rtt = 10;
	NS_LOG_INFO("rtt "<<m_rtt<<"rtt_var "<<m_rtt_var);
}
void Path::IncreaseBudget(uint32_t delta_ts)
{
	uint32_t byte=RATE_GAIN_CYCLE[m_cycleIndex]*m_minRate*delta_ts/(1000*8);
	NS_LOG_FUNCTION("byte"<<byte);
	NS_LOG_INFO(m_pathId<<" byte "<<byte<<" delta "<<delta_ts<<" minRate "<<m_minRate);
	if(m_byteRemaining<0)
	{
		m_byteRemaining+=byte;
	}else
	{
		m_byteRemaining=byte;
	}
}
void Path::UseBudget(uint32_t bytes)
{
	m_byteRemaining-=bytes;
}
//TODO lack of history record
void Path::PaceCallback()
{
	uint64_t now=Simulator::Now().GetMilliSeconds();
	uint32_t elapsed=now-m_lastUpdate;
	NS_LOG_FUNCTION(now<<"time"<<elapsed);
	IncreaseBudget(elapsed);
	if(m_paceTimer.IsExpired())
	{
		while(!m_paceQueue.empty())
		{
			if(m_byteRemaining>=0)
			{
				pace_packet_t ele=m_paceQueue.front();
				m_paceQueue.pop_front();
				UseBudget(ele.len);
				NS_LOG_INFO(m_pathId<<"pacer send out "<<ele.seq);
				m_total-=ele.len;
				m_sender->OnPacePacket(m_info,ele.seq,m_transSeq,
						ele.retrans,ele.len);
				m_transSeq++;
			}else{
				break;
			}
		}
		//in bw_probe mode,generate fake data when m_paceQueue.empty;
		if(m_paceQueue.empty()&&m_byteRemaining>0&&m_cycleIndex==0)
		{
			m_sender->OnBwProbe(m_info,this,m_byteRemaining);
			UseBudget(m_byteRemaining);
		}
		m_lastUpdate=now;
		Time next=MilliSeconds((uint64_t)m_paceGap);
		m_paceTimer=Simulator::Schedule(next,&Path::PaceCallback,this);
	}
}
void Path::OnFeedBack(uint8_t *feedback,int len)
{
//TODO
}
int64_t Path::GetPaceQueueMs()
{
	//TODO
	return 0;
}
uint8_t Path::GetPathState()
{
	return m_pathState;
}
}



