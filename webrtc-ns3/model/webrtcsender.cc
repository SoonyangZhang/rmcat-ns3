#include "webrtcsender.h"
#include "webrtcheader.h"
#include "ns3/log.h"
#include "processmodule.h"
#include"rtc_base/timeutils.h"//for test
#include<string>
#include "stdio.h"
//fisrst AddPacket then OnSentPacket
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("WebrtcSender");
const uint32_t kInitialBitrateBps = 500000;
void TraceBitrate(uint32_t bps)
{
	NS_LOG_INFO("trace bitrate webrtc "<<bps);
}
WebrtcSender::WebrtcSender()
{
}
WebrtcSender::~WebrtcSender(){}
void WebrtcSender::InitialSetup(Ipv4Address dest_ip,uint16_t dest_port)
{
	m_peerIP=dest_ip;
	m_peerPort=dest_port;
	m_pm=new ProcessModule();
	//webrtc::test::ScopedFieldTrials override_field_trials(
      //"WebRTC-SendSideBwe-WithOverhead/Enabled/");
	m_packetGenerator.SetTarget(this);
	m_send_bucket=(new webrtc::PacedSender(&m_clock, this, nullptr));
    //m_send_bucket->SetPacingRates(kInitialBitrateBps, 0);
    m_controller=new webrtc::SendSideCongestionController(&m_clock,&m_packetGenerator,
    		&m_nullLog,m_send_bucket);
   //m_controller->SetTraceBitrateFun(&TraceBitrate);
	//kInitialBitrateBps
   m_controller->SetBweBitrates(WEBRTC_MIN_BITRATE, kInitialBitrateBps, 10 * kInitialBitrateBps);
   // m_controller->SetTransportOverhead(header_len);//deprecated
	m_send_bucket->SetEstimatedBitrate(kInitialBitrateBps);
        m_send_bucket->SetProbingEnabled(false);
	m_pm->RegisterModule(m_send_bucket,RTC_FROM_HERE);
	m_pm->RegisterModule(m_controller,RTC_FROM_HERE);
	NS_LOG_FUNCTION(m_pm->GetSize());
}
void WebrtcSender::Connect()
{
	if(m_pathState==path_idle||m_pathState==path_disconnected
			||m_pathState==path_connecting)
	{
		WebrtcHeader header;
		header.SetMid(ProtoType::PT_CONNECT);
		header.SetUid(m_uid);
		proto_connect_t con;
		con.cid=m_cid;
		m_pathState=path_connecting;
		header.SetMessagePayload((void*)&con,sizeof(con));
		Ptr<Packet> p=Create<Packet>(0);
		p->AddHeader(header);
		Send(p);
		Time next=MilliSeconds(m_rtt);//100ms
		m_rxTimer=Simulator::Schedule(next,&WebrtcSender::Connect,this);
	}
	if(m_pathState==path_connected)
	{
		m_rxTimer.Cancel();
	}
}
void WebrtcSender::Stop()
{

}
void WebrtcSender::TargetReceive(uint32_t len)
{
	//NS_LOG_FUNCTION("byte "<<len);
	uint32_t i=0;
	uint16_t splits[m_maxSplitPackets];
	uint16_t npacket=0;
	npacket=FrameSplit(splits,len);
	int64_t now=Simulator::Now().GetMilliSeconds();
	uint32_t timestamp;
	proto_segment_t seg;
	m_frameId++;
	if(m_first_ts==-1)
	{
		timestamp=0;
		m_first_ts=now;
	}else
	{
		timestamp=(uint32_t)(now-m_first_ts);
	}
	for(i=0;i<npacket;i++)
	{
	seg.packet_id=m_packetId;
	seg.fid=m_frameId;
	seg.ftype=0;//ftype
	seg.index=i;
	seg.total=npacket;
	seg.remb=0;
	seg.send_ts=0;
	seg.timestamp=timestamp;
	seg.transport_seq=m_sequenceNumber;
	seg.data_size=splits[i];
	m_cache.insert(std::make_pair(seg.transport_seq,seg));
    m_send_bucket->InsertPacket(webrtc::PacedSender::kNormalPriority, m_uid, m_sequenceNumber, now,seg.data_size, false);
	m_sequenceNumber++;
	m_packetId++;
	}
	
}
void WebrtcSender::RecvPacket(Ptr<Socket> socket)
{
    	if(!m_running){return;}
	Address remoteAddr;
    	auto packet = m_socket->RecvFrom (remoteAddr);
	WebrtcHeader header;
	packet->RemoveHeader(header);
	uint8_t mid=header.GetMid();
        int64_t now=Simulator::Now().GetMilliSeconds();
	switch(mid)
	{
	case PT_CONNECT_ACK:
	{
		proto_connect_ack_t ack;
		header.GetMessage((void*)&ack,sizeof(ack));
		ProcessConnectAck(ack);
	}
	break;
	case PT_FEEDBACK:
	{
		uint8_t buffer[packet->GetSize()];
		packet->CopyData ((uint8_t*)&buffer, packet->GetSize ());
		std::unique_ptr<webrtc::rtcp::TransportFeedback>feedback=
		webrtc::rtcp::TransportFeedback::ParseFrom((uint8_t*)&buffer, packet->GetSize ());
		//NS_LOG_INFO(now<<" feedback "<<packet->GetSize()<<" "<<feedback->GetBaseSequence());
		if(m_controller)
		{
		m_controller->OnTransportFeedback(*feedback.get());
		}
		
	}
	break;
	case PT_PING:
	{
    	proto_ping_t ping;
    	header.GetMessage((void*)&ping,sizeof(ping));
    	ProcessPing(ping);
	}
	break;
	case PT_PONG:
	{
    	proto_pong_t pong;
    	header.GetMessage((void*)&pong,sizeof(pong));
    	ProcessPong(pong);
	}
	break;
	default:
		NS_LOG_FUNCTION("MID "<<mid);
	break;
	}
}
void WebrtcSender::ProcessConnectAck(proto_connect_ack_t ack)
{
	if(m_pathState==path_connecting)
	{
		m_pathState=path_connected;
		//NS_LOG_FUNCTION("connected");
		m_rxTimer.Cancel();
		m_packetGenerator.Start();
		m_pm->Start();
		m_pingTimer=Simulator::ScheduleNow(&WebrtcSender::SendPing,this);
	}
}
void WebrtcSender::ProcessPing(proto_ping_t ping)
{
	WebrtcHeader header;
	header.SetMid(ProtoType::PT_PONG);
	header.SetUid(m_uid);
	header.SetMessagePayload((void*)&ping,sizeof(ping));
	Ptr<Packet> p=Create<Packet>(0);
	p->AddHeader(header);
	Send(p);
}
void WebrtcSender::ProcessPong(proto_pong_t pong)
{
	int64_t now_ts=Simulator::Now().GetMilliSeconds();
	uint64_t keep_rtt=5;
	uint64_t averageRtt=0;
	if (now_ts > pong.ts + 5)
		keep_rtt = (uint32_t)(now_ts - pong.ts);
	m_rtt=keep_rtt;
	if(m_numRtt==0)
	{
		m_controller->OnRttUpdate(keep_rtt,keep_rtt);
		m_sumRtt=keep_rtt;
		m_maxRtt=keep_rtt;
		m_numRtt++;
		return;
	}
	m_numRtt+=1;
	m_sumRtt+=keep_rtt;
	if(keep_rtt>m_maxRtt)
	{
		m_maxRtt=keep_rtt;
	}
	averageRtt=m_sumRtt/m_numRtt;
	m_controller->OnRttUpdate(averageRtt,m_maxRtt);
}
uint16_t WebrtcSender::FrameSplit(uint16_t splits[],uint32_t size)
{
	uint16_t ret, i;
	uint16_t remain_size;
	NS_ASSERT((size/m_segmentSize)<m_maxSplitPackets);
	if(size<m_segmentSize)
	{
		ret=1;
		splits[0]=size;
	}else
	{
		ret=size/m_segmentSize;
		for(i=0;i<ret;i++)
		{
			splits[i]=m_segmentSize;
		}
		remain_size=size%m_segmentSize;
		if(remain_size>0)
		{
			splits[ret]=remain_size;
			ret++;
		}
	}
	return ret;
}
void WebrtcSender::Send(Ptr<Packet>packet)
{
	if(m_socket==NULL)
	{
		NS_LOG_ERROR("socket  is null,cant send data");
		return ;
	}
	m_socket->SendTo(packet,0,InetSocketAddress{m_peerIP,m_peerPort});
}
bool WebrtcSender::TimeToSendPacket(uint32_t ssrc,
                                  uint16_t sequence_number,
                                  int64_t capture_time_ms,
                                  bool retransmission,
                                  const webrtc::PacedPacketInfo& cluster_info)
								  {
	int64_t now=rtc::TimeMillis();//Simulator::Now().GetMilliSeconds();
	//NS_LOG_INFO(now<<" "<<capture_time_ms<<"pace sender "<<ssrc<<" "<<sequence_number);
	std::map<uint16_t,proto_segment_t>::iterator iter;
	iter=m_cache.find(sequence_number);
	if(iter==m_cache.end())
	{
		NS_LOG_ERROR("packet not found"<<sequence_number);
		return true;
	}
	uint8_t header_len=sizeof(proto_header_t)+sizeof(proto_segment_t);
	proto_segment_t segment=iter->second;
	segment.send_ts=(uint16_t)(Simulator::Now().GetMilliSeconds()-m_first_ts-segment.timestamp);
	m_cache.erase(iter);
	m_controller->AddPacket(m_uid,sequence_number,segment.data_size+header_len,webrtc::PacedPacketInfo());
	WebrtcHeader header;
	header.SetUid(m_uid);
	header.SetMid(ProtoType::PT_SEG);
	header.SetMessagePayload((void*)&segment,sizeof(segment));
	Ptr<Packet> packet=Create<Packet>((uint32_t)segment.data_size);
	packet->AddHeader(header);
	Send(packet);
	rtc::SentPacket sentPacket((int64_t)sequence_number,now);
	m_controller->OnSentPacket(sentPacket);
	return true;							  
}
size_t WebrtcSender::TimeToSendPadding(size_t bytes,
                                     const webrtc::PacedPacketInfo& cluster_info)
									 {
NS_LOG_INFO("send padding "<<bytes);
	return bytes;
									 }
void  WebrtcSender::StartApplication()
{
    if (m_socket == NULL) {
        m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
        auto res = m_socket->Bind ();
        NS_ASSERT (res == 0);
    }
    m_running=true;
    m_socket->SetRecvCallback (MakeCallback(&WebrtcSender::RecvPacket,this));
    Connect();
}
void WebrtcSender::StopApplication()
{
	m_running=false;
	m_pm->Stop();
	m_pingTimer.Cancel();
	m_packetGenerator.Stop();
	m_pm->DeRegisterModule(m_send_bucket);
	m_pm->DeRegisterModule(m_controller);
	delete m_send_bucket;
	delete m_controller;
	delete m_pm;
}
void WebrtcSender::SendPing(void)
{
	if(m_pingTimer.IsExpired())
	{
		proto_ping_t ping;
		ping.ts=Simulator::Now().GetMilliSeconds();
		WebrtcHeader header;
		header.SetMid(ProtoType::PT_PING);
		header.SetUid(m_uid);
		header.SetMessagePayload((void*)&ping,sizeof(ping));
		Ptr<Packet> p=Create<Packet>(0);
		p->AddHeader(header);
		Send(p);
		Time next=MilliSeconds(m_rtt);
		m_pingTimer=Simulator::Schedule(next,&WebrtcSender::SendPing,this);
	}
}
}
