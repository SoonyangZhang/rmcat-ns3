#include "screamsender.h"
#include "scream-header.h"
#include "ns3/log.h"
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ScreamSender");
ScreamSender::ScreamSender()
{
	m_running=false;
	m_screamTx = new ScreamTx();
	m_rtpQueue = new RtpQueue();
	m_videoEnc=nullptr;
	m_ssrc=1234;
}
ScreamSender::~ScreamSender()
{
	delete m_screamTx;
	delete m_rtpQueue;
	delete m_videoEnc;
}
void ScreamSender::SetTraceFilePath(char*trace)
{
	m_videoEnc=new VideoEnc(m_rtpQueue,m_frameRate,trace);
	m_screamTx->registerNewStream(m_rtpQueue, m_ssrc, 1.0f, 256e3f, 2*1024e3f, 4*8192e3f,2e6f);//, 1e6f, 0.2f, 0.1f, 0.1f);
}
void ScreamSender::InitialSetup(Ipv4Address dest_ip,uint16_t dest_port)
{
	m_peerIp=dest_ip;
	m_peerPort=dest_port;
}
void ScreamSender::StartApplication()
{
	if(m_socket==NULL)
	{
        m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
        auto res = m_socket->Bind ();
        NS_ASSERT (res == 0);
	}
	m_socket->SetRecvCallback (MakeCallback(&ScreamSender::RecvPacket,this));
	m_running=true;
	m_nextFrameTime=Simulator::Now().GetMilliSeconds();
	m_tickEvent=Simulator::ScheduleNow(&ScreamSender::Process,this);
}
void ScreamSender::StopApplication()
{
	m_running=false;
	m_tickEvent.Cancel();
}
void ScreamSender::Send(Ptr<Packet>packet)
{
	if(m_socket==NULL)
	{
		NS_LOG_ERROR("socket  is null,cant send data");
		return ;
	}
	m_socket->SendTo(packet,0,InetSocketAddress{m_peerIp,m_peerPort});
}
void ScreamSender::Process()
{
	int64_t now=Simulator::Now().GetMilliSeconds();
	float time_s=Simulator::Now().GetSeconds();
	uint64_t time_us=now*1000;
	float retVal = -1.0;
	if(m_tickEvent.IsExpired())
	{
		if(now>=m_nextFrameTime)//generate a frame
		{
			m_nextFrameTime=now+m_videoTick;
			//NS_LOG_INFO(now<<" generate a frame");
            m_videoEnc->setTargetBitrate(m_screamTx->getTargetBitrate(m_ssrc));
            int bytes = m_videoEnc->encode(time_s);
            m_screamTx->newMediaFrame(time_us, m_ssrc, bytes);
            retVal = m_screamTx->isOkToTransmit(time_us, m_ssrc);
		}
		if (now>=m_nextCallN && retVal != 0.0f)
		{
			retVal = m_screamTx->isOkToTransmit(time_us, m_ssrc);
			if(retVal>0)
			{
				m_nextCallN=now+retVal;
				NS_LOG_INFO("ret "<<retVal);
			}
		}
        if (retVal == 0) {
            /*
            * RTP packet can be transmitted
            */
            void *rtpPacket = 0;
            int size;
            uint16_t seqNr;
            m_rtpQueue->sendPacket(rtpPacket, size, seqNr);
            retVal = m_screamTx->addTransmitted(time_us, m_ssrc, size, seqNr);
            m_nextCallN =now+retVal;
            rtp_hdr_t rtp;
            rtp.seq_number=seqNr;
            rtp.ssrc=m_ssrc;
            rtp.timestamp=(uint32_t)now;
            ScreamHeader header;
            header.SetMid(ScreamProto::SC_RTP);
        	header.SetHeaderPayload((void*)&rtp,sizeof(rtp));
        	Ptr<Packet> p=Create<Packet>(size);
        	p->AddHeader(header);
        	Send(p);
            NS_LOG_INFO(now<<" send "<<seqNr);
        }
		Time next=MilliSeconds(m_tick);
		Simulator::Schedule(next,&ScreamSender::Process,this);
	}
}
void ScreamSender::RecvPacket(Ptr<Socket>socket)
{
	if(!m_running){return;}
	Address remoteAddr;
	auto p= m_socket->RecvFrom (remoteAddr);
	ScreamHeader header;
	p->RemoveHeader(header);
	rtcp_common_t rtcp;
	header.GetHeaderPayload((void*)&rtcp,sizeof(rtcp));
	uint8_t buffer[128];
	p->CopyData ((uint8_t*)&buffer, p->GetSize ());
	scream_feedback_t *feedback;
	feedback=(scream_feedback_t*)buffer;
	int64_t now=Simulator::Now().GetMilliSeconds();
	uint64_t time_us=now*1000;
        NS_LOG_INFO("incoming feedback"<<feedback->aseq<<" "<<feedback->timestamp);
	uint32_t rxTimestamp=feedback->timestamp;
	m_screamTx->incomingFeedback(time_us,m_ssrc,rxTimestamp,feedback->aseq,
			feedback->ack_vec,feedback->ecn_bytes);
}
}

