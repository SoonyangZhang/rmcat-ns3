#ifndef WEBRTC_RECEIVER_H
#define WEBRTC_RECEIVER_H
#include "processmodule.h"
#include "modules/congestion_controller/include/receive_side_congestion_controller.h"
#include "modules/rtp_rtcp/source/rtcp_packet/transport_feedback.h"
#include "modules/pacing/packet_router.h"
#include "simulationclock.h"
#include "webrtcheader.h"
#include "ns3/socket.h"
#include "ns3/mpcommon.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include <memory>
#include "webrtcsender.h"
#include "ns3/callback.h"
namespace ns3
{
class WebrtcReceiver:public ns3::Application,webrtc::PacketRouter
{
public:
	WebrtcReceiver();
	~WebrtcReceiver();
	void Init(uint32_t uid,uint32_t cid);
	void Bind(uint16_t port);
	void AddSendRtpModule(webrtc::RtpRtcp* rtp_module, bool remb_candidate);
	void RemoveSendRtpModule(webrtc::RtpRtcp* rtp_module);

	void SetWebrtcSender(Ptr<WebrtcSender>sender){m_sender=sender;}
	typedef Callback<void,uint64_t> DelayCallback;
	void SetDelayCallback(DelayCallback cb){m_delayCb=cb;}
	//void AddReceiveRtpModule(webrtc::RtcpFeedbackSenderInterface* rtcp_sender,
	  //                         bool remb_candidate);
	//void RemoveReceiveRtpModule(webrtc::RtcpFeedbackSenderInterface* rtcp_sender);
  	void AddReceiveRtpModule(webrtc::RtpRtcp* rtp_module,bool remb_candidate);
  	void RemoveReceiveRtpModule(webrtc::RtpRtcp* rtp_module);
	  // Implements PacedSender::Callback.
	bool TimeToSendPacket(uint32_t ssrc,
	                        uint16_t sequence_number,
	                        int64_t capture_timestamp,
	                        bool retransmission,
	                        const webrtc::PacedPacketInfo& packet_info) override;

	size_t TimeToSendPadding(size_t bytes,
	                           const webrtc::PacedPacketInfo& packet_info) override;

	void SetTransportWideSequenceNumber(uint16_t sequence_number);
	uint16_t AllocateSequenceNumber() override;

	  // Called every time there is a new bitrate estimate for a receive channel
	  // group. This call will trigger a new RTCP REMB packet if the bitrate
	  // estimate has decreased or if no RTCP REMB packet has been sent for
	  // a certain time interval.
	  // Implements RtpReceiveBitrateUpdate.
	void OnReceiveBitrateChanged(const std::vector<uint32_t>& ssrcs,
	                               uint32_t bitrate_bps) override;

	  // Ensures remote party notified of the receive bitrate limit no larger than
	  // |bitrate_bps|.
	void SetMaxDesiredReceiveBitrate(int64_t bitrate_bps);

	  // Send REMB feedback.
	bool SendRemb(int64_t bitrate_bps, const std::vector<uint32_t>& ssrcs);

	  // Send transport feedback packet to send-side.
	bool SendTransportFeedback(webrtc::rtcp::TransportFeedback* packet) override;

private:
	virtual void StartApplication();
	virtual void StopApplication();
	void SendPing(void);
	void RecvPacket(Ptr<Socket> socket);
	void Send(Ptr<Packet> packet);
	void ProcessConnect(proto_connect_t con);
	void ProcessPing(proto_ping_t ping);
	void ProcessPong(proto_pong_t pong);
	std::unique_ptr<webrtc::ReceiveSideCongestionController> m_controller;
	SimulationClock m_clock;
	ProcessModule *m_pm;
	bool m_firstPacket{true};
	Ptr<Socket> m_socket;
    Ipv4Address m_peerIp;
    uint16_t m_peerPort;
	uint16_t m_port;
	uint32_t m_uid{4321};
	uint32_t m_cid{12};
	uint32_t m_peerUid{0};
    uint64_t m_sumRtt{0};
    uint64_t m_numRtt{0};
    uint64_t m_rtt{100};
    uint64_t m_maxRtt{0};
    EventId m_pingTimer;
    uint8_t m_pathState{path_idle};
    webrtc::RemoteBitrateEstimator *m_rbe;
    bool m_running{false};
    uint16_t m_lastPacket{0};
    Ptr<WebrtcSender> m_sender;//I know this ugly,but I had to get the first timestap for delay computation.
    DelayCallback m_delayCb;

};
}
#endif
