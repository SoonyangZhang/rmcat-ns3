#include "scream-header.h"
#include "ns3/log.h"
#include <memory.h>
namespace ns3
{
NS_LOG_COMPONENT_DEFINE ("ScreamHeader");
NS_OBJECT_ENSURE_REGISTERED (ScreamHeader);
ScreamHeader::ScreamHeader()
{
	memset(m_hdr,0,SCREAM_HDR_LEN);
}
ScreamHeader::~ScreamHeader(){}
uint32_t ScreamHeader::SetHeaderPayload(void *buf,uint32_t buflen)
{
	uint32_t min=SCREAM_HDR_LEN>buflen?buflen:SCREAM_HDR_LEN;
	memcpy(m_hdr,buf,min);
	return min;
}
uint32_t ScreamHeader::GetHeaderPayload(void *buf,uint32_t buflen)
{
	uint32_t min=SCREAM_HDR_LEN>buflen?buflen:SCREAM_HDR_LEN;
	memcpy(buf,m_hdr,min);
	return min;
}
TypeId ScreamHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("ScreamHeader")
      .SetParent<Header> ()
      .AddConstructor<ScreamHeader> ()
    ;
    return tid;
}
TypeId ScreamHeader::GetInstanceTypeId (void) const
{
	return GetTypeId();
}
uint32_t ScreamHeader::GetSerializedSize (void) const
{
	uint32_t hdrlen=0;
	switch(m_mid)
	{
	case ScreamProto::SC_RTP:
	{
		hdrlen=sizeof(rtp_hdr_t);
	}
	break;
	case ScreamProto::SC_RTCP:
	{
		hdrlen=sizeof(rtcp_common_t);
	}
	break;
	default:
		NS_LOG_ERROR("unrecognized message type");
		break;
	}
	return hdrlen+sizeof(m_mid);
}
void ScreamHeader::Serialize (Buffer::Iterator start) const
{
	NS_LOG_FUNCTION (this << &start);
	Buffer::Iterator i = start;
	i.WriteU8(m_mid);
	switch(m_mid)
	{
	case ScreamProto::SC_RTP:
	{
		uint16_t a,*ptr=nullptr;
		rtp_hdr_t*header=(rtp_hdr_t*)m_hdr;
		ptr=(uint16_t*)header;
		a=*ptr;
		i.WriteHtonU16(a);
		i.WriteHtonU16(header->seq_number);
		i.WriteHtonU32(header->timestamp);
		i.WriteHtonU32(header->ssrc);
	}
	break;
	case ScreamProto::SC_RTCP:
	{
		uint16_t a,*ptr=nullptr;
		rtcp_common_t *hdr=(rtcp_common_t*)m_hdr;
		ptr=(uint16_t*)hdr;
		a=*ptr;
		i.WriteHtonU16(a);
		i.WriteHtonU16(hdr->length);
	}
	break;
	default:
		break;
	}
}
uint32_t ScreamHeader::Deserialize (ns3::Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	m_mid=i.ReadU8();
	switch(m_mid)
	{
	case ScreamProto::SC_RTP:
	{
		uint16_t a,*ptr=nullptr;
		rtp_hdr_t*header=(rtp_hdr_t*)m_hdr;
		ptr=(uint16_t*)header;
		a=i.ReadNtohU16();
		*ptr=a;
		header->seq_number=i.ReadNtohU16();
		header->timestamp=i.ReadNtohU32();
		header->ssrc=i.ReadNtohU32();
	}
	break;
	case ScreamProto::SC_RTCP:
	{
		uint16_t a,*ptr=nullptr;
		rtcp_common_t *hdr=(rtcp_common_t*)m_hdr;
		ptr=(uint16_t*)hdr;
		a=i.ReadNtohU16();
		*ptr=a;
		hdr->length=i.ReadNtohU16();
	}
	break;
	}
	return GetSerializedSize();
}
void  ScreamHeader::Print (std::ostream &os) const
{
	switch(m_mid)
	{
	case ScreamProto::SC_RTP:
	{
		os<<"sc_rtp";
	}
	break;
	case ScreamProto::SC_RTCP:
	{
		os<<"sc_rtcp";
	}
	break;
	}
}
}
