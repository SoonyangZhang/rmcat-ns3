#ifndef WEBRTC_SENDER_H
#define WEBRTC_SENDER_H
#include "processmodule.h"
#include "rtc_base/socket.h"
#include "test/field_trial.h"
#include "system_wrappers/include/field_trial.h"
#include "logging/rtc_event_log/rtc_event_log.h"
#include "modules/congestion_controller/include/send_side_congestion_controller.h"
#include "modules/rtp_rtcp/source/rtcp_packet/transport_feedback.h"
#include "modules/pacing/paced_sender.h"
#include "packetgenerator.h"
#include "simulationclock.h"
#include "webrtcheader.h"
#include "ns3/simulator.h"
#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/webmpcommon.h"
#include <memory>
#include<map>
namespace ns3
{
static const int kTargetBitrateBps = 800000;
constexpr unsigned kFirstClusterBps = 900000;
constexpr unsigned kSecondClusterBps = 1800000;

// The error stems from truncating the time interval of probe packets to integer
// values. This results in probing slightly higher than the target bitrate.
// For 1.8 Mbps, this comes to be about 120 kbps with 1200 probe packets.
constexpr int kBitrateProbingError = 150000;

const float kPaceMultiplier = 2.5f;
void TraceBitrate(uint32_t bps);
class WebrtcSender:public Application,Target,webrtc::PacedSender::PacketSender
{
public:
	WebrtcSender();
	~WebrtcSender();
	PacketGenerator* GetPacketGenerator(){return &m_packetGenerator;}
	void InitialSetup(Ipv4Address dest_ip,uint16_t dest_port);
	void Connect();
	void Stop();
	void TargetReceive(uint32_t len) override;
	uint16_t FrameSplit(uint16_t splits[],uint32_t size);
    virtual bool TimeToSendPacket(uint32_t ssrc,
                                  uint16_t sequence_number,
                                  int64_t capture_time_ms,
                                  bool retransmission,
                                  const webrtc::PacedPacketInfo& cluster_info);
    virtual size_t TimeToSendPadding(size_t bytes,
                                     const webrtc::PacedPacketInfo& cluster_info);
    void Send(Ptr<Packet>packet);
    int64_t GetFirstTs(){return m_first_ts;}
private:
	virtual void StartApplication();
	virtual void StopApplication();
	void SendPing(void);
	void RecvPacket(Ptr<Socket> socket);
	void ProcessPing(proto_ping_t ping);
	void ProcessPong(proto_pong_t pong);
	void ProcessConnectAck(proto_connect_ack_t ack);
	PacketGenerator m_packetGenerator;
	webrtc::PacedSender *m_send_bucket;
	webrtc::SendSideCongestionController * m_controller;
	ProcessModule *m_pm;
	SimulationClock m_clock;
	Ptr<Socket> m_socket;
	std::map<uint16_t,proto_segment_t> m_cache;
	uint32_t m_frameId{0};
	uint32_t m_packetId{0};
	uint32_t m_uid{11};
	uint32_t m_cid{12345};
	uint32_t m_ssrc{12345};//m_cid
	uint16_t m_sequenceNumber{0};
	uint16_t m_maxSplitPackets{1000};
	uint16_t m_segmentSize{1000};
    Ipv4Address m_peerIP;
    uint16_t m_peerPort;
    int64_t m_first_ts{-1};
    uint64_t m_sumRtt{0};
    uint64_t m_numRtt{0};
    uint64_t m_rtt{100};
    uint64_t m_maxRtt{0};
    uint8_t m_pathState{path_idle};
    EventId m_pingTimer;
    EventId m_rxTimer;
    webrtc::RtcEventLogNullImpl m_nullLog;
    bool m_running{false};
};
}
#endif
