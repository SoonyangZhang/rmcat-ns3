#pragma once
#include <stdint.h>
namespace ns3{
class WebrtcSeqRecord{
public:
    WebrtcSeqRecord();
    ~WebrtcSeqRecord(){}
    bool IsInitialized();
    bool ShoudGenerateReport(uint32_t thresh);
    void Reset();
    uint32_t TotalNumber();
    uint32_t PacketLost();
    uint8_t FractionLoss();
    void OnNewSeq(uint32_t seq);
    uint32_t m_lastSeq;
    uint32_t m_highSeq;
    int64_t m_recvCount{0};
};
}
