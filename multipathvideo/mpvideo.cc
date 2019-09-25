#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/node-list.h"
#include "ns3/socket.h"
#include "ns3/multipathvideo-module.h"
using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("MpVideo");
static uint16_t servPort=5000;
static uint16_t queueSize=10;
static uint8_t links=3;
static int64_t startTime=0;
static int64_t stopTime=10;
static int64_t endTime=20;//2s
int main(int argc, char *argv[])
{
	LogComponentEnable("MpVideo", LOG_LEVEL_ALL);
	LogComponentEnable("RecvPath", LOG_LEVEL_ALL);
	LogComponentEnable("Path", LOG_LEVEL_ALL);
	LogComponentEnable("MpSender", LOG_LEVEL_ALL);
	LogComponentEnable("MpReceiver", LOG_LEVEL_ALL);
	NodeContainer n0n1;	
	n0n1.Create (2);
	
	InternetStackHelper stack;
	stack.Install (n0n1);	
	
	vector<Ipv4InterfaceContainer> ipv4Ints;
    for(int i=0; i <links; i++)
	{
	PointToPointHelper p2plink;
    p2plink.SetQueue ("ns3::DropTailQueue", "MaxPackets",UintegerValue(queueSize));
	p2plink.SetChannelAttribute("Delay", StringValue("100ms"));
	p2plink.SetDeviceAttribute ("DataRate", StringValue("5Mbps"));
	
	NetDeviceContainer netDevices;
    netDevices = p2plink.Install(n0n1);
    // Attribution of the IP addresses
    std::stringstream netAddr;
    netAddr << "10.1." << (i+1) << ".0";
    string str = netAddr.str();

    Ipv4AddressHelper ipv4addr;
    ipv4addr.SetBase(str.c_str(), "255.255.255.0");
    Ipv4InterfaceContainer interface = ipv4addr.Assign(netDevices);
    ipv4Ints.insert(ipv4Ints.end(), interface);
	}	
	std::vector<Ipv4Address> destAddresses;
	GetNodeAddress(n0n1.Get(1),destAddresses);
	int32_t i=0;
    Ptr<MpSender> sendApp = CreateObject<MpSender> ();
    Ptr<MpReceiver> recvApp = CreateObject<MpReceiver> ();
    n0n1.Get(0)->AddApplication (sendApp);
    n0n1.Get(1)->AddApplication (recvApp);
	recvApp->Bind(servPort);	
	sendApp->Init(0,1234);//cid uid;
	for(i=0;i<destAddresses.size();i++)
	{
		sendApp->AddPath(destAddresses[i],servPort);
		NS_LOG_INFO(destAddresses[i]);
	}
    sendApp->SetStartTime (Seconds (startTime));
    sendApp->SetStopTime (Seconds (stopTime));

    recvApp->SetStartTime (Seconds (startTime));
    recvApp->SetStopTime (Seconds (stopTime));	
	Simulator::Stop (Seconds (endTime));
    Simulator::Run ();
    Simulator::Destroy ();
	return 0;
}
