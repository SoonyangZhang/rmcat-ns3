#ifndef TRACE_RAZORTRACE_H_
#define TRACE_RAZORTRACE_H_
#include <iostream>
#include <fstream>
#include <string>
namespace ns3{
class RazorTraceReceiver{
public:
	RazorTraceReceiver(){}
	~RazorTraceReceiver(){
		Close();
	}
	void OpenTraceLossFile(std::string name);
	void CloseTraceLossFile();
	void OnLossSeq(uint32_t seq);
private:
	void Close();
	std::fstream m_lossSeq;
};
class RazorTraceSender{
public:
	RazorTraceSender(){}
	~RazorTraceSender(){
		Close();
	}
	void OpenTraceSentFile(std::string name);
	void CloseTraceSentFile();
	void OnSentSeq(uint32_t seq);
private:
	void Close();
	std::fstream m_sendSeq;
};
}
#endif /* TRACE_RAZORTRACE_H_ */
