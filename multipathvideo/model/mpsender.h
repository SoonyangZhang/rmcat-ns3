#ifndef MP_SENDER_H
#define MP_SENDER_H
#include "mpcommon.h"
#include "path.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/data-rate.h"
#include <map>
namespace ns3
{
//consider nack situation
class MpSender:public Application
{
public:
	MpSender();
	virtual ~MpSender();
	void Init(uint32_t cid,uint32_t uid);
	uint32_t GetUid() {return m_uid;}
	uint32_t GetCid(){return m_cid;}
	void AddPath(Ipv4Address destIp,uint16_t destPort);
	void PathConnect();
	void FakePacket();
	void Schedule(uint8_t ftype,uint32_t byte);//packet schedule rule;
	void OnPacePacket(PathAddressInfo info,uint32_t packet_id, uint16_t seq,int retrans,uint32_t size);//for video segment
	void OnPacket(PathAddressInfo info,Ptr<Packet> packet);//like ping pong message
	void OnBwProbe(PathAddressInfo info,Path*path,uint32_t size);//may ugly,but have to
	void OnChangeBitRate();
	uint16_t FrameSplit(uint16_t splits[],uint32_t size);
private:
	virtual void StartApplication();
	virtual void StopApplication();
	Path *GetPath(Address &localAddr,Address &remoteAddr);
	void RecvPacket(Ptr<Socket>socket);
	std::map<Ipv4Address,Ptr<Socket>,comp_socket> m_sockets;
	std::map<PathAddressInfo,Path*,comp_path> m_paths;
	EventId m_gennerateTimer;
	std::map<uint32_t,segment_ref_t>m_txBuffer;
	int64_t m_currentPacketId{-1};
	uint32_t m_packetId{0};//counter;
	uint32_t m_frameId;
	DataRate m_aggregateBw;
	Time m_packetClear{MilliSeconds(500)};//clean too old packets, for nack;500ms
	uint32_t m_cid{1234};
	uint32_t m_uid{0};//client
	bool m_active{false};
	uint32_t m_pathNum{0};
	uint32_t m_fs{40};//40 milliseconds
	uint32_t m_encodeRate{80000};//in bps;
	uint32_t m_minRate{80000};//10KB
	uint32_t m_maxRate{16000000};//2MB
	bool m_slowStart{true};
	int32_t m_firstTs{-1};//first frame;
	uint32_t m_maxSplitPackets{1000};
	uint32_t m_segmentSize{1000};
};
}
#endif
