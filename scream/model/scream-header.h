#ifndef SCREAM_HEADER_H
#define SCREAM_HEADER_H
#include "ns3/header.h"
#include "ns3/type-id.h"
namespace ns3
{
//version 0 mean rtp ,1 mean rtcp
typedef struct
{
#if 0
	uint16_t version:2;
	uint16_t padbit:1;
	uint16_t extbit:1;
	uint16_t cc:4;
	uint16_t markbit:1;
	uint16_t paytype:7;
#else
	uint16_t cc:4;
	uint16_t extbit:1;
	uint16_t padbit:1;
	uint16_t version:2;
	uint16_t paytype:7;
	uint16_t markbit:1;
#endif
	uint16_t seq_number;
	uint32_t timestamp;
	uint32_t ssrc;
} rtp_hdr_t;
/*
 * RTCP common header word
 */
typedef struct {
#if 0   //BIG_ENDIA
	uint16_t version:2;
	uint16_t padbit:1;
	uint16_t rc:5;
	uint16_t packet_type:8;
#else
	uint16_t rc:5;
	uint16_t padbit:1;
	uint16_t version:2;
	uint16_t packet_type:8;
#endif
    uint16_t length;           /* pkt len in words, w/o this word */

} rtcp_common_t;
typedef struct
{
	uint32_t ssrc;
	uint32_t aseq:16;
	uint32_t ecn_bytes:16;
	uint64_t ack_vec;
	uint32_t timestamp;
}scream_feedback_t;
#define SCREAM_HDR_LEN 128
enum ScreamProto
{
	SC_RTP,
	SC_RTCP,  //for know,just this two
};
class ScreamHeader:public ns3::Header
{
public:
	ScreamHeader();
	~ScreamHeader();
	void SetMid(uint32_t mid){m_mid=mid;}
	uint8_t GetMid(){return m_mid;}
	uint32_t SetHeaderPayload(void *buf,uint32_t buflen);
	uint32_t GetHeaderPayload(void *buf,uint32_t buflen);
    static ns3::TypeId GetTypeId ();
    virtual ns3::TypeId GetInstanceTypeId (void) const;
	virtual uint32_t GetSerializedSize (void) const;
    virtual void Serialize (ns3::Buffer::Iterator start) const;
    virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
    virtual void Print (std::ostream &os) const;
private:
	uint8_t m_mid;//the first serialized byte
	uint8_t m_hdr[SCREAM_HDR_LEN];//hdr
	uint32_t m_len;
	
};
}
#endif
