#include "mpsender.h"
#include "ns3/log.h"
#include <math.h>
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("MpSender");

MpSender::MpSender()
{

}
MpSender::~MpSender()
{
	if(!m_paths.empty())
	{
		NS_LOG_ERROR("should clear the path before");
	}
}
void MpSender::Init(uint32_t cid,uint32_t uid)
{
	m_cid=cid;
	m_uid=uid;
	std::vector<Ipv4Address> srcAddresses;
	GetNodeAddress(GetNode(),srcAddresses);
	for(uint32_t i=0;i<srcAddresses.size();i++)
	{
		Ptr<Socket> socket=Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
		auto local = InetSocketAddress{srcAddresses[i], 0};
		auto ret=socket->Bind(local);
		NS_ASSERT (ret == 0);
		socket->SetRecvCallback (MakeCallback (&MpSender::RecvPacket,this));
		m_sockets.insert(std::make_pair(srcAddresses[i],socket));
	}
}
void MpSender::AddPath(Ipv4Address destIp,uint16_t destPort)
{
	Address addr;
	for(auto it=m_sockets.begin();it!=m_sockets.end();it++)
	{
		Ipv4Address srcIp=it->first;
		Ptr<Socket> socket=it->second;
		if(IsExistPath(GetNode(),srcIp,destIp))
		{
			PathAddressInfo addrInfo;
			NS_LOG_INFO("Exist path "<<srcIp<<"to "<<destIp);
			socket->GetSockName(addr);
			InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (addr);
			addrInfo.srcIp=srcIp;
			addrInfo.srcPort=iaddr.GetPort();
			addrInfo.destIp=destIp;
			addrInfo.destPort=destPort;
			Path *path=new Path(addrInfo,m_pathNum,this);
			path->InitRate(m_minRate,m_maxRate);
			m_paths.insert(std::make_pair(addrInfo,path));
			m_pathNum++;
		}
	}
}
void MpSender::PathConnect()
{
	auto it=m_paths.begin();
	for(;it!=m_paths.end();it++)
	{
		Path *path=it->second;
		path->Connect();
	}
}
//video packet generator
void MpSender::FakePacket()
{
	if(m_gennerateTimer.IsExpired())
	{
		uint32_t byte=ceil(0.001*m_fs*m_encodeRate/8);
		Schedule(0,byte);
		Time next=MilliSeconds((uint64_t)m_fs);
		Simulator::Schedule(next,&MpSender::FakePacket,this);
	}
}
void MpSender::Schedule(uint8_t ftype,uint32_t byte)
{
	NS_ASSERT((byte/m_segmentSize)<m_maxSplitPackets);
	uint32_t i=0;
	uint16_t splits[m_maxSplitPackets];
	uint16_t npacket=0;
	npacket=FrameSplit(splits,byte);
	uint64_t now=Simulator::Now().GetMilliSeconds();
	NS_LOG_FUNCTION(now<<"byte "<<byte<<"npackets "<<npacket);
	uint32_t timeStamp;
	uint8_t header_len=sizeof(sim_header_t)+sizeof(sim_segment_t);
	if(m_firstTs==-1)
	{
		timeStamp=0;
		m_firstTs=now;
	}else
	{
		timeStamp=(uint32_t)(now-m_firstTs);
	}
	for(i=0;i<npacket;i++)
	{
		sim_segment_t seg;
		seg.packet_id=m_packetId;
		seg.fid=m_frameId;
		seg.timestamp=timeStamp;
		seg.index=i;
		seg.total=npacket;
		seg.ftype=ftype;
		seg.remb=0;
		seg.data_size=splits[i];
		segment_ref_t ref;
		ref.ref=0;
		ref.seg=seg;
		m_txBuffer.insert(std::make_pair(m_packetId,ref));
		m_packetId++;
	}
	if(m_slowStart)
	{
		//the simplest scheduler,send packet to all path;
		for(i=0;i<npacket;i++)
		{
			m_currentPacketId++;
			auto it=m_txBuffer.find(m_currentPacketId);
			NS_ASSERT(it!=m_txBuffer.end());
			segment_ref_t val=it->second;
			for(auto pathit=m_paths.begin();pathit!=m_paths.end();pathit++)
			{
				Path *path=pathit->second;
				if(path->GetPathState()==path_connected)
				{
					val.ref++;
					path->AddPacePacket(val.seg.packet_id,0,val.seg.data_size+header_len);
				}
			}
			if(m_currentPacketId==0xFFFFFFFF)
			{
				m_currentPacketId=-1;
			}
		}

	}else{
		//add your own packets schedule algorithm
	}
}
uint16_t MpSender::FrameSplit(uint16_t splits[],uint32_t size)
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
void MpSender::OnPacePacket(PathAddressInfo info,uint32_t packet_id, uint16_t seq,
		int retrans,uint32_t size)
{
	auto it=m_txBuffer.find(packet_id);
	NS_LOG_FUNCTION("packet"<<packet_id);
	NS_ASSERT(it!=m_txBuffer.end());
	segment_ref_t ref=it->second;
	int64_t now=Simulator::Now().GetMilliSeconds();
	ref.seg.send_ts=(uint16_t)(now-m_firstTs-ref.seg.timestamp);
	ref.seg.transport_seq=seq;
	ref.ref--;
	if(!ref.ref)
	{
		m_txBuffer.erase(it);// no retrans for now;
	}
	sim_header_t sHeader;
	INIT_SIM_HEADER(sHeader,RazorProtoType::SIM_SEG,m_uid);
	RazorHeader header;
	header.SetHeader(sHeader);
	header.SetMessagePayload((void*)&(ref.seg),sizeof(ref.seg));
	Ptr<Packet> packet=Create<Packet>((uint32_t)ref.seg.data_size);//fake video size;
	packet->AddHeader(header);
	Ptr<Socket> socket=m_sockets[info.srcIp];
	socket->SendTo(packet,0,InetSocketAddress{info.destIp,info.destPort});
}
void MpSender::OnPacket(PathAddressInfo info,Ptr<Packet> packet)
{
	Ptr<Socket> socket=m_sockets[info.srcIp];
	socket->SendTo(packet,0,InetSocketAddress{info.destIp,info.destPort});
}
void MpSender::OnBwProbe(PathAddressInfo info,Path *path,uint32_t size)
{
	Ptr<Socket> socket=m_sockets[info.srcIp];
	uint16_t splits[m_maxSplitPackets];
	uint32_t i=0;
	uint32_t timeStamp=0;
	uint64_t now=Simulator::Now().GetMilliSeconds();
	uint32_t npacket=FrameSplit(splits,size);
	NS_LOG_FUNCTION("npacket "<<npacket<<"len "<<size);
	uint8_t header_len=sizeof(sim_header_t)+sizeof(sim_segment_t);
	if(m_firstTs==-1)
	{
		timeStamp=0;
		m_firstTs=now;
	}else
	{
		timeStamp=(uint32_t)(now-m_firstTs);
	}
	for(i=0;i<npacket;i++)
	{
		if(splits[i]>header_len)
		{
			sim_segment_t seg;
			seg.packet_id=0;
			seg.fid=0;
			seg.timestamp=timeStamp;
			seg.index=i;
			seg.total=npacket;
			seg.ftype=0x80;
			seg.remb=0;
			seg.data_size=splits[i]-header_len;
			seg.transport_seq=path->GetTransSeq();
			sim_header_t sHeader;
			INIT_SIM_HEADER(sHeader,RazorProtoType::SIM_SEG,m_uid);
			RazorHeader header;
			header.SetHeader(sHeader);
			header.SetMessagePayload((void*)&seg,sizeof(seg));
			Ptr<Packet> packet=Create<Packet>((uint32_t)seg.data_size);
			packet->AddHeader(header);
			socket->SendTo(packet,0,InetSocketAddress{info.destIp,info.destPort});
		}
	}


}
void MpSender::OnChangeBitRate()
{

}
void MpSender::StartApplication()
{
	PathConnect();
}
void MpSender::StopApplication()
{
	for(auto it=m_paths.begin();it!=m_paths.end();)
	{
		delete it->second;
		m_paths.erase(it++);
	}
}
Path* MpSender::GetPath(Address &localAddr,Address &remoteAddr)
{
	PathAddressInfo info;
	info.srcIp = InetSocketAddress::ConvertFrom (localAddr).GetIpv4 ();
	info.srcPort = InetSocketAddress::ConvertFrom (localAddr).GetPort ();
	info.destIp=InetSocketAddress::ConvertFrom (remoteAddr).GetIpv4 ();
	info.destPort=InetSocketAddress::ConvertFrom (remoteAddr).GetPort ();
	NS_LOG_INFO("sender"<<info.destIp<<info.destPort);
	auto it=m_paths.find(info);
	NS_ASSERT(it!=m_paths.end());
	return it->second;
}
void MpSender::RecvPacket(Ptr<Socket>socket)
{
    Address remoteAddr;
    Address localAddr;
    auto packet = socket->RecvFrom (remoteAddr);
    socket->GetSockName(localAddr);
    Path *path=GetPath(localAddr,remoteAddr);
	RazorHeader header;
	packet->RemoveHeader(header);
    sim_header_t sHeader;
    header.GetHeader(sHeader);
    switch(sHeader.mid)
    {
    case SIM_CONNECT_ACK:
    {
    	NS_LOG_INFO("receive connecte ack");
	uint8_t oldState=path->GetPathState();
	path->ProcessMessage(sHeader,header);
    	if(m_active==false)
    	{
    		m_active=true;
    		m_gennerateTimer=Simulator::ScheduleNow(&MpSender::FakePacket,this);
    	}
	uint8_t newState=path->GetPathState();
	if(oldState==path_connecting&&newState==path_connected)
	{
		path->StartPace();
	}

    }
    break;
    case SIM_DISCONNECT:
    case SIM_DISCONNECT_ACK:
    case SIM_PING:
    case SIM_PONG:
    case SIM_FEEDBACK:
    {
    	path->ProcessMessage(sHeader,header);
    }
    break;
    case SIM_SEG:
    {

    }
    break;
    case SIM_SEG_ACK:
    {

    }
    break;
    default :
    	NS_LOG_WARN("message is unrecognizable");
    	break;
    }
}
}
