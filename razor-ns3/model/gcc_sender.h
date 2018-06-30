#ifndef GCC_SENDER_H
#define GCC_SENDER_H
#include "pace_sender.h"
#include "ns3/timer.h"
#include "ns3/data-rate.h"
namespace ns3
{
class GccSender
{
public:
	GccSender();
	~GccSender();
	void ChangePacingRate(uint64_t bitrate);
	void NotifyPacingPerformed (void);
	void Start(void);
	void InsertPacket(int num,uint32_t length);
    static void DataSend(void* handler, uint32_t packet_id, int retrans, size_t size);
private:
	Timer  m_pacingTimer{Timer::REMOVE_ON_DESTROY};
	uint64_t m_pacingTime;// in unit MilliSeconds(m_pacingTime)
	DataRate m_minPacingRate;
	DataRate m_curentPacingRate;
	pace_sender_t *m_paceSender;
	uint64_t m_paceBitRate;
	uint32_t m_sequence;
	
};
}
#endif
