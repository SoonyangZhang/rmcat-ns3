#include "sim_endpoint.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "cf_platform.h"
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("SimEndpoint");
SimEndpoint::SimEndpoint()
{
	m_initiator=false;
	m_uid=0;
	m_cid=0;
	m_state=session_idle;
	m_rtt=100;//in milliseconds
	m_rtt_var=5;
	m_conCount=m_conRetries=3;
	m_conTimeout=MilliSeconds((uint64_t)m_rtt);
	m_firstRtt=true;
	m_sender=nullptr;
	m_receiver=nullptr;
	m_packetSize=1000;
	m_bitRate=8000000;
	m_frameRate=40;//in ms
	m_lastGenerateDataTime=0;
}
SimEndpoint::~SimEndpoint(){}
void SimEndpoint::InitialSetup(Ipv4Address dest_ip,uint16_t dest_port)
{
	m_peerIP=dest_ip;
	m_peerPort=dest_port;
	m_initiator=true;
	m_uid=1;
	if(m_socket==NULL)
	{
		m_socket=Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
        auto res = m_socket->Bind ();
        NS_ASSERT (res == 0);
	}
	m_socket->SetRecvCallback (MakeCallback(&SimEndpoint::RecvPacket,this));
}
void SimEndpoint::Bind(uint16_t port)
{
	m_port=port;
    m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
    auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
    auto ret = m_socket->Bind (local);
    NS_ASSERT (ret == 0);
    m_socket->SetRecvCallback (MakeCallback (&SimEndpoint::RecvPacket,this));

}
void SimEndpoint::Disconnect()
{
	RazorHeader header;
	sim_disconnect_t discon{m_cid};
	sim_header_t sHeader;
	INIT_SIM_HEADER(sHeader,RazorProtoType::SIM_DISCONNECT,m_uid);
	header.SetHeader(sHeader);
	header.SetMessagePayload((void*)&discon,sizeof(sim_disconnect_t));
	Ptr<Packet>  p=Create<Packet>(0);
	p->AddHeader(header);
	NS_LOG_INFO("send disconnect message");
	m_state=session_disconnected;
	Send(p);
}
void SimEndpoint::RecvPacket(Ptr<Socket> socket)
{
    Address remoteAddr;
    auto packet = m_socket->RecvFrom (remoteAddr);
	RazorHeader header;
	packet->RemoveHeader(header);
    sim_header_t sHeader;
    header.GetHeader(sHeader);
    switch (sHeader.mid)
    {
    case SIM_CONNECT:
    {
    	sim_connect_t con;
    	header.GetMessage((void*)&con,sizeof(con));
    	auto srcIP = InetSocketAddress::ConvertFrom (remoteAddr).GetIpv4 ();
    	auto srcPort = InetSocketAddress::ConvertFrom (remoteAddr).GetPort ();
    	if(!m_initiator)
    	{
    		m_peerIP=srcIP;
    		m_peerPort=srcPort;
    	}
    	ProcessConnect(sHeader,con);
    }
    break;
    case SIM_CONNECT_ACK:
    {
    	sim_connect_ack_t ack;
    	header.GetMessage((void*)&ack,sizeof(ack));
    	ProcessConnectAck(sHeader,ack);
    }
    break;
    case SIM_PING:
    {
    	sim_ping_t ping;
    	header.GetMessage((void*)&ping,sizeof(ping));
    	ProcessPing(sHeader,ping);
    }
    break;
    case SIM_PONG:
    {
    	sim_pong_t pong;
    	header.GetMessage((void*)&pong,sizeof(pong));
    	ProcessPong(sHeader,pong);
    }
    break;
    case SIM_SEG:
    {
    	if(m_receiver)
    	{
    		sim_segment_t seg;
    		header.GetMessage((void*)&seg,sizeof(seg));
    		m_receiver->RecvFakeData(seg);
    	}
    }
    break;
    case SIM_SEG_ACK:
    {
    	//TODO
    }
    break;
    case SIM_FEEDBACK:
    {
    	if(m_sender)
    	{
    		sim_feedback_t feedback;
    		header.GetMessage((void*)&feedback,sizeof(feedback));
    		m_sender->ProcessFeedBack(feedback);
    	}
    }
    break;
    case SIM_DISCONNECT:
    {
    	sim_disconnect_t discon;
    	header.GetMessage((void*)&discon,sizeof(discon));
    	ProcessDisconnect(sHeader,discon);
    }
    break;
    case SIM_DISCONNECT_ACK:
    {
    	sim_disconnect_ack_t disconack;
    	header.GetMessage((void*)&disconack,sizeof(disconack));
    	ProcessDisconnectAck(sHeader,disconack);
    }
    break;
    default :
    	NS_LOG_WARN("message is unrecognizable");
    	break;
    }

}
void SimEndpoint::SetConnectionCallback(Callback<void,uint32_t>connectionSucceeded,
		Callback<void,uint32_t>connectionFailed)
{
	NS_LOG_FUNCTION(this<<&connectionSucceeded<<&connectionFailed);
	m_connSucceeded=connectionSucceeded;
	m_connFailed=connectionFailed;
}
void SimEndpoint::SendPing(void)
{
	m_heartbeat=MilliSeconds(4*m_rtt);
	if(m_pingEvent.IsExpired())
	{
	m_pingEvent.Cancel();	
	m_pingEvent=Simulator::Schedule(m_heartbeat,&SimEndpoint::SendPing,this);
	//NS_LOG_INFO("heartbeat expire");
	sim_ping_t ping;
	ping.ts=Simulator::Now().GetMilliSeconds();
	sim_header_t sHeader;
	INIT_SIM_HEADER(sHeader,RazorProtoType::SIM_PING,m_uid);
	RazorHeader header;
	header.SetHeader(sHeader);
	header.SetMessagePayload((void*)&ping,sizeof(ping));
	Ptr<Packet> p=Create<Packet>(0);
	p->AddHeader(header);
	//NS_LOG_INFO(Simulator::Now().GetMilliSeconds()<<" send ping message");
	Send(p);
	}
}
void SimEndpoint::Connect()
{
	do_connect();
	m_state=session_connecting;
}
void SimEndpoint::StartApplication()
{
	if(m_initiator==true)
	{
		Connect();
	}
}
void SimEndpoint::StopApplication()
{
	
	if(!m_pingEvent.IsExpired())
	{
		m_pingEvent.Cancel();
		
	}
if(m_initiator==true)
{
	if(!m_generateDataEvent.IsExpired())
	{
		m_generateDataEvent.Cancel();
	}
	Disconnect();
}
}
void SimEndpoint::Send(Ptr<Packet> packet)
{
	if(m_socket==NULL)
	{
		NS_LOG_ERROR("socket  is null,cant send data");
		return ;
	}
	m_socket->SendTo(packet,0,InetSocketAddress{m_peerIP,m_peerPort});
}
void SimEndpoint::do_connect()
{
	int backoffCount=0x01<<(m_conRetries-m_conCount);
	Time timeout=m_conTimeout*backoffCount;
	if(m_conCount==0)
	{
		NS_LOG_INFO("connection failed");
		if(!m_connFailed.IsNull())
		{
			m_connFailed(m_uid);
		}
		StopApplication();
		return;
	}
	m_reConEvent=Simulator::Schedule(timeout,&SimEndpoint::do_connect,this);
	m_conCount--;
	RazorHeader header;
	sim_header_t sHeader;
	INIT_SIM_HEADER(sHeader,RazorProtoType::SIM_CONNECT,m_uid);
	header.SetHeader(sHeader);
	sim_connect_t con;
	con.cid=m_cid;
	con.token_size=1;
	con.token[0]=123;
	header.SetMessagePayload((void*)&con,sizeof(sim_connect_t));
	Ptr<Packet> p=Create<Packet>(0);
	p->AddHeader(header);
	NS_LOG_INFO(Simulator::Now().GetMilliSeconds()<<"send conncet message"<<"backoffcount"<<" "<<backoffCount);
	//m_socket->SendTo(p,0,InetSocketAddress{m_peerIP,m_peerPort});
	Send(p);
}
void SimEndpoint::ProcessConnect(sim_header_t sheader,sim_connect_t con)
{
	NS_LOG_FUNCTION(this<<" "<<Simulator::Now().GetMilliSeconds());
	m_state=session_connected;
	RazorHeader header;
	sim_header_t sHeader;
	INIT_SIM_HEADER(sHeader,RazorProtoType::SIM_CONNECT_ACK,m_uid);
	header.SetHeader(sHeader);
	sim_connect_ack_t ack;
	ack.cid=con.cid;
	ack.result=0;
	header.SetMessagePayload((void*)&ack,sizeof(ack));
	Ptr<Packet> p=Create<Packet>(0);
	p->AddHeader(header);
	Send(p);
	if(!m_initiator)
	{
	if(m_receiver==nullptr)
	{
		ReceiverInit();
		m_pingEvent=Simulator::Schedule(MilliSeconds(m_rtt),&SimEndpoint::SendPing,this);	
	}
	}
}
void SimEndpoint::ProcessConnectAck(sim_header_t sheader,sim_connect_ack_t ack)
{
	NS_LOG_FUNCTION(this<<" "<<Simulator::Now().GetMilliSeconds());
	if(m_state==session_connecting)
	{
	m_state=session_connected;
	if(!m_connSucceeded.IsNull())
	{
		m_connSucceeded(m_uid);
	}
	m_reConEvent.Cancel();
	m_pingEvent=Simulator::ScheduleNow(&SimEndpoint::SendPing,this);
	}
	else
	{
	NS_LOG_INFO("the endpoint has already connected");
	}
	if(m_sender==nullptr)
	{
		SenderInit();
	}
}
void SimEndpoint::ProcessPing(sim_header_t sheader,sim_ping_t ping)
{
	//NS_LOG_FUNCTION_NOARGS();
	//NS_LOG_FUNCTION(this<<" "<<Simulator::Now().GetMilliSeconds());
	RazorHeader header;
	sim_header_t sHeader;
	INIT_SIM_HEADER(sHeader,RazorProtoType::SIM_PONG,m_uid);
	header.SetHeader(sHeader);
	header.SetMessagePayload((void*)&ping,sizeof(ping));
	Ptr<Packet> p=Create<Packet>(0);
	p->AddHeader(header);
	Send(p);
}
void SimEndpoint::ProcessPong(sim_header_t sheader,sim_pong_t pong)
{
	//NS_LOG_FUNCTION(this<<" "<<Simulator::Now().GetMilliSeconds());
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
void SimEndpoint::ProcessDisconnect(sim_header_t sheader,sim_disconnect_t discon)
{
	//NS_LOG_FUNCTION(this<<" "<<Simulator::Now().GetMilliSeconds());
	m_state=session_disconnected;
	RazorHeader header;
	sim_header_t sHeader;
	INIT_SIM_HEADER(sHeader,RazorProtoType::SIM_DISCONNECT_ACK,m_uid);
	header.SetHeader(sHeader);
	sim_disconnect_ack_t disconack;
	disconack.cid=discon.cid;
	disconack.result=0;
	header.SetMessagePayload((void*)&disconack,sizeof(disconack));
	Ptr<Packet> p=Create<Packet>(0);
	p->AddHeader(header);
	Send(p);
	if(m_receiver)
	{
		delete m_receiver;
		m_receiver=nullptr;
	}
	if(m_sender)
	{
		delete m_sender;
		m_sender=nullptr;
	}
}
void SimEndpoint::ProcessDisconnectAck(sim_header_t sheader,sim_disconnect_ack_t disconack)
{
	NS_LOG_FUNCTION(this<<" "<<Simulator::Now().GetMilliSeconds());
	m_state=session_disconnected;
}
void SimEndpoint::CalculateRtt(uint32_t keep_rtt)
{
	if (keep_rtt < 5)
		keep_rtt = 5;
	m_rtt_var=(3*m_rtt_var+SU_ABS(m_rtt, keep_rtt)) / 4;
	if (m_rtt_var < 10)
		m_rtt_var = 10;
	m_rtt = (7 * m_rtt + keep_rtt) / 8;
	if (m_rtt < 10)
		m_rtt = 10;
	//NS_LOG_INFO("rtt "<<m_rtt<<"rtt_var "<<m_rtt_var);
	if(m_sender)
	{
		m_sender->UpdateRtt(m_rtt,m_rtt_var);
	}
	if(m_receiver)
	{
		m_receiver->UpdateRtt(m_rtt,m_rtt_var);
	}
}
void SimEndpoint::SenderInit()
{
	m_sender=new SimSender();
	sim_header_t sHeader;
	INIT_SIM_HEADER(sHeader,RazorProtoType::MIN_MSG_ID,m_uid);
	m_sender->SetHeader(sHeader);
	INIT_SIM_HEADER(sHeader,RazorProtoType::MIN_MSG_ID,m_uid);
	m_sender->SetBitRates(SIM_MIN_BITRATE,m_bitRate,SIM_MAX_BITRATE);
	m_sender->SetSegmentSize(m_packetSize);
	m_sender->SetSendSegmentCallback(MakeCallback(&SimEndpoint::Send,this));
	m_sender->SetVideoBitChangeCallback(MakeCallback(&SimEndpoint::ChangeRate,this));
	m_sender->PaceStart();
	m_generateDataEvent=Simulator::ScheduleNow(&SimEndpoint::GenerateFakeData,this);
}
void SimEndpoint::ReceiverInit()
{
	if(m_receiver==nullptr)
	{
		m_receiver=new SimReceiver();
		sim_header_t sHeader;
		INIT_SIM_HEADER(sHeader,RazorProtoType::MIN_MSG_ID,m_uid);
		m_receiver->SetHeader(sHeader);
		m_receiver->SetFeedBack(MakeCallback(&SimEndpoint::Send,this));
	}
}
void SimEndpoint::ChangeRate(uint32_t bitrate)
{
	uint32_t rate=bitrate*1000;
	double now=Simulator::Now().GetSeconds();
	NS_LOG_INFO(now<<" "<<m_bitRate/1000<<" "<<rate/1000);
	m_bitRate=rate;
	if(rate<=0)
	{
	NS_LOG_FUNCTION("fuck insane bitrate"<<bitrate<< "and set it to min");	
	m_bitRate=SIM_MIN_BITRATE;
	}
	
}
void SimEndpoint::GenerateFakeData()
{
	if(m_generateDataEvent.IsExpired())
	{
		uint32_t len=0;
		int64_t now=Simulator::Now().GetMilliSeconds();
		if(this->m_lastGenerateDataTime==0)
		{
			len=0.001*m_frameRate*m_bitRate/8;
			//m_lastGenerateDataTime=now;
		}else
		{
			len=0.001*(now-this->m_lastGenerateDataTime)*m_bitRate/8;
			
		}
		if(len<=0){
		//NS_LOG_INFO("generate packets "<<len<<" not send");
		this->m_lastGenerateDataTime=now;
	m_generateDataEvent=Simulator::Schedule(MilliSeconds(m_frameRate),
				&SimEndpoint::GenerateFakeData,this);
		return;
		}
		this->m_lastGenerateDataTime=now;
		uint8_t remain=(len%m_packetSize)>0?1:0;
		//NS_LOG_INFO(Simulator::Now().GetMilliSeconds()<<"generate packets "<<len/m_packetSize+remain);
		if(m_sender!=nullptr)
		{
			m_sender->PutFakeData(0,len);
		}

		m_generateDataEvent=Simulator::Schedule(MilliSeconds(m_frameRate),
				&SimEndpoint::GenerateFakeData,this);
	}
}
void SimEndpoint::SetPacketSize(uint32_t size)
{
	m_packetSize=size;
}
}
