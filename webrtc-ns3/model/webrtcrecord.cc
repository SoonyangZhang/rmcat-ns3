#include "webrtcrecord.h"
#include <limits>
namespace ns3{
const uint32_t kUINT32_MAX=std::numeric_limits<uint32_t>::max();
WebrtcSeqRecord::WebrtcSeqRecord(){
    m_lastSeq=kUINT32_MAX;
    m_highSeq=kUINT32_MAX;
}
void WebrtcSeqRecord::OnNewSeq(uint32_t seq){
    if(!IsInitialized()){
        m_highSeq=seq;
        m_recvCount++;
    }else{
        if(seq>m_highSeq){
            m_highSeq=seq;
            m_recvCount++;
        }
    }
}
bool WebrtcSeqRecord::IsInitialized(){
    return m_highSeq!=kUINT32_MAX;
}
void WebrtcSeqRecord::Reset(){
    m_lastSeq=m_highSeq;
    m_recvCount=0;
}
bool WebrtcSeqRecord::ShoudGenerateReport(uint32_t thresh){
    uint32_t total_number=TotalNumber();
    if(total_number>=thresh){
        return true;
    }
    return false;
}
uint32_t WebrtcSeqRecord::TotalNumber(){
    uint32_t total_number=0;
    if(IsInitialized()){
        if(m_lastSeq!=kUINT32_MAX){
            total_number=m_highSeq-m_lastSeq;
        }else{
            total_number=m_highSeq;
        }
    }
    return total_number;
}
uint32_t WebrtcSeqRecord::PacketLost(){
    uint32_t total_number=TotalNumber();
    uint32_t lost=total_number-m_recvCount;
    return lost;
}
uint8_t WebrtcSeqRecord::FractionLoss(){
    uint8_t fraction_loss=0;
    uint32_t total_number=TotalNumber();
    uint32_t lost=PacketLost();
    if(total_number!=0){
        uint32_t temp=lost*255/total_number;
        fraction_loss=(uint8_t)temp;
    }
    return fraction_loss;
}
}
