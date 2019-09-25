#include "mpcommon.h"
#include"ns3/socket.h"
#include"ns3/log.h"
#include<cassert>
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("MpCommon");
void GetNodeAddress(Ptr<Node>node,std::vector<Ipv4Address>&addresses)
{
	uint32_t interface=0;
	uint32_t totalInterface=0;
	Ptr<Ipv4> ipv4 =node->GetObject<Ipv4> ();
	totalInterface=ipv4->GetNInterfaces();
	NS_LOG_INFO("totalinterfaces "<<totalInterface);
	for(interface=0;interface<totalInterface;interface++)
	{
		Ipv4Address srcIp = ipv4->GetAddress (interface, 0).GetLocal ();
		if(srcIp==Ipv4Address::GetLoopback()) continue;
		addresses.push_back(srcIp);
		NS_LOG_INFO("srcIp "<<srcIp);
	}
}
bool IsExistPath (Ptr<Node> node,Ipv4Address src, Ipv4Address dst)
{
  bool found = false;
  uint32_t interface=0;
  Ptr<Node> m_node=node;
  // Look up the source address
  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
  assert (ipv4->GetRoutingProtocol () != 0);
  Ipv4Header l3Header;
  Socket::SocketErrno errno_;
  Ptr<Ipv4Route> route;
  interface=ipv4->GetInterfaceForAddress(src);      //Pablo UC
  Ptr<NetDevice> oif=ipv4->GetNetDevice(interface); //Pablo UC
  l3Header.SetSource (src);
  l3Header.SetDestination (dst);
  route = ipv4->GetRoutingProtocol ()->RouteOutput (Ptr<Packet> (), l3Header, oif, errno_);
  if ((route != 0) && (src == route->GetSource ()))
  {
    NS_LOG_INFO ("IsThereRoute -> Route from srcAddr "<< src << " to dstAddr " << dst << " oit "<<oif<<", exist !");
    found = true;
  }else
    NS_LOG_INFO ("IsThereRoute -> No Route from srcAddr "<< src << " to dstAddr " << dst << " oit "<<oif<<", exist !");

  return found;
}
}
