// bafwidth  report and nack or similar quic ack with block
#ifndef MP_RECEIVER_H
#define MP_RECEIVER_H
#include "mpcommon.h"
#include<map>
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
namespace ns3
{
//data processing should be in Receiver,not at the ReceivePath
//for disconnect message
class ReceivePath;
class MpReceiver:public Application
{
public:
	MpReceiver();
	virtual ~MpReceiver();
	void Init(uint32_t uid,uint32_t cid);
	uint32_t GetUid() {return m_uid;}
	uint32_t GetCid(){return m_cid;}
	void Bind(uint16_t port);
	void OnPacketPlugin(PathAddressInfo info,Ptr<Packet> packet);
private:
	void RecvPacket(Ptr<Socket>socket);
	virtual void StartApplication();
	virtual void StopApplication();
	std::map<Ipv4Address,Ptr<Socket>,comp_socket>m_sockets;
	std::map<PathAddressInfo,ReceivePath*,comp_path> m_paths;
	//std::map<uint32_t,sim_segment_t>m_rxBuffer;
	uint32_t m_cid{4321};
	uint32_t m_uid{1};//server
	bool m_running;
	uint32_t m_pathNum{0};
};	
}
#endif
