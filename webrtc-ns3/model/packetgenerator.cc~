#include "packetgenerator.h"
#include <math.h>
#include "ns3/log.h"
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("PacketGenerator");
void PacketGenerator::Start()
{
	m_timer=Simulator::ScheduleNow(&PacketGenerator::Generate,this);
}
void PacketGenerator::Stop()
{
	m_timer.Cancel();
}
void PacketGenerator::Generate()
{
	if(m_timer.IsExpired())
	{
		uint32_t byte=ceil(0.001*m_fs*m_bitRate/8);
		//NS_LOG_FUNCTION(byte);
		if(m_target)
		{
			m_target->TargetReceive(byte);
		}
		Time next=MilliSeconds(m_fs);
		m_timer=Simulator::Schedule(next,&PacketGenerator::Generate,this);
	}
}
void PacketGenerator::OnNetworkChanged(uint32_t bitrate_bps,
                      uint8_t fraction_loss,  // 0 - 255.
                      int64_t rtt_ms,
                      int64_t probing_interval_ms)
{
	double lossRate=fraction_loss/255;
	double now=Simulator::Now().GetSeconds();
	float kbps=bitrate_bps/1000;
	NS_LOG_INFO(now<<" "<<m_bitRate/1000<<" "<<bitrate_bps/1000);
	if(!m_rateCb.IsNull()){m_rateCb(kbps);}
	m_bitRate=bitrate_bps;
}
}
