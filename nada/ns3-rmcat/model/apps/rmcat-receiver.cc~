/******************************************************************************
 * Copyright 2016-2017 Cisco Systems, Inc.                                    *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 *                                                                            *
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
 * Receiver application implementation for rmcat ns3 module.
 *
 * @version 0.1.0
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#include "rmcat-receiver.h"
#include "rmcat-header.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("RmcatReceiver");

namespace ns3 {

RmcatReceiver::~RmcatReceiver () {}

void RmcatReceiver::Setup (uint16_t port)
{
    m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
    auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
    auto ret = m_socket->Bind (local);
    NS_ASSERT (ret == 0);
    m_socket->SetRecvCallback (MakeCallback (&RmcatReceiver::RecvPacket,this));

    m_running = false;
    m_waiting = true;
}

void RmcatReceiver::StartApplication ()
{
    m_running = true;
}

void RmcatReceiver::StopApplication ()
{
    m_running = false;
}

void RmcatReceiver::RecvPacket (Ptr<Socket> socket)
{
    if (!m_running) {
        return;
    }

    Address remoteAddr;
    auto packet = m_socket->RecvFrom (remoteAddr);
    NS_ASSERT (packet);
    MediaHeader header;
    NS_LOG_INFO ("RmcatReceiver::RecvPacket, " << packet->ToString ());
    packet->RemoveHeader (header);
    auto srcIp = InetSocketAddress::ConvertFrom (remoteAddr).GetIpv4 ();
    auto srcPort = InetSocketAddress::ConvertFrom (remoteAddr).GetPort ();
    if (m_waiting) {
        m_waiting = false;
        m_srcId = header.flow_id;
        m_srcIp = srcIp;
        m_srcPort = srcPort;
    } else {
        // Only one flow supported
        NS_ASSERT (m_srcId == header.flow_id);
        NS_ASSERT (m_srcIp == srcIp);
        NS_ASSERT (m_srcPort == srcPort);
    }

    //TODO (deferred): We need to aggregate feedback information
    //                 (for the moment, one feedback packet per media packet)

    auto recvTimestamp = Simulator::Now ().GetMilliSeconds ();
	uint64_t tmp=header.send_tstmp;
	uint64_t gap=recvTimestamp-tmp;
	if(!m_delayCb.IsNull()){m_delayCb(gap);}
    SendFeedback (header.sequence, recvTimestamp);
}

void RmcatReceiver::SendFeedback (uint32_t sequence,
                                  uint64_t recvTimestamp)
{
    FeedbackHeader header;
    header.flow_id = m_srcId;
    header.sequence = sequence;
    header.receive_tstmp = recvTimestamp;

    auto packet = Create<Packet> ();
    packet->AddHeader (header);
    NS_LOG_INFO ("RmcatReceiver::SendFeedback, " << packet->ToString ());

    m_socket->SendTo (packet, 0, InetSocketAddress{m_srcIp, m_srcPort});
}

}

