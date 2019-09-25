/*
 * recvpath.cc
 *
 *  Created on: 2018Äê5ÔÂ14ÈÕ
 *      Author: zsy
 */
#include "recvpath.h"
#include "mpreceiver.h"
#include "ns3/log.h"
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("RecvPath");
ReceivePath::ReceivePath(MpReceiver*receiver,uint32_t pathIndex,PathAddressInfo info)
{
	m_receiver=receiver;
	m_pathId=pathIndex;
	m_info=info;
}
ReceivePath::~ReceivePath()
{
	m_receiver=nullptr;
}
void ReceivePath::ProcessMessage(sim_header_t &sHeader,RazorHeader&header)
{
	switch(sHeader.mid)
	{
	case SIM_CONNECT:
	{
    	sim_connect_t con;
    	header.GetMessage((void*)&con,sizeof(con));
    	ProcessConnect(con);
	}
	break;
	case SIM_SEG:
	{
		sim_segment_t seg;
		header.GetMessage((void*)&seg,sizeof(seg));
		ProcessSegment(seg);
	}
	break;
	default:
		NS_LOG_INFO("wait for future processing");
	break;
	}
}
void ReceivePath::ProcessConnect(sim_connect_t &con)
{
	NS_LOG_FUNCTION(this<<" "<<Simulator::Now().GetMilliSeconds());
	m_pathState=path_connected;
	RazorHeader header;
	sim_header_t sHeader;
	uint32_t uid=m_receiver->GetUid();
	m_pathState=path_connected;
	INIT_SIM_HEADER(sHeader,RazorProtoType::SIM_CONNECT_ACK,uid);
	header.SetHeader(sHeader);
	sim_connect_ack_t ack;
	ack.cid=con.cid;
	ack.result=0;
	header.SetMessagePayload((void*)&ack,sizeof(ack));
	Ptr<Packet> p=Create<Packet>(0);
	p->AddHeader(header);
	m_receiver->OnPacketPlugin(m_info,p);

}
void ReceivePath::ProcessSegment(sim_segment_t &seg)
{
	NS_LOG_INFO(m_pathId<<"recv segment "<<seg.packet_id<<" len "<<seg.data_size);
}
void ReceivePath::Feedback()
{

}
}
