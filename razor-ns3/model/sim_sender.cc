#include<utility>
#include"sim_sender.h"
#include"razor_header.h"
#include"ns3/log.h"
#include"ns3/nstime.h"
#include "ns3/packet.h"
#include<string>
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("SimSender");
SimSender::SimSender()
{
	m_transport_seq=0;
	m_active=true;
	m_first_ts=-1;
	m_remb=1;//0 open receiver bandwidth estimate
	m_paceTime=5;
	m_segmentSize=1000;
	m_lossFraction=0;
	m_paceQueueLen=3000; //300;  change it
	m_maxSplitPackets=1000;
	m_cc=razor_sender_create((void*)this,&SimSender::ChangeBitRate,(void*)this,
			&SimSender::PacePacketSend,m_paceQueueLen);


}
SimSender::~SimSender()
{
	if(m_cc!=NULL)
	{
		razor_sender_destroy(m_cc);
	}
	m_cc=NULL;
}
void SimSender::Reset()
{
	m_frame_id=0;
	m_packet_id=0;
	m_transport_seq=0;
	m_first_ts=-1;
	m_lossFraction=0;
	m_cache.clear();
	if(m_cc!=NULL)
	{
		razor_sender_destroy(m_cc);
		m_cc=NULL;
	}
	if(!m_paceEvent.IsExpired())
	{
		m_paceEvent.Cancel();
	}
	m_cc=razor_sender_create((void*)this,&SimSender::ChangeBitRate,(void*)this,
			&SimSender::PacePacketSend,m_paceQueueLen);
	m_paceEvent=Simulator::ScheduleNow(&SimSender::PaceHeartBeat,this);
}
void SimSender::SetHeader(sim_header_t header)
{
	m_header=header;
	m_header.mid=RazorProtoType::MIN_MSG_ID;
}
void SimSender::UpdateRtt(uint32_t rtt,uint32_t rtt_var)
{
	if(m_cc!=NULL)
	{
		m_cc->update_rtt(m_cc,rtt+rtt_var);
	}
}
void SimSender::ProcessFeedBack(sim_feedback_t feedback)
{
	//NS_LOG_FUNCTION_NOARGS();
	if(m_cc!=NULL)
	{
		m_cc->on_feedback(m_cc,feedback.feedback,feedback.feedback_size);
	}
}
void SimSender::SetBitRates(uint32_t min_bitrate, uint32_t start_bitrate, uint32_t max_bitrate)
{
	if(m_cc!=NULL)
	{
		m_cc->set_bitrates(m_cc,min_bitrate,start_bitrate,max_bitrate);
	}
}
uint16_t SimSender::FrameSplit(uint16_t splits[],uint32_t size)
{
	uint16_t ret, i;
	uint16_t remain_size;
	if(size<m_segmentSize)
	{
		ret=1;
		splits[0]=size;
	}else
	{
		ret=size/m_segmentSize;
		for(i=0;i<ret;i++)
		{
			splits[i]=m_segmentSize;
		}
		remain_size=size%m_segmentSize;
		if(remain_size>0)
		{
			splits[ret]=remain_size;
			ret++;
		}
	}
	return ret;
}
void SimSender::PutFakeData(uint8_t ftype,uint32_t size)
{
	NS_ASSERT((size/m_segmentSize)<m_maxSplitPackets);
	uint16_t splits[m_maxSplitPackets],total,i;
	total=FrameSplit(splits,size);
	int64_t now_ts;
	uint32_t timestamp;
	sim_segment_t seg;
	now_ts=Simulator::Now().GetMilliSeconds();
	uint8_t header_len=sizeof(sim_header_t)+sizeof(sim_segment_t);
	if(m_first_ts==-1)
	{
		timestamp=0;
		m_first_ts=now_ts;
	}else
	{
		timestamp=(uint32_t)(now_ts-m_first_ts);
	}
	for(i=0;i<total;i++)
	{
		seg.packet_id=m_packet_id;
		seg.fid=m_frame_id;
		seg.ftype=ftype;
		seg.index=i;
		seg.total=total;

		seg.remb=m_remb;
		seg.send_ts=0;
		seg.timestamp=timestamp;
		seg.transport_seq=0;
		seg.data_size=splits[i];
		m_cache.insert(std::make_pair(seg.packet_id,seg));
		m_cc->add_packet(m_cc,seg.packet_id,0,seg.data_size+header_len);
		m_packet_id++;
	}
	m_frame_id++;
	//NS_LOG_INFO("splits packet "<<total);
}
void SimSender::SetSendSegmentCallback(Callback<void,Ptr<Packet>>sendPacket)
{
	m_sendPacket=sendPacket;
}
void SimSender::SetVideoBitChangeCallback(Callback<void,uint32_t> changeBitRate)
{
	m_changeBitRate=changeBitRate;
}
void SimSender::ChangeBitRate(void* trigger, uint32_t bitrate, uint8_t fraction_loss, uint32_t rtt)
{
	double now=Simulator::Now().GetSeconds();
	SimSender *obj=static_cast<SimSender*>(trigger);
	uint32_t overhead_bitrate, per_packets_second, payload_bitrate, video_bitrate_kbps;
	double loss;
	uint8_t header_len=sizeof(sim_header_t)+sizeof(sim_segment_t);
	uint32_t packet_size_bit=(header_len+obj->m_segmentSize)*8;
	per_packets_second = (bitrate + packet_size_bit - 1) / packet_size_bit;
	overhead_bitrate = per_packets_second * header_len * 8;
	payload_bitrate = bitrate - overhead_bitrate;
	if(obj->m_lossFraction==0)
	{
		obj->m_lossFraction=fraction_loss;
	}else
	{
		obj->m_lossFraction=(3*obj->m_lossFraction+fraction_loss)/4;
	}
	loss=(double)obj->m_lossFraction/255.0;
	if (loss > 0.5)
		loss = 0.5;
	video_bitrate_kbps = (uint32_t)((1.0 - loss) * payload_bitrate) / 1000;
	NS_LOG_INFO(std::to_string(now)<<" "<<video_bitrate_kbps<<" l "<<std::to_string(loss));
	if(!obj->m_changeBitRate.IsNull())
	{
		//to prevent channel overuse 
		uint32_t protect_rate=video_bitrate_kbps;
		obj->m_changeBitRate(protect_rate);//video_bitrate_kbps
	}

}
void SimSender::PacePacketSend(void* handler, uint32_t packet_id, int retrans, size_t size)
{
	//NS_LOG_INFO(Simulator::Now().GetMilliSeconds()<<"packet send "<<packet_id<<"size "<<size);
	SimSender *obj=static_cast<SimSender*>(handler);
	if(!obj->m_sendPacket.IsNull())
	{
		//constructor Packet;
		std::map<uint32_t,sim_segment_t>::iterator iter;
		iter=obj->m_cache.find(packet_id);
		if(iter==obj->m_cache.end())
		{
			NS_LOG_ERROR("packet not found"<<packet_id);
			return ;
		}
		sim_segment_t segment=iter->second;
		segment.send_ts=(uint16_t)(Simulator::Now().GetMilliSeconds()-obj->m_first_ts-segment.timestamp);
		segment.transport_seq=++(obj->m_transport_seq);
		obj->m_cache.erase(iter);
		if(!obj->m_sentSeqCb.IsNull()){
			obj->m_sentSeqCb(segment.packet_id);
		}
		uint8_t header_len=sizeof(sim_header_t)+sizeof(sim_segment_t);
		if(obj->m_cc)
		{
			obj->m_cc->on_send(obj->m_cc,segment.transport_seq,header_len+segment.data_size);
		}
		//NS_LOG_INFO("packet_id"<<packet_id<<"seq"<<segment.transport_seq);
		sim_header_t sHeader=obj->m_header;
		sHeader.mid=RazorProtoType::SIM_SEG;
		RazorHeader header;
		header.SetHeader(sHeader);
		header.SetMessagePayload((void*)&segment,sizeof(segment));
		Ptr<Packet> packet=Create<Packet>((uint32_t)segment.data_size);
		packet->AddHeader(header);
		//NS_LOG_INFO("send media packet "<<packet_id);
		obj->m_sendPacket(packet);
	}
}
//must be call after SetBitRates;
void SimSender::PaceStart()
{
  m_paceEvent=Simulator::ScheduleNow(&SimSender::PaceHeartBeat,this);
}
void SimSender::PaceHeartBeat()
{
	if(m_paceEvent.IsExpired())
	{
		if(m_cc==NULL)
		{
			NS_LOG_ERROR("m_cc is null,should not happen");
			return;
		}
		m_cc->heartbeat(m_cc);
		Time paceTime=MilliSeconds((uint64_t)m_paceTime);
		m_paceEvent=Simulator::Schedule(paceTime,&SimSender::PaceHeartBeat,this);
	}
}
}
