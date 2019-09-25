#include "mpreceiver.h"
#include "recvpath.h"
#include <vector>
#include"ns3/log.h"
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("MpReceiver");
MpReceiver::MpReceiver()
{
}
MpReceiver::~MpReceiver()
{

}
void MpReceiver::Init(uint32_t uid,uint32_t cid)
{
m_uid=uid;
m_cid=cid;
}
void MpReceiver::Bind(uint16_t port)
{
	std::vector<Ipv4Address> srcAddresses;
	GetNodeAddress(GetNode(),srcAddresses);
	for(uint32_t i=0;i<srcAddresses.size();i++)
	{
		Ptr<Socket> socket=Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
		auto local = InetSocketAddress{srcAddresses[i], port};
		auto ret=socket->Bind(local);
		NS_ASSERT (ret == 0);
		socket->SetRecvCallback (MakeCallback (&MpReceiver::RecvPacket,this));
		m_sockets.insert(std::make_pair(srcAddresses[i],socket));
	}
	m_running = false;
}
void MpReceiver::RecvPacket(Ptr<Socket>socket)
{
    Address remoteAddr;
    auto packet = socket->RecvFrom (remoteAddr);
    Address localAddr;
    socket->GetSockName(localAddr);
	PathAddressInfo info;
	info.srcIp = InetSocketAddress::ConvertFrom (localAddr).GetIpv4 ();
	info.srcPort = InetSocketAddress::ConvertFrom (localAddr).GetPort ();
	info.destIp=InetSocketAddress::ConvertFrom (remoteAddr).GetIpv4 ();
	info.destPort=InetSocketAddress::ConvertFrom (remoteAddr).GetPort ();
	NS_LOG_INFO("RECVFROM "<<info.destIp<<info.destPort);
	auto it=m_paths.find(info);
	ReceivePath *path=nullptr;
	if(it==m_paths.end())
	{
		path=new ReceivePath(this,m_pathNum,info);
		m_paths.insert(std::make_pair(info,path));
		m_pathNum++;
	}else{
		path=it->second;
	}
	RazorHeader header;
	packet->RemoveHeader(header);
    sim_header_t sHeader;
    header.GetHeader(sHeader);
    path->ProcessMessage(sHeader,header);
}
void MpReceiver::StartApplication()
{
	m_running=true;
}
void MpReceiver::StopApplication()
{
	m_running=false;
	for(auto it=m_paths.begin();it!=m_paths.end();)
	{
		delete it->second;
		m_paths.erase(it++);
	}
}
void MpReceiver::OnPacketPlugin(PathAddressInfo info,Ptr<Packet> packet)
{
	Ptr<Socket> socket=m_sockets[info.srcIp];
	socket->SendTo(packet,0,InetSocketAddress{info.destIp,info.destPort});
}
}

