#ifndef SCREAM_SENDER_H
#define SCREAM_SENDER_H
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/event-id.h"
#include "rtpqueue.h"
#include "videoenc.h"
#include "screamTx.h"
namespace ns3
{
class ScreamSender:public ns3::Application
{
public:
	ScreamSender();
	~ScreamSender();
	void SetTraceFilePath(char*trace);
	void InitialSetup(Ipv4Address dest_ip,uint16_t dest_port);
	VideoEnc* GetEncoder(){return m_videoEnc;}
private:
	void RecvPacket(Ptr<Socket>socket);
	virtual void StartApplication();
	virtual void StopApplication();
	void Send(Ptr<Packet>packet);
	void Process();
    Ipv4Address m_peerIp;
    uint16_t m_peerPort;
    Ptr<Socket> m_socket;
	int64_t m_videoTick{40};//40ms;
	int64_t m_nextFrameTime;
	int64_t m_tick{5};//5ms;
	float m_frameRate{25};
	EventId m_tickEvent;
	RtpQueue *m_rtpQueue;
	VideoEnc *m_videoEnc;
	ScreamTx *m_screamTx;
	bool m_running;
	uint32_t m_ssrc;
	int m_nextCallN{-1};
};
}
#endif
