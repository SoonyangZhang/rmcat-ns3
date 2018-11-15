/******************************************************************************
 * Copyright 2016-2017 cisco Systems, Inc.                                    *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 ******************************************************************************/

/**
 * @file
 * Simple example demonstrating the usage of the rmcat ns3 module, using:
 *  - NADA as controller for rmcat flows
 *  - Statistics-based traffic source as codec
 *  - [Optionally] TCP flows
 *  - [Optionally] UDP flows
 *
 * @version 0.1.0
 * @author聽Jiantao Fu
 * @author聽Sergio Mena
 * @author Xiaoqing Zhu
 */

#include "ns3/nada-controller.h"
#include "ns3/rmcat-sender.h"
#include "ns3/rmcat-receiver.h"
#include "ns3/rmcat-constants.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/data-rate.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/log.h"
#include "ns3/mytrace.h"
#include <string>
const uint32_t RMCAT_DEFAULT_RMIN  =  150000;  // in bps: 150Kbps
const uint32_t RMCAT_DEFAULT_RMAX  = 3000000;  // in bps: 1.5Mbps
const uint32_t RMCAT_DEFAULT_RINIT =  150000;  // in bps: 150Kbps

const uint32_t TOPO_DEFAULT_BW     = 2000000;    // in bps: 1Mbps
const uint32_t TOPO_DEFAULT_PDELAY =      100;    // in ms:   50ms
const uint32_t TOPO_DEFAULT_QDELAY =     300;    // in ms:  300ms

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("NADA");
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

	std::string errorModelType = "ns3::RateErrorModel";
  	ObjectFactory factory;
  	factory.SetTypeId (errorModelType);
  	Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
	devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));	
    return nodes;
}

static void InstallTCP (Ptr<Node> sender,
                        Ptr<Node> receiver,
                        uint16_t port,
                        float startTime,
                        float stopTime)
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

static Time GetIntervalFromBitrate (uint64_t bitrate, uint32_t packetSize)
{
    if (bitrate == 0u) {
        return Time::Max ();
    }
    const auto secs = static_cast<double> (packetSize + IPV4_UDP_OVERHEAD) /
                            (static_cast<double> (bitrate) / 8. );
    return Seconds (secs);
}

static void InstallUDP (Ptr<Node> sender,
                        Ptr<Node> receiver,
                        uint16_t serverPort,
                        uint64_t bitrate,
                        uint32_t packetSize,
                        uint32_t startTime,
                        uint32_t stopTime)
{
    // configure UDP source/sender/client
    auto serverAddr = receiver->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal ();
    const auto interPacketInterval = GetIntervalFromBitrate (bitrate, packetSize);
    uint32_t maxPacketCount = 0XFFFFFFFF;
    UdpClientHelper client{serverAddr, serverPort};
    client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    client.SetAttribute ("Interval", TimeValue (interPacketInterval));
    client.SetAttribute ("PacketSize", UintegerValue (packetSize));

    auto clientApps = client.Install (sender);
    clientApps.Start (Seconds (startTime));
    clientApps.Stop (Seconds (stopTime));

    // configure TCP sink/receiver/server
    UdpServerHelper server{serverPort};
    auto serverApps = server.Install (receiver);
    serverApps.Start (Seconds (startTime));
    serverApps.Stop (Seconds (stopTime));
}

static void InstallNADA (bool nada,
                         Ptr<Node> sender,
                         Ptr<Node> receiver,
                         uint16_t port,
                         float initBw,
                         float minBw,
                         float maxBw,
                         float startTime,
                         float stopTime,MyTracer *tracer)
{
    Ptr<RmcatSender> sendApp = CreateObject<RmcatSender> ();
    Ptr<RmcatReceiver> recvApp = CreateObject<RmcatReceiver> ();
    sender->AddApplication (sendApp);
    receiver->AddApplication (recvApp);

    if (nada) {
        sendApp->SetController (std::make_shared<rmcat::NadaController> ());
    }
    Ptr<Ipv4> ipv4 = receiver->GetObject<Ipv4> ();
    Ipv4Address receiverIp = ipv4->GetAddress (1, 0).GetLocal ();
    sendApp->Setup (receiverIp, port); // initBw, minBw, maxBw);

    const auto fps = 25.;
    //auto innerCodec = new syncodecs::StatisticsCodec{fps};
    auto codec = new syncodecs::PerfectCodec{DEFAULT_PACKET_SIZE};//new syncodecs::ShapedPacketizer{innerCodec, DEFAULT_PACKET_SIZE};
    codec->SetRateCallback(MakeCallback(&MyTracer::RateTrace,tracer));
    sendApp->SetCodec (std::shared_ptr<syncodecs::Codec>{codec});

    recvApp->Setup (port);
    recvApp->SetDelayCallback(MakeCallback(&MyTracer::DelayTrace,tracer));
    sendApp->SetStartTime (Seconds (startTime));
    sendApp->SetStopTime (Seconds (stopTime));

    recvApp->SetStartTime (Seconds (startTime));
    recvApp->SetStopTime (Seconds (stopTime));
}
static uint16_t tcpServPort=1234;
int main (int argc, char *argv[])
{
    int nRmcat = 1;
    int nTcp = 0;
    int nUdp = 0;
    bool log = false;
    bool nada = true;
    std::string strArg  = "strArg default";

	Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (0.05));
	Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

	Config::SetDefault ("ns3::BurstErrorModel::ErrorRate", DoubleValue (0.05));
	Config::SetDefault ("ns3::BurstErrorModel::BurstSize", StringValue ("ns3::UniformRandomVariable[Min=1|Max=3]"));



   //LogComponentEnable ("RmcatSender", LOG_INFO);
    LogComponentEnable ("SynCodecs", LOG_ALL);
    const uint64_t linkBw   = TOPO_DEFAULT_BW;
    const uint32_t msDelay  = TOPO_DEFAULT_PDELAY;
    const uint32_t msQDelay = TOPO_DEFAULT_QDELAY;

    const float minBw =  RMCAT_DEFAULT_RMIN;
    const float maxBw =  RMCAT_DEFAULT_RMAX;
    const float initBw = RMCAT_DEFAULT_RINIT;

    const float endTime = 200.;

    NodeContainer nodes = BuildExampleTopo (linkBw, msDelay, msQDelay);

    int port = 8000;
	std::string nadafile1=std::string("nada1");
	MyTracer nadaTracer1;
	nadaTracer1.OpenTraceFile(nadafile1);
    InstallNADA(nada, nodes.Get (0), nodes.Get (1), port,
                     initBw, minBw, maxBw, 0.0, endTime,&nadaTracer1);

	/*std::string nadafile2=std::string("nada2");
	MyTracer nadaTracer2;
	nadaTracer2.OpenTraceFile(nadafile2);
    InstallNADA(nada, nodes.Get (0), nodes.Get (1), port+1,
                     initBw, minBw, maxBw, 40.0, endTime,&nadaTracer2);

	std::string nadafile3=std::string("nada3");
	MyTracer nadaTracer3;
	nadaTracer3.OpenTraceFile(nadafile3);
    InstallNADA(nada, nodes.Get (0), nodes.Get (1), port+2,
                     initBw, minBw, maxBw, 80.0, endTime,&nadaTracer3);
    //InstallTCP(nodes.Get (0), nodes.Get (1),tcpServPort,20,100);
   // Ptr<NetDevice> netDevice=nodes.Get(0)->GetDevice(0);
	//ChangeBw change(netDevice);
	//change.Start();*/
    Simulator::Stop (Seconds (endTime));
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
