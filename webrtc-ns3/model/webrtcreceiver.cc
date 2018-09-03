/*
 * webrtcreceiver.cc
 *
 *  Created on: 2018Äê5ÔÂ20ÈÕ
 *      Author: zsy
 */
#include "rtc_base/buffer.h"
#include "webrtcreceiver.h"
#include "ns3/log.h"
#include "modules/rtp_rtcp/source/rtcp_packet/common_header.h"
#include "webrtclog.h"
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("WebrtcReceiver");
WebrtcReceiver::WebrtcReceiver(){}
WebrtcReceiver::~WebrtcReceiver(){}
void WebrtcReceiver::Init(uint32_t uid,uint32_t cid)
{
	m_uid=uid;
	m_cid=cid;
}
void WebrtcReceiver::Bind(uint16_t port)
{
	m_port=port;
    m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
    auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
    auto ret = m_socket->Bind (local);
    NS_ASSERT (ret == 0);
    m_socket->SetRecvCallback (MakeCallback (&WebrtcReceiver::RecvPacket,this));
}
void WebrtcReceiver::AddSendRtpModule(webrtc::RtpRtcp* rtp_module, bool remb_candidate)
{
	NS_LOG_FUNCTION("no");
}
void WebrtcReceiver::RemoveSendRtpModule(webrtc::RtpRtcp* rtp_module)
{
	NS_LOG_FUNCTION("no");
}
void WebrtcReceiver::AddReceiveRtpModule(webrtc::RtpRtcp* rtp_module, bool remb_candidate)
//void WebrtcReceiver::AddReceiveRtpModule(webrtc::RtcpFeedbackSenderInterface* rtcp_sender,
//                           bool remb_candidate)
{
	NS_LOG_FUNCTION("no");
}
void WebrtcReceiver::RemoveReceiveRtpModule(webrtc::RtpRtcp* rtp_module)
//void WebrtcReceiver::RemoveReceiveRtpModule(webrtc::RtcpFeedbackSenderInterface* rtcp_sender)
{
	NS_LOG_FUNCTION("no");
}
bool WebrtcReceiver::TimeToSendPacket(uint32_t ssrc,
                        uint16_t sequence_number,
                        int64_t capture_timestamp,
                        bool retransmission,
                        const webrtc::PacedPacketInfo& packet_info)
{
	NS_LOG_FUNCTION("no");
	return true;
}
size_t WebrtcReceiver::TimeToSendPadding(size_t bytes,
                           const webrtc::PacedPacketInfo& packet_info)
{
	NS_LOG_FUNCTION("no");
	return bytes;
}
void WebrtcReceiver::SetTransportWideSequenceNumber(uint16_t sequence_number)
{
	NS_LOG_FUNCTION("no");
}
uint16_t WebrtcReceiver::AllocateSequenceNumber()
{
	NS_LOG_FUNCTION("no");
	return 0;
}
void WebrtcReceiver::OnReceiveBitrateChanged(const std::vector<uint32_t>& ssrcs,
                               uint32_t bitrate_bps)
{
	NS_LOG_FUNCTION("no");
}
void WebrtcReceiver::SetMaxDesiredReceiveBitrate(int64_t bitrate_bps)
{
	NS_LOG_FUNCTION("no");
}
bool WebrtcReceiver::SendRemb(int64_t bitrate_bps, const std::vector<uint32_t>& ssrcs)
{
	NS_LOG_FUNCTION("no");
	return true;
}
bool WebrtcReceiver::SendTransportFeedback(webrtc::rtcp::TransportFeedback* packet)
{
	packet->SetSenderSsrc(m_uid);
	rtc::Buffer serialized= packet->Build();
	int64_t now=Simulator::Now().GetMilliSeconds();
	Ptr<Packet> p=Create<Packet>((uint8_t*)serialized.data(),serialized.size());
	std::unique_ptr<webrtc::rtcp::TransportFeedback> feedback=webrtc::rtcp::TransportFeedback::ParseFrom((uint8_t*)serialized.data(),serialized.size());
	webrtc::rtcp::CommonHeader rtcpHeader;
	rtcpHeader.Parse((uint8_t*)serialized.data(),serialized.size());
	//uint8_t *ptr=(uint8_t*)serialized.data();
	//uint8_t fmt=ptr[0] & 0x1F;//rtcpHeader.fmt();
	//uint8_t type=rtcpHeader.type();
	//printf("fmt%u type%u \n",fmt,type);
	uint32_t media_ssrc=feedback->media_ssrc();
	uint32_t sender_ssrc=feedback->sender_ssrc();
	/*for(const auto receive:feedback->GetReceivedPackets())
	{
	NS_LOG_FUNCTION("receive seq "<<receive.sequence_number()<<"delta "<<receive.delta_us());	
	}*/
	//RTC_LOG(LS_INFO)
	//NS_LOG_FUNCTION("size "<<rtcpHeader.payload_size_bytes());
	//NS_LOG_FUNCTION("ssrc "<<sender_ssrc<<"media_ssrc "<<media_ssrc);
	WebrtcHeader header;
        //NS_LOG_FUNCTION(now<<" feedback "<<serialized.size()<<" "<<packet->GetBaseSequence());
	header.SetMid(ProtoType::PT_FEEDBACK);
	header.SetUid(m_uid);
	p->AddHeader(header);
	Send(p);
	return true;
}
void WebrtcReceiver::Send(Ptr<Packet>packet)
{
	if(m_socket==NULL)
	{
		NS_LOG_ERROR("socket  is null,cant send data");
		return ;
	}
	m_socket->SendTo(packet,0,InetSocketAddress{m_peerIp,m_peerPort});
}
void WebrtcReceiver::RecvPacket(Ptr<Socket> socket)
{
    	if(!m_running)
	{return;}
	Address remoteAddr;
    	auto packet = m_socket->RecvFrom (remoteAddr);
	m_peerIp = InetSocketAddress::ConvertFrom (remoteAddr).GetIpv4 ();
	m_peerPort = InetSocketAddress::ConvertFrom (remoteAddr).GetPort ();
	WebrtcHeader header;
	packet->RemoveHeader(header);
	m_peerUid=header.GetUid();
	uint8_t mid=header.GetMid();
	int64_t now=Simulator::Now().GetMilliSeconds();
	switch(mid)
	{
	case PT_CONNECT:
	{
		//NS_LOG_FUNCTION("receive connect");
		proto_connect_t con;
		header.GetMessage((void*)&con,sizeof(con));
		ProcessConnect(con);
	}
	break;
	case PT_SEG:
	{
		proto_segment_t seg;
		uint32_t overhead=0;
		header.GetMessage((void*)&seg,sizeof(seg));
		//NS_LOG_FUNCTION(now<<"receive seg"<<seg.transport_seq);
		overhead=sizeof(proto_header_t)+sizeof(proto_segment_t)+seg.data_size;
		if(!m_firstPacket)
		{
			//if(seg.transport_seq!=m_lastPacket+1)
			//NS_LOG_FUNCTION("packet lost"<<m_lastPacket+1);
		}
		if(m_firstPacket)
		{
			m_firstPacket=false;
			m_controller.reset(new webrtc::ReceiveSideCongestionController(
					&m_clock,this));
			m_rbe=m_controller->GetRemoteBitrateEstimator(true);//sbe;
			m_pm=new ProcessModule();
			std::string fileAndLine;
		    fileAndLine=std::string("webrtc_receiver")+std::string("171");
			rtc::Location location(__FUNCTION__,fileAndLine.c_str());
			m_pm->RegisterModule(m_rbe,location);
			m_pm->RegisterModule(m_controller.get(),location);
			NS_LOG_FUNCTION(m_pm->GetSize());
			m_pm->Start();
		}
		if(m_sender!=0&&!m_delayCb.IsNull())
		{
		uint64_t tmp=seg.send_ts+seg.timestamp+m_sender->GetFirstTs();
		uint64_t gap=now-tmp;
		m_delayCb(gap);
		}
		m_lastPacket=seg.transport_seq;
		webrtc::RTPHeader fakeHeader;
		fakeHeader.ssrc=header.GetUid();
		fakeHeader.extension.hasTransportSequenceNumber=true;
		fakeHeader.extension.transportSequenceNumber=seg.transport_seq;
		m_controller->OnReceivedPacket(now,overhead,fakeHeader);
	}
	break;
	case PT_PING:
	{
		//NS_LOG_FUNCTION("receive ping");
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
	}
}
void WebrtcReceiver::ProcessConnect(proto_connect_t con)
{
	if(m_pathState==path_idle)
	{
		Time gap=MilliSeconds(m_rtt);
		m_pingTimer=Simulator::Schedule(gap,&WebrtcReceiver::SendPing,this);
	}
	m_pathState=path_connected;
	WebrtcHeader header;
	header.SetMid(ProtoType::PT_CONNECT_ACK);
	header.SetUid(m_uid);
	proto_connect_ack_t ack;
	ack.cid=con.cid;
	ack.result=0;
	header.SetMessagePayload((void*)&ack,sizeof(ack));
	Ptr<Packet> p=Create<Packet>(0);
	p->AddHeader(header);
	Send(p);
}
void WebrtcReceiver::ProcessPing(proto_ping_t ping)
{
	WebrtcHeader header;
	header.SetMid(ProtoType::PT_PONG);
	header.SetUid(m_uid);
	header.SetMessagePayload((void*)&ping,sizeof(ping));
	Ptr<Packet> p=Create<Packet>(0);
	p->AddHeader(header);
	Send(p);

}
void WebrtcReceiver::ProcessPong(proto_pong_t pong)
{
	int64_t now_ts=Simulator::Now().GetMilliSeconds();
	uint64_t keep_rtt=5;
	uint64_t averageRtt=0;
	if (now_ts > pong.ts + 5)
		keep_rtt = (uint32_t)(now_ts - pong.ts);
	m_rtt=keep_rtt;
	if(m_numRtt==0)
	{	
		if(m_controller){
		m_controller->OnRttUpdate(keep_rtt,keep_rtt);
		}
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
	if(m_controller)
	{
		m_controller->OnRttUpdate(averageRtt,m_maxRtt);
	}
	//RTC_LOG(LS_INFO)<<"prcess pong";
	//NS_LOG_FUNCTION(m_rtt<<" "<<averageRtt);
}
void WebrtcReceiver::SendPing(void)
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
		Simulator::Schedule(next,&WebrtcReceiver::SendPing,this);
	}
}
void WebrtcReceiver::StartApplication()
{
	m_running=true;
}
void WebrtcReceiver::StopApplication()
{
	m_running=false;
	m_pm->Stop();
	m_pm->DeRegisterModule(m_controller.get());
	if(m_rbe)
	{
	m_pm->DeRegisterModule(m_rbe);
	}
	delete m_pm;
}
}



