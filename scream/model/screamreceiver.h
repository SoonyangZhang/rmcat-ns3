#ifndef SCREAM_RECEIVER_H
#define SCREAM_RECEIVER_H
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/event-id.h"
#include "ns3/callback.h"
#include "screamRx.h"
#include "scream-header.h"
namespace ns3
{
class ScreamReceiver:public ns3::Application
{
public:
	ScreamReceiver();
	~ScreamReceiver();
	void Bind(uint16_t port);
	typedef Callback<void,uint64_t> DelayCallback;
	void SetDelayCallback(DelayCallback cb){m_delayCb=cb;}
private:
	void RecvPacket(Ptr<Socket>socket);
	virtual void StartApplication();
	virtual void StopApplication();
	void Send(Ptr<Packet>packet);
	void Process();
    Ipv4Address m_peerIp;
    uint16_t m_peerPort;
	uint64_t m_rtcpFbInterval{5};
	ScreamRx *m_screamRx;
	Ptr<Socket> m_socket;
	uint16_t m_port;
	bool  m_running{false};
	EventId m_feedbackEvent;
	bool m_first{true};
	DelayCallback m_delayCb;
};
}
#endif
