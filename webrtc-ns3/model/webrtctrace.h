#ifndef MY_TRACE_H
#define MY_TRACE_H
#include <iostream>
#include <fstream>
#include <string>
namespace ns3
{
enum WebrtcTraceEnable:uint8_t{
	E_WEBRTC_RATE=0x01,
	E_WEBRTC_DELAY=0x02,
    E_WEBRTC_FEEDBACK=0x04,
	E_WEBRTC_ALL=E_WEBRTC_RATE|E_WEBRTC_DELAY|E_WEBRTC_FEEDBACK,
};    
class WebrtcTrace
{
public:
	WebrtcTrace(){}
	~WebrtcTrace();
	void Log(std::string name,uint8_t enable);
	void RateTrace(float rate);
	void DelayTrace(uint64_t delay);
    void OnFeedback(uint32_t id,uint32_t seq);
private:
    void Close();
	void OpenTraceRateFile(std::string name);
	void CloseTraceRateFile();
    void OpenTraceDelayFile(std::string name);
    void CloseTraceDelayFile();
    void OpenTraceFeedbackFile(std::string name);
    void CloseTraceFeedbackFile();
	std::fstream m_traceRateFile;//
	std::fstream m_traceDelayFile;
    std::fstream m_traceFeedback;
};
}
#endif
