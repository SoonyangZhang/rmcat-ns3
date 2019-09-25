#ifndef RAZOR_HEADER_H
#define RAZOR_HEADER_H
#include"ns3/type-id.h"
#include "ns3/header.h"
namespace ns3
{
enum RazorProtoType
{
	MIN_MSG_ID	=	0,

	SIM_CONNECT,			/*连接接收端请求*/
	SIM_CONNECT_ACK,		/*响应*/

	SIM_DISCONNECT,
	SIM_DISCONNECT_ACK,

	SIM_PING,
	SIM_PONG,

	SIM_SEG,
	SIM_SEG_ACK,
	SIM_FEEDBACK,

	MAX_MSG_ID	
};
#define protocol_ver			0x01

typedef struct
{
	uint8_t		ver;			/*协议版本，默认为0x01*/
	uint8_t		mid;			/*消息ID*/
	uint32_t	uid;			/*消息发起方ID*/
}sim_header_t;

#define INIT_SIM_HEADER(h, msg_id, userid) \
	h.ver = protocol_ver;	\
	h.mid = (msg_id);		\
	h.uid = (userid)

#define SIM_HEADER_SIZE			6
#define SIM_VIDEO_SIZE			1000
#define SIM_TOKEN_SIZE			128
#define NACK_NUM				80
#define SIM_FEEDBACK_SIZE		1000
typedef struct
{
	uint32_t cid;						/*协商的呼叫ID，每次是一个随机值,和disconnect等消息保持一致*/
	uint16_t token_size;				/*token是一个验证信息，可以用类似证书的方式来进行验证*/
	uint8_t	 token[SIM_TOKEN_SIZE];
}sim_connect_t;

typedef struct
{
	uint32_t cid;
	uint32_t result;
}sim_connect_ack_t;

typedef struct
{
	uint32_t cid;
}sim_disconnect_t;

typedef sim_connect_ack_t sim_disconnect_ack_t;

typedef struct
{
	uint32_t	packet_id;				/*包序号*/
	uint32_t	fid;					/*帧序号*/
	uint32_t	timestamp;				/*帧时间戳*/
	uint16_t	index;					/*帧分包序号*/
	uint16_t	total;					/*帧分包总数*/
	uint8_t		ftype;					/*视频帧类型*/

	uint8_t		remb;					//0X80 means its bandwidth probe
	uint16_t	send_ts;				/*发送时刻相对帧产生时刻的时间戳*/
	uint16_t	transport_seq;			/*传输通道序号，这个是传输通道每发送一个报文，它就自增长1，而且重发报文也会增长*/
	
	uint16_t	data_size;

}sim_segment_t;
//uint8_t		data[SIM_VIDEO_SIZE]; this video payload  is moved to Packet part
#define SIM_SEGMENT_HEADER_SIZE (SIM_HEADER_SIZE + 24)

typedef struct
{
	uint32_t	base_packet_id;			/*被接收端确认连续最大的包ID*/
	uint32_t	acked_packet_id;		/*立即确认的报文序号id,用于计算rtt*/
	/*重传的序列*/
	uint8_t		nack_num;
	uint16_t	nack[NACK_NUM];
}sim_segment_ack_t;

typedef struct
{
	int64_t	ts;							/*心跳时刻*/
}sim_ping_t;

typedef sim_ping_t sim_pong_t;

typedef struct
{
	uint32_t	base_packet_id;			/*被接收端确认连续最大的包ID*/

	uint16_t	feedback_size;
	uint8_t		feedback[SIM_FEEDBACK_SIZE];
}sim_feedback_t;
#define MAX_MSGLEN 1400
class RazorHeader:public ns3::Header
{
public:
	RazorHeader();
	~RazorHeader();
    static ns3::TypeId GetTypeId ();
    virtual ns3::TypeId GetInstanceTypeId (void) const;
    virtual uint32_t GetSerializedSize (void) const;
	void SetMid(uint32_t mid){m_mid=mid;}	
	void SetUid(uint32_t uid){m_uid=uid;}
    void SetHeader(sim_header_t &header);
    void GetHeader(sim_header_t &header);
	uint32_t SetMessagePayload(void *buf,uint32_t buflen);
	uint8_t GetMid(){return m_mid;}
	uint32_t GetMessage(void *buf,uint32_t buflen);
    static uint32_t GetMessageLen(uint8_t mid,void *message);
    virtual void Serialize (ns3::Buffer::Iterator start) const;
    virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
    virtual void Print (std::ostream &os) const;
private:
	uint8_t m_ver;
	uint8_t m_mid;
	uint32_t m_uid;
	uint8_t m_message[MAX_MSGLEN];//message payload
	uint32_t m_len;//message legth
};
//when packet received
//RazorHeader razorHeader;
//packet->RemoveHeader (RazorHeader);
}
	


#endif
