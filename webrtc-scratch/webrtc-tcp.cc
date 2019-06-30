#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/webrtc-ns3-module.h"
#include "ns3/nstime.h"
#include "ns3/mytrace.h"
#include <stdarg.h>
#include <string>
using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("Webrtc-Tcp");
const uint32_t TOPO_DEFAULT_BW     = 3000000;    // in bps: 1Mbps
const uint32_t TOPO_DEFAULT_PDELAY =      100;    // in ms:   50ms
const uint32_t TOPO_DEFAULT_QDELAY =     300;    // in ms:  300ms
const uint32_t DEFAULT_PACKET_SIZE = 1000;
const static uint32_t rateArray[]=
{
2000000,
1500000,
1000000,
 500000,
1000000,
1500000,
};
class ChangeBw
{
public:
	ChangeBw(Ptr<NetDevice> netdevice)
	{
	m_total=sizeof(rateArray)/sizeof(rateArray[0]);
	m_netdevice=netdevice;
	}
	//ChangeBw(){}
	~ChangeBw(){}
	void Start()
	{
		Time next=Seconds(m_gap);
		m_timer=Simulator::Schedule(next,&ChangeBw::ChangeRate,this);		
	}
	void ChangeRate()
	{
		if(m_timer.IsExpired())
		{
		NS_LOG_INFO(Simulator::Now().GetSeconds()<<" "<<rateArray[m_index]/1000);
		//Config::Set ("/ChannelList/0/$ns3::PointToPointChannel/DataRate",DataRateValue (rateArray[m_index]));
		PointToPointNetDevice *device=static_cast<PointToPointNetDevice*>(PeekPointer(m_netdevice));
		device->SetDataRate(DataRate(rateArray[m_index]));
		m_index=(m_index+1)%m_total;
		Time next=Seconds(m_gap);
		m_timer=Simulator::Schedule(next,&ChangeBw::ChangeRate,this);
		}

	}
private:
	uint32_t m_index{1};
	uint32_t m_gap{20};
	uint32_t m_total{6};
	Ptr<NetDevice>m_netdevice;
	EventId m_timer;
};
static NodeContainer BuildExampleTopo (uint64_t bps,
                                       uint32_t msDelay,
                                       uint32_t msQdelay)
{
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (bps)));
    pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
    auto bufSize = std::max<uint32_t> (DEFAULT_PACKET_SIZE, bps * msQdelay / 8000);
    pointToPoint.SetQueue ("ns3::DropTailQueue",
                           "Mode", StringValue ("QUEUE_MODE_BYTES"),
                           "MaxBytes", UintegerValue (bufSize));
    NetDeviceContainer devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);
    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    address.Assign (devices);

    // Uncomment to capture simulated traffic
    // pointToPoint.EnablePcapAll ("rmcat-example");

    // disable tc for now, some bug in ns3 causes extra delay
    TrafficControlHelper tch;
    tch.Uninstall (devices);

	/* //add random loss
	std::string errorModelType = "ns3::RateErrorModel";
  	ObjectFactory factory;
  	factory.SetTypeId (errorModelType);
  	Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
	devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));*/
    return nodes;
}

static void InstallWebrtc(
                         Ptr<Node> sender,
                         Ptr<Node> receiver,
                         uint16_t port,
                         float startTime,
                         float stopTime,uint32_t maxrate,WebrtcTrace*tracer)
{
    Ptr<WebrtcSender> sendApp = CreateObject<WebrtcSender> ();
    sendApp->SetMaxRate(maxrate);
    Ptr<WebrtcReceiver> recvApp = CreateObject<WebrtcReceiver>();
   	sender->AddApplication (sendApp);
    receiver->AddApplication (recvApp);
    Ptr<Ipv4> ipv4 = receiver->GetObject<Ipv4> ();
    Ipv4Address receiverIp = ipv4->GetAddress (1, 0).GetLocal ();
	sendApp->InitialSetup(receiverIp,port);
	recvApp->Bind(port);
	recvApp->SetWebrtcSender(sendApp);
	if(tracer){
		sendApp->SetRecvFeedbackTraceFun(MakeCallback(&WebrtcTrace::OnFeedback,tracer));
		PacketGenerator *encoder=sendApp->GetPacketGenerator();
		encoder->SetRateCallback(MakeCallback(&WebrtcTrace::RateTrace,tracer));
		recvApp->SetDelayCallback(MakeCallback(&WebrtcTrace::DelayTrace,tracer));	
	}

    sendApp->SetStartTime (Seconds (startTime));
    sendApp->SetStopTime (Seconds (stopTime));
    recvApp->SetStartTime (Seconds (startTime));
    recvApp->SetStopTime (Seconds (stopTime));
}
static void InstallTcp(
                         Ptr<Node> sender,
                         Ptr<Node> receiver,
                         uint16_t port,
                         float startTime,
                         float stopTime
)
{
    // configure TCP source/sender/client
    auto serverAddr = receiver->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal ();
    BulkSendHelper source{"ns3::TcpSocketFactory",
                           InetSocketAddress{serverAddr, port}};
    // Set the amount of data to send in bytes. Zero is unlimited.
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    source.SetAttribute ("SendSize", UintegerValue (DEFAULT_PACKET_SIZE));

    auto clientApps = source.Install (sender);
    clientApps.Start (Seconds (startTime));
    clientApps.Stop (Seconds (stopTime));

    // configure TCP sink/receiver/server
    PacketSinkHelper sink{"ns3::TcpSocketFactory",
                           InetSocketAddress{Ipv4Address::GetAny (), port}};
    auto serverApps = sink.Install (receiver);
    serverApps.Start (Seconds (startTime));
    serverApps.Stop (Seconds (stopTime));	
	
}
static double simDuration=300;
uint16_t servPort=5432;
uint16_t tcpServPort=5000;
float appStart=0.0;
float appStop=simDuration;

int main(int argc, char *argv[])
{
	Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (0.05));
	Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

	Config::SetDefault ("ns3::BurstErrorModel::ErrorRate", DoubleValue (0.05));
	Config::SetDefault ("ns3::BurstErrorModel::BurstSize", StringValue ("ns3::UniformRandomVariable[Min=1|Max=3]"));
    //LogComponentEnable("Webrtc-Tcp",LOG_LEVEL_ALL);
    SetClockForWebrtc();//that's a must
    //LogComponentEnable("PacketGenerator",LOG_LEVEL_ALL);
	uint64_t linkBw   = TOPO_DEFAULT_BW;
    uint32_t msDelay  = TOPO_DEFAULT_PDELAY;
    uint32_t msQDelay = TOPO_DEFAULT_QDELAY;
    CommandLine cmd;
    std::string instance=std::string("1");
    cmd.AddValue ("it", "instacne", instance);
    cmd.Parse (argc, argv);
    if(instance==std::string("1")){
        linkBw=3000000;
        msDelay=100;
        msQDelay=300;
    }else if(instance==std::string("2")){
        linkBw=3000000;
        msDelay=100;
        msQDelay=400;        
    }else if(instance==std::string("3")){
        linkBw=3000000;
        msDelay=100;
        msQDelay=600;        
    }else if(instance==std::string("4")){
        linkBw=4000000;
        msDelay=100;
        msQDelay=300;        
    }else if(instance==std::string("5")){
        linkBw=4000000;
        msDelay=100;
        msQDelay=400;        
    }else if(instance==std::string("6")){
        linkBw=4000000;
        msDelay=100;
        msQDelay=600;        
    }else if(instance==std::string("7")){
        linkBw=5000000;
        msDelay=100;
        msQDelay=300;        
    }else if(instance==std::string("8")){
        linkBw=5000000;
        msDelay=100;
        msQDelay=400;        
    }else if(instance==std::string("9")){
        linkBw=5000000;
        msDelay=100;
        msQDelay=600;        
    }else if(instance==std::string("10")){
        linkBw=10000000;
        msDelay=100;
        msQDelay=300;        
    }
    NodeContainer nodes = BuildExampleTopo (linkBw, msDelay, msQDelay);
    int test_pair=1;
    std::string log=instance+"_webrtc_"+std::to_string(test_pair);
    test_pair++;
	WebrtcTrace trace1;
	trace1.Log(log,WebrtcTraceEnable::E_WEBRTC_ALL);
    InstallWebrtc(nodes.Get (0), nodes.Get (1),servPort,appStart,appStop,linkBw,&trace1);

    log=instance+"_webrtc_"+std::to_string(test_pair);
    test_pair++;
	WebrtcTrace trace2;
	trace2.Log(log,WebrtcTraceEnable::E_WEBRTC_ALL);
    InstallWebrtc(nodes.Get (0), nodes.Get (1),servPort+1,appStart+40,appStop,linkBw,&trace2);

    log=instance+"_webrtc_"+std::to_string(test_pair);
    test_pair++;
	WebrtcTrace trace3;
	trace3.Log(log,WebrtcTraceEnable::E_WEBRTC_ALL);
    InstallWebrtc(nodes.Get (0), nodes.Get (1),servPort+2,appStart+80,appStop,linkBw,&trace3);

    //InstallTcp(nodes.Get (0), nodes.Get (1),tcpServPort,20,100);
   // Ptr<NetDevice> netDevice=nodes.Get(0)->GetDevice(0);
//	ChangeBw change(netDevice);
//	change.Start();
    Simulator::Stop (Seconds(simDuration + 10.0));
    Simulator::Run ();
    Simulator::Destroy();	
	return 0;
}



