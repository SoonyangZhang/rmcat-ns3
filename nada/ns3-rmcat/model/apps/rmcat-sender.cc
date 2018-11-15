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
 * Sender application implementation for rmcat ns3 module.
 *
 * @version 0.1.0
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#include "rmcat-sender.h"
#include "rmcat-header.h"
#include "ns3/dummy-controller.h"
#include "ns3/nada-controller.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"

#include <sys/stat.h>

NS_LOG_COMPONENT_DEFINE ("RmcatSender");

namespace ns3 {

RmcatSender::RmcatSender ()
: m_destIP{}
, m_destPort{0}
, m_initBw{0}
, m_minBw{0}
, m_maxBw{0}
, m_paused{false}
, m_flowId{0}
, m_sequence{0}
, m_enqueueEvent{}
, m_sendEvent{}
, m_sendOversleepEvent{}
, m_rVin{0.}
, m_rSend{0.}
, m_rateShapingBytes{0}
, m_nextSendTstmp{0}
{}

RmcatSender::~RmcatSender () {}

void RmcatSender::PauseResume (bool pause)
{
    NS_ASSERT (pause != m_paused);
    if (pause) {
        Simulator::Cancel (m_enqueueEvent);
        Simulator::Cancel (m_sendEvent);
        Simulator::Cancel (m_sendOversleepEvent);
        m_rateShapingBuf.clear ();
        m_rateShapingBytes = 0;
    } else {
        m_rVin = m_initBw;
        m_rSend = m_initBw;
        m_enqueueEvent = Simulator::ScheduleNow (&RmcatSender::EnqueuePacket, this);
        m_nextSendTstmp = 0;
    }
    m_paused = pause;
}

void RmcatSender::SetCodec (std::shared_ptr<syncodecs::Codec> codec)
{
    m_codec = codec;
}

// TODO (deferred): allow flexible input of video traffic trace path via config file, etc.
void RmcatSender::SetCodecType (SyncodecType codecType)
{
    syncodecs::Codec* codec = NULL;
    switch (codecType) {
        case SYNCODEC_TYPE_PERFECT:
        {
            codec = new syncodecs::PerfectCodec{DEFAULT_PACKET_SIZE};
            break;
        }
        case SYNCODEC_TYPE_FIXFPS:
        {
            const auto fps = SYNCODEC_DEFAULT_FPS;
            auto innerCodec = new syncodecs::SimpleFpsBasedCodec{fps};
            codec = new syncodecs::ShapedPacketizer{innerCodec, DEFAULT_PACKET_SIZE};
            break;
        }
        case SYNCODEC_TYPE_STATS:
        {
            const auto fps = SYNCODEC_DEFAULT_FPS;
            auto innerStCodec = new syncodecs::StatisticsCodec{fps};
            codec = new syncodecs::ShapedPacketizer{innerStCodec, DEFAULT_PACKET_SIZE};
            break;
        }
        case SYNCODEC_TYPE_TRACE:
        case SYNCODEC_TYPE_HYBRID:
        {
            const std::vector<std::string> candidatePaths = {
                ".",      // If run from top directory (e.g., with gdb), from ns-3.26/
                "../",    // If run from with test_new.py with designated directory, from ns-3.26/2017-xyz/
                "../..",  // If run with test.py, from ns-3.26/testpy-output/201...
            };

            const std::string traceSubDir{"src/ns3-rmcat/model/syncodecs/video_traces/chat_firefox_h264"};
            std::string traceDir{};

            for (auto c : candidatePaths) {
                std::ostringstream currPathOss;
                currPathOss << c << "/" << traceSubDir;
                struct stat buffer;
                if (::stat (currPathOss.str ().c_str (), &buffer) == 0) {
                    //filename exists
                    traceDir = currPathOss.str ();
                    break;
                }
            }

            NS_ASSERT_MSG (!traceDir.empty (), "Traces file not found in candidate paths");

            auto filePrefix = "chat";
            auto innerCodec = (codecType == SYNCODEC_TYPE_TRACE) ?
                                 new syncodecs::TraceBasedCodecWithScaling{
                                    traceDir,        // path to traces directory
                                    filePrefix,      // video filename
                                    SYNCODEC_DEFAULT_FPS,             // Default FPS: 30fps
                                    true} :          // fixed mode: image resolution doesn't change
                                 new syncodecs::HybridCodec{
                                    traceDir,        // path to traces directory
                                    filePrefix,      // video filename
                                    SYNCODEC_DEFAULT_FPS,             // Default FPS: 30fps
                                    true};           // fixed mode: image resolution doesn't change

            codec = new syncodecs::ShapedPacketizer{innerCodec, DEFAULT_PACKET_SIZE};
            break;
        }
        case SYNCODEC_TYPE_SHARING:
        {
            auto innerShCodec = new syncodecs::SimpleContentSharingCodec{};
            codec = new syncodecs::ShapedPacketizer{innerShCodec, DEFAULT_PACKET_SIZE};
            break;
        }
        default:  // defaults to perfect codec
            codec = new syncodecs::PerfectCodec{DEFAULT_PACKET_SIZE};
    }

    // update member variable
    m_codec = std::shared_ptr<syncodecs::Codec>{codec};
}

void RmcatSender::SetController (std::shared_ptr<rmcat::SenderBasedController> controller)
{
    m_controller = controller;
}

void RmcatSender::Setup (Ipv4Address destIP,
                         uint16_t destPort)
{
    if (!m_codec) {
        m_codec = std::make_shared<syncodecs::PerfectCodec> (DEFAULT_PACKET_SIZE);
    }

    if (!m_controller) {
        m_controller = std::make_shared<rmcat::DummyController> ();
    } else {
        m_controller->reset ();
    }

    m_destIP = destIP;
    m_destPort = destPort;
}

void RmcatSender::SetRinit (float r)
{
    m_initBw = r;
    if (m_controller) m_controller->setInitBw (m_initBw);
}

void RmcatSender::SetRmin (float r)
{
    m_minBw = r;
    if (m_controller) m_controller->setMinBw (m_minBw);
}

void RmcatSender::SetRmax (float r)
{
    m_maxBw = r;
    if (m_controller) m_controller->setMaxBw (m_maxBw);
}

void RmcatSender::StartApplication ()
{
    m_flowId = rand ();
    m_sequence = 0;

    NS_ASSERT (m_minBw <= m_initBw);
    NS_ASSERT (m_initBw <= m_maxBw);

    m_rVin = m_initBw;
    m_rSend = m_initBw;

    if (m_socket == NULL) {
        m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
        auto res = m_socket->Bind ();
        NS_ASSERT (res == 0);
    }
    m_socket->SetRecvCallback (MakeCallback (&RmcatSender::RecvPacket, this));

    //m_enqueueEvent = Simulator::Schedule (Seconds (0.0), &RmcatSender::EnqueuePacket, this);
    m_enqueueEvent = Simulator::ScheduleNow (&RmcatSender::EnqueuePacket, this);
    m_nextSendTstmp = 0;
}

void RmcatSender::StopApplication ()
{
    Simulator::Cancel (m_enqueueEvent);
    Simulator::Cancel (m_sendEvent);
    Simulator::Cancel (m_sendOversleepEvent);
    m_rateShapingBuf.clear ();
    m_rateShapingBytes = 0;
}

void RmcatSender::EnqueuePacket ()
{
    syncodecs::Codec& codec = *m_codec;
    codec.setTargetRate (m_rVin);
    ++codec; // Advance codec/packetizer to next frame/packet
    const auto bytesToSend = codec->first.size ();
    NS_ASSERT (bytesToSend > 0);
    NS_ASSERT (bytesToSend <= DEFAULT_PACKET_SIZE);

    m_rateShapingBuf.push_back (bytesToSend);
    m_rateShapingBytes += bytesToSend;

    NS_LOG_INFO (Simulator::Now().GetSeconds()<<"RmcatSender::EnqueuePacket, packet enqueued, packet length: " << bytesToSend
                 << ", buffer size: " << m_rateShapingBuf.size ()
                 << ", buffer bytes: " << m_rateShapingBytes);

    auto secsToNextEnqPacket = codec->second;
    Time tNext{Seconds (secsToNextEnqPacket)};
	NS_LOG_INFO("time value"<<tNext.GetSeconds());
    m_enqueueEvent = Simulator::Schedule (tNext, &RmcatSender::EnqueuePacket, this);

    if (!USE_BUFFER) {
        m_sendEvent = Simulator::ScheduleNow (&RmcatSender::SendPacket, this,
                                              secsToNextEnqPacket * 1000.);
        return;
    }

    if (m_rateShapingBuf.size () == 1) {
        // Buffer was empty
        const auto now = Simulator::Now ().GetMilliSeconds ();
        const auto msToNextSentPacket = now < m_nextSendTstmp ?
                                                m_nextSendTstmp - now : 0;
        NS_LOG_INFO ("(Re-)starting the send timer: now " << now
                     << ", bytesToSend " << bytesToSend
                     << ", msToNextSentPacket " << msToNextSentPacket
                     << ", m_rSend " << m_rSend
                     << ", m_rVin " << m_rVin
                     << ", secsToNextEnqPacket " << secsToNextEnqPacket);

        Time tNext{MilliSeconds (msToNextSentPacket)};
        m_sendEvent = Simulator::Schedule (tNext, &RmcatSender::SendPacket, this, msToNextSentPacket);
    }
}

void RmcatSender::SendPacket (uint64_t msSlept)
{
    NS_ASSERT (m_rateShapingBuf.size () > 0);
    NS_ASSERT (m_rateShapingBytes < MAX_QUEUE_SIZE_SANITY);

    const auto bytesToSend = m_rateShapingBuf.front ();
    NS_ASSERT (bytesToSend > 0);
    NS_ASSERT (bytesToSend <= DEFAULT_PACKET_SIZE);
    m_rateShapingBuf.pop_front ();
    NS_ASSERT (m_rateShapingBytes >= bytesToSend);
    m_rateShapingBytes -= bytesToSend;

    const auto now = Simulator::Now ().GetMilliSeconds ();

    NS_LOG_INFO ("RmcatSender::SendPacket, packet dequeued, packet length: " << bytesToSend
                 << ", buffer size: " << m_rateShapingBuf.size ()
                 << ", buffer bytes: " << m_rateShapingBytes);

    // Synthetic oversleep: random uniform [0% .. 1%]
    auto oversleepMs = msSlept * (rand () % 100) / 10000;
    Time tOver{MilliSeconds (oversleepMs)};
    m_sendOversleepEvent = Simulator::Schedule (tOver, &RmcatSender::SendOverSleep,
                                                this, m_sequence, bytesToSend);

    m_controller->processSendPacket (now, m_sequence++, bytesToSend);

    // schedule next sendData
    auto msToNextSentPacketD = static_cast<double> (bytesToSend) * 8. * 1000. / m_rSend;
    auto msToNextSentPacket = static_cast<uint64_t> (msToNextSentPacketD);

    if (!USE_BUFFER || m_rateShapingBuf.size () == 0) {
        // Buffer became empty
        m_nextSendTstmp = now + msToNextSentPacket;
        return;
    }

    Time tNext{MilliSeconds (msToNextSentPacket)};
    m_sendEvent = Simulator::Schedule (tNext, &RmcatSender::SendPacket, this, msToNextSentPacket);
}

void RmcatSender::SendOverSleep (uint32_t seq, uint32_t bytesToSend) {

    ns3::MediaHeader header;
    header.flow_id = m_flowId;
    header.sequence = seq;
    header.packet_size = bytesToSend;
    const auto now = Simulator::Now ().GetMilliSeconds ();
    header.send_tstmp = now;

    auto packet = Create<Packet> (bytesToSend);
    packet->AddHeader (header);

    NS_LOG_INFO ("RmcatSender::SendOverSleep, " << packet->ToString ());
    m_socket->SendTo (packet, 0, InetSocketAddress{m_destIP, m_destPort});
}

void RmcatSender::RecvPacket (Ptr<Socket> socket)
{
    Address remoteAddr;
    auto Packet = m_socket->RecvFrom (remoteAddr);
    NS_ASSERT (Packet);

    auto rIPAddress = InetSocketAddress::ConvertFrom (remoteAddr).GetIpv4 ();
    auto rport = InetSocketAddress::ConvertFrom (remoteAddr).GetPort ();
    NS_ASSERT (rIPAddress == m_destIP);
    NS_ASSERT (rport == m_destPort);

    // get the feedback header
    FeedbackHeader header;
    NS_LOG_INFO (Simulator::Now().GetSeconds()<<"RmcatSender::RecvPacket, " << Packet->ToString ());
    Packet->RemoveHeader (header);
    NS_ASSERT (header.flow_id == m_flowId);

    const auto now = Simulator::Now ().GetMilliSeconds ();
    NS_ASSERT (header.receive_tstmp <= now);

    m_controller->processFeedback (now,
                                   header.sequence,
                                   header.receive_tstmp);
    CalcBufferParams (now);
}

void RmcatSender::CalcBufferParams (uint64_t now)
{
    //Calculate rate shaping buffer parameters
    const auto r_ref = m_controller->getBandwidth (now); // in bps
    float bufferLen;
    //Purpose: smooth out timing issues between send and receive
    // feedback for the common case: buffer oscillating between 0 and 1 packets
    if (m_rateShapingBuf.size () > 1) {
        bufferLen = static_cast<float> (m_rateShapingBytes);
    } else {
        bufferLen = 0;
    }

    syncodecs::Codec& codec = *m_codec;

    // TODO (deferred):  encapsulate rate shaping buffer in a separate class
    if (USE_BUFFER && static_cast<bool> (codec)) {
        const float fps = 1. / static_cast<float>  (codec->second);
        m_rVin = std::max<float> (m_minBw, r_ref - BETA_V * 8. * bufferLen * fps);
        m_rSend = r_ref + BETA_S * 8. * bufferLen * fps;
        NS_LOG_INFO ("New rate shaping buffer parameters: r_ref " << r_ref
                     << ", m_rVin " << m_rVin
                     << ", m_rSend " << m_rSend
                     << ", fps " << fps
                     << ", buffer length " << bufferLen);
    } else {
        m_rVin = r_ref;
        m_rSend = r_ref;
    }
}

}
