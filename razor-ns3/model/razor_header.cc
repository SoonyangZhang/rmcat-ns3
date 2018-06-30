#include"razor_header.h"
#include "ns3/log.h"
#include <string>
#include<string.h>
namespace ns3
{
NS_LOG_COMPONENT_DEFINE ("RazorHeader");
NS_OBJECT_ENSURE_REGISTERED (RazorHeader);
RazorHeader::RazorHeader()
{
	m_ver=protocol_ver;
	memset(m_message,0,MAX_MSGLEN);
}
RazorHeader::~RazorHeader(){}
TypeId RazorHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("RazorHeader")
      .SetParent<Header> ()
      .AddConstructor<RazorHeader> ()
    ;
    return tid;	
}
TypeId RazorHeader::GetInstanceTypeId (void) const
{
	return GetTypeId();
}
uint32_t RazorHeader::GetSerializedSize (void) const
{
	uint32_t hdlen=(sizeof(m_ver)+sizeof(m_mid)+sizeof(m_uid));
	uint32_t msglen=0;
	switch (m_mid)
	{
		case SIM_CONNECT:
		msglen=sizeof(sim_connect_t);
		break;
		case SIM_CONNECT_ACK:
		msglen=sizeof(sim_connect_ack_t);
		break;
		case SIM_DISCONNECT:
		msglen=sizeof(sim_disconnect_t);
		break;
		case SIM_DISCONNECT_ACK:
		msglen=sizeof(sim_disconnect_ack_t);
		break;
		case SIM_PING:
		msglen=sizeof(sim_ping_t);
		break;
		case SIM_PONG:
		msglen=sizeof(sim_pong_t);
		break;
		case SIM_SEG:
		msglen=sizeof(sim_segment_t);
		break;
		case SIM_SEG_ACK:
		msglen=sizeof(sim_segment_ack_t);
		break;
		case SIM_FEEDBACK:
		msglen=sizeof(sim_feedback_t);
		break;
		default:
		msglen=0;
		break;		
	}
	return hdlen+msglen;
}
void RazorHeader::SetHeader(sim_header_t &header)
{
	m_ver=header.ver;
	m_mid=header.mid;
	m_uid=header.uid;
}
void RazorHeader::GetHeader(sim_header_t &header)
{
	header.ver=m_ver;
	header.mid=m_mid;
	header.uid=m_uid;
}
uint32_t RazorHeader::SetMessagePayload(void *buf,uint32_t buflen)
{
	NS_ASSERT_MSG(0<buflen&&buflen<=MAX_MSGLEN,"out of lenfth");
	uint32_t len=GetMessageLen(m_mid,buf);
	uint8_t *byte=(uint8_t*)buf;	
	memcpy(m_message,byte,len);
	return len;
}
uint32_t RazorHeader::GetMessage(void *buf,uint32_t buflen)
{
	uint32_t len=GetMessageLen(m_mid,(void*)m_message);
	if(len>buflen){return 0;}
	memcpy(buf,m_message,len);
	return len;
}
uint32_t RazorHeader::GetMessageLen(uint8_t mid,void *message)
{
	uint32_t len;
	switch (mid)
	{
		case SIM_CONNECT:
		{
			sim_connect_t *con=(sim_connect_t*)message;
			len=sizeof(con->cid)+sizeof(con->token_size)+con->token_size*sizeof(uint8_t);
		}
		break;
		case SIM_CONNECT_ACK:
		{
			len=sizeof(sim_connect_ack_t);
		}
		case SIM_DISCONNECT:
		{
			len=sizeof(sim_disconnect_t);
		}
		case SIM_DISCONNECT_ACK:
		{
			len=sizeof(sim_disconnect_ack_t);
		}
		case SIM_PING:
		case SIM_PONG:
		{
			len=sizeof(sim_ping_t);
		}
		break;
		case SIM_SEG:
		{
			len=sizeof(sim_segment_t);
		}
		break;
		case SIM_SEG_ACK:
		{
			sim_segment_ack_t *seg_ack=(sim_segment_ack_t*)message;
			len=sizeof(seg_ack->acked_packet_id)+sizeof(seg_ack->base_packet_id)+
					sizeof(seg_ack->nack_num)+seg_ack->nack_num*sizeof(uint16_t);
		}
		break;
		case SIM_FEEDBACK:
		{
			sim_feedback_t *feedback=(sim_feedback_t*)message;
			len=sizeof(feedback->base_packet_id)+sizeof(feedback->feedback_size)+
					feedback->feedback_size*sizeof(uint8_t);
		}
		break;
		default :
			len=0;
			break;
	}
	return len;
}
void RazorHeader::Serialize (Buffer::Iterator start) const
{
	NS_LOG_FUNCTION (this << &start);
	Buffer::Iterator i = start;
	i.WriteU8(m_ver);
	i.WriteU8(m_mid);
	i.WriteHtonU32(m_uid);
	NS_ASSERT(MIN_MSG_ID<m_mid&&m_mid<MAX_MSG_ID);
	switch (m_mid)
	{
		case SIM_CONNECT:
		{
			sim_connect_t *con=(sim_connect_t*)m_message;
			i.WriteHtonU32(con->cid);
			uint16_t token_size=con->token_size;
			i.WriteHtonU16(token_size);
			int j=0;
			for(j=0;j<token_size;j++)
			{
				i.WriteU8(con->token[j]);
			}
		}
		break;
		case SIM_CONNECT_ACK:
		{
			sim_connect_ack_t *ack=(sim_connect_ack_t*)m_message;
			i.WriteHtonU32(ack->cid);
			i.WriteHtonU32(ack->result);
		}
		break;
		case SIM_DISCONNECT:
		{
			sim_disconnect_t *discon=(sim_disconnect_t*)m_message;
			i.WriteHtonU32(discon->cid);
		}
		break;
		case SIM_DISCONNECT_ACK:
		{
			sim_disconnect_ack_t *discon_ack=(sim_disconnect_ack_t*)m_message;
			i.WriteHtonU32(discon_ack->cid);
		}
		break;
		case SIM_PING:
		{
			sim_ping_t *ping=(sim_ping_t*)m_message;
			i.WriteHtonU64((uint64_t)ping->ts);
		}
		break;
		case SIM_PONG:
		{
			sim_pong_t *pong=(sim_pong_t*)m_message;
			i.WriteHtonU64((uint64_t)pong->ts);
		}
		break;
		case SIM_SEG:
		{
			sim_segment_t *seg=(sim_segment_t*)m_message;
			i.WriteHtonU32(seg->packet_id);
			i.WriteHtonU32(seg->fid);
			i.WriteHtonU32(seg->timestamp);
			i.WriteHtonU16(seg->index);
			i.WriteHtonU16(seg->total);
			i.WriteU8(seg->ftype);
			i.WriteU8(seg->remb);
			i.WriteHtonU16(seg->send_ts);
			i.WriteHtonU16(seg->transport_seq);
			i.WriteHtonU16(seg->data_size);
		}
		break;
		case SIM_SEG_ACK:
		{
			sim_segment_ack_t *seg_ack=(sim_segment_ack_t*)m_message;
			i.WriteHtonU32(seg_ack->base_packet_id);
			i.WriteHtonU32(seg_ack->acked_packet_id);
			uint8_t nack_num=seg_ack->nack_num;
			i.WriteU8(nack_num);
			uint8_t index=0;
			for(index=0;index<nack_num;index++)
			{
				i.WriteHtonU16(seg_ack->nack[index]);
			}
		}
		break;
		case SIM_FEEDBACK:
		{
			sim_feedback_t *feedback=(sim_feedback_t*)m_message;
			i.WriteHtonU32(feedback->base_packet_id);
			uint16_t feedback_size=feedback->feedback_size;
			i.WriteHtonU16(feedback_size);
			uint16_t index=0;
			for(index=0;index<feedback_size;index++)
			{
				i.WriteU8(feedback->feedback[index]);
			}
		}
		break;
		default:
		break;		
	}
	
}
uint32_t RazorHeader::Deserialize (Buffer::Iterator start)
{
	NS_LOG_FUNCTION (this << &start);
    Buffer::Iterator i = start;	
	m_ver=i.ReadU8();
	m_mid=i.ReadU8();
	m_uid=i.ReadNtohU32();
	switch (m_mid)
	{
	case SIM_CONNECT:
	{
		sim_connect_t *con=(sim_connect_t*)m_message;
		con->cid=i.ReadNtohU32();
		con->token_size=i.ReadNtohU16();
		uint16_t token_size=con->token_size;
		int j=0;
		for(j=0;j<token_size;j++)
		{
			con->token[j]=i.ReadU8();
		}
	}
	break;
	case SIM_CONNECT_ACK:
	{
		sim_connect_ack_t *ack=(sim_connect_ack_t*)m_message;
		ack->cid=i.ReadNtohU32();
		ack->result=i.ReadNtohU32();
	}
	break;
	case SIM_DISCONNECT:
	{
		sim_disconnect_t *discon=(sim_disconnect_t*)m_message;
		discon->cid=i.ReadNtohU32();
	}
	break;
	case SIM_DISCONNECT_ACK:
	{
		sim_disconnect_ack_t *discon_ack=(sim_disconnect_ack_t*)m_message;
		discon_ack->cid=i.ReadNtohU32();
	}
	break;
	case SIM_PING:
	{
		sim_ping_t *ping=(sim_ping_t*)m_message;
		ping->ts=(int64_t)i.ReadNtohU64();
	}
	break;
	case SIM_PONG:
	{
		sim_pong_t * pong=(sim_pong_t*)m_message;
		pong->ts=(int64_t)i.ReadNtohU64();
	}
	break;
	case SIM_SEG:
	{
		sim_segment_t *seg=(sim_segment_t*)m_message;
		seg->packet_id=i.ReadNtohU32();
		seg->fid=i.ReadNtohU32();
		seg->timestamp=i.ReadNtohU32();
		seg->index=i.ReadNtohU16();
		seg->total=i.ReadNtohU16();
		seg->ftype=i.ReadU8();
		seg->remb=i.ReadU8();
		seg->send_ts=i.ReadNtohU16();
		seg->transport_seq=i.ReadNtohU16();
		seg->data_size=i.ReadNtohU16();
	}
	break;
	case SIM_SEG_ACK:
	{
		sim_segment_ack_t *seg_ack=(sim_segment_ack_t*)m_message;
		seg_ack->base_packet_id=i.ReadNtohU32();
		seg_ack->acked_packet_id=i.ReadNtohU32();
		seg_ack->nack_num=i.ReadU8();
		uint8_t nack_num=seg_ack->nack_num;
		uint8_t index=0;
		for(index=0;index<nack_num;index++)
		{
			seg_ack->nack[index]=i.ReadNtohU16();
		}
	}
	break;
	case SIM_FEEDBACK:
	{
		sim_feedback_t *feedback=(sim_feedback_t*)m_message;
		feedback->base_packet_id=i.ReadNtohU32();
		feedback->feedback_size=i.ReadNtohU16();
		uint16_t feedback_size=feedback->feedback_size;
		uint16_t index=0;
		for(index=0;index<feedback_size;index++)
		{
			feedback->feedback[index]=i.ReadU8();
		}
	}
	break;
	default:
	break;
	}
	return GetSerializedSize();
}
void RazorHeader::Print(std::ostream &os) const
{
    NS_LOG_FUNCTION (this << &os);
	os<<"m_ver"<<m_ver<<"messgae id";
	switch(m_mid)
	{
		case SIM_CONNECT:
		os<<"sim_connect";
		break;
		case SIM_CONNECT_ACK:
		os<<"sim_connect_ack";
		break;
		case SIM_DISCONNECT:
		os<<"sim_disconnect";
		break;
		case SIM_DISCONNECT_ACK:
		os<<"sim_disconnect_ack";
		break;
		case SIM_PING:
		os<<"sim_ping";
		break;
		case SIM_PONG:
		os<<"sim_pong";
		break;
		case SIM_SEG:
		os<<"sim_seg";
		break;
		case SIM_SEG_ACK:
		os<<"sim_seg_ack";
		break;
		case SIM_FEEDBACK:
		os<<"sim_feed_back";
		break;
		default:
		os<<"no this message type";
		break;
	}
	os<<"uid"<<m_uid;
}
}
