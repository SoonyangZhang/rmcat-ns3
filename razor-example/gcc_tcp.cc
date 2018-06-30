#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/zsy-pacer-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/packet-sink-helper.h"
#include <stdarg.h>
#define gLOG_EMERG -1
#define gLOG_DEBUG 0
#define gLOG_INFO 1
#define gLOG_WARN  2
#define gLOG_ERROR 3
#define gLOG_LAST_PRIORITY 4
NS_LOG_COMPONENT_DEFINE("PACER_EXAMPLE");
using namespace ns3;
using namespace std;
const uint32_t TOPO_DEFAULT_BW     = 2000000;    // in bps: 1Mbps
const uint32_t TOPO_DEFAULT_PDELAY =      100;    // in ms:   50ms
const uint32_t TOPO_DEFAULT_QDELAY =     300;    // in ms:  300ms
const uint32_t DEFAULT_PACKET_SIZE = 1000;
void WriteLog(int priority,const char *file,int line,const char* fmt,va_list vl){
     if (priority < gLOG_EMERG || priority >= gLOG_LAST_PRIORITY) {
         return;
     }
    char str[512 + 1] = "";
 
    int size = vsnprintf(str, 512, fmt, vl);
    if (size <= 0) {
        return;
    }
printf("%s,%d",file,line);
    printf("%s\n",str);
    return;
}
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

    return nodes;
}
static void InstallApps (
                         Ptr<Node> sender,
                         Ptr<Node> receiver,
                         uint16_t port,
                         float startTime,
                         float stopTime)
{
	Ptr<SimEndpoint> sendApp=CreateObject<SimEndpoint>();
	Ptr<SimEndpoint> recvApp=CreateObject<SimEndpoint>();
   	sender->AddApplication (sendApp);
    	receiver->AddApplication (recvApp);
    Ptr<Ipv4> ipv4 = receiver->GetObject<Ipv4> ();
    Ipv4Address receiverIp = ipv4->GetAddress (1, 0).GetLocal ();
    	sendApp->Setcid(1243);
	recvApp->Setcid(4321);
	sendApp->InitialSetup(receiverIp,port);
	recvApp->Bind(port);
	sendApp->RateInit(500000,40);//1.5Mbps
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
static double simDuration=200;
uint16_t servPort=5432;
uint16_t tcpServPort=5000;
float appStart=0.0;
float appStop=simDuration-2;
int main(int argc, char *argv[])
{
	LogComponentEnable("PACER_EXAMPLE", LOG_LEVEL_ALL);	
	//LogComponentEnable("RazorHeader", LOG_LEVEL_ALL);
	LogComponentEnable("SimEndpoint",LOG_LEVEL_ALL);
	//LogComponentEnable("SimSender",LOG_LEVEL_ALL);
	//LogComponentEnable("SimReceiver",LOG_LEVEL_ALL);
    const uint64_t linkBw   = TOPO_DEFAULT_BW;
    const uint32_t msDelay  = TOPO_DEFAULT_PDELAY;
    const uint32_t msQDelay = TOPO_DEFAULT_QDELAY;
    NodeContainer nodes = BuildExampleTopo (linkBw, msDelay, msQDelay);
	InstallApps(nodes.Get (0), nodes.Get (1),servPort,appStart,appStop);
	//razor_setup_log((razor_log_func)WriteLog);
	InstallTcp(nodes.Get (0), nodes.Get (1),tcpServPort,20,100);
    Simulator::Stop (Seconds(simDuration + 10.0));
    Simulator::Run ();
    Simulator::Destroy();
	return 0;	
}
