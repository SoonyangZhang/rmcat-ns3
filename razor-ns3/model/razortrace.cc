#include <unistd.h>
#include <memory.h>
#include "razortrace.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
namespace ns3{
void RazorTraceReceiver::OpenTraceLossFile(std::string name){
	char buf[FILENAME_MAX];
	memset(buf,0,FILENAME_MAX);
	std::string path = std::string (getcwd(buf, FILENAME_MAX)) + "/traces/"
			+name+"_loss.txt";
	m_lossSeq.open(path.c_str(), std::fstream::out);
}
void RazorTraceReceiver::CloseTraceLossFile(){
	if(m_lossSeq.is_open()){
		m_lossSeq.close();
	}
}
void RazorTraceReceiver::OnLossSeq(uint32_t seq){
	if(m_lossSeq.is_open()){
		char line [256];
		memset(line,0,256);
		//double now=Simulator::Now().GetSeconds();
		sprintf (line, "%d",seq);
		m_lossSeq<<line<<std::endl;
	}
}
void RazorTraceReceiver::Close(){
	CloseTraceLossFile();
}
void RazorTraceSender::OpenTraceSentFile(std::string name){
	char buf[FILENAME_MAX];
	memset(buf,0,FILENAME_MAX);
	std::string path = std::string (getcwd(buf, FILENAME_MAX)) + "/traces/"
			+name+"_sent.txt";
	m_sendSeq.open(path.c_str(), std::fstream::out);
}
void RazorTraceSender::CloseTraceSentFile(){
	if(m_sendSeq.is_open()){
		m_sendSeq.close();
	}
}
void RazorTraceSender::OnSentSeq(uint32_t seq){
	if(m_sendSeq.is_open()){
		char line [256];
		memset(line,0,256);
		//double now=Simulator::Now().GetSeconds();
		sprintf (line, "%d",seq);
		m_sendSeq<<line<<std::endl;
	}
}
void RazorTraceSender::Close(){
	CloseTraceSentFile();
}
}




