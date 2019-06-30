#include <unistd.h>
#include <memory.h>
#include <string>
#include<stdio.h>
#include "webrtctrace.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
using namespace std;
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("webrtc_trace");
WebrtcTrace::~WebrtcTrace()
{
    Close();
}
void WebrtcTrace::Log(std::string name,uint8_t enable){
	if(enable&E_WEBRTC_RATE){
		OpenTraceRateFile(name);
	}
	if(enable&E_WEBRTC_DELAY){
		OpenTraceDelayFile(name);
	}
	if(enable&E_WEBRTC_FEEDBACK){
		OpenTraceFeedbackFile(name);
	}
}
void WebrtcTrace::RateTrace(float rate){
    if(m_traceRateFile.is_open()){
        char line [255];
        memset(line,0,255);
        double now=Simulator::Now().GetSeconds();
        sprintf (line, "%16f %16f",
                now,rate);
        m_traceRateFile<<line<<std::endl;        
    }    
}
void WebrtcTrace::DelayTrace(uint64_t delay){
    if(m_traceDelayFile.is_open()){
        char line [255];
        memset(line,0,255);
        double now=Simulator::Now().GetSeconds();
        //Time temp=MilliSenconds(delay);
        sprintf (line, "%16f %16u",
                now,delay);
        m_traceDelayFile<<line<<std::endl;        
    }    
}
void WebrtcTrace::OnFeedback(uint32_t id,uint32_t seq){
    if(m_traceFeedback.is_open()){
        char line [255];
        memset(line,0,255);
        //Time temp=MilliSenconds(delay);
        sprintf (line, "%u %16u",
                id,seq);
        m_traceFeedback<<line<<std::endl;          
    }
}
void WebrtcTrace::Close(){
    CloseTraceRateFile();
    CloseTraceDelayFile();
    CloseTraceFeedbackFile();
}
void WebrtcTrace::CloseTraceRateFile(){
	if(m_traceRateFile.is_open()){
        m_traceRateFile.flush();
		m_traceRateFile.close();         
    }
   
}
void WebrtcTrace::CloseTraceDelayFile(){
	if(m_traceDelayFile.is_open()){
        m_traceDelayFile.flush();
		m_traceDelayFile.close();         
    } 
}
void WebrtcTrace::CloseTraceFeedbackFile(){
    if(m_traceFeedback.is_open()){
        m_traceFeedback.flush();
        m_traceFeedback.close();
    }
}
void WebrtcTrace::OpenTraceRateFile(std::string name){
	char buf[FILENAME_MAX];
	memset(buf,0,FILENAME_MAX);
	std::string path = std::string (getcwd(buf, FILENAME_MAX)) + "/traces/"
			+name+"_rate.txt";
	m_traceRateFile.open(path.c_str(), std::fstream::out);    
}
void WebrtcTrace::OpenTraceDelayFile(std::string name){
	char buf[FILENAME_MAX];
	memset(buf,0,FILENAME_MAX);
	std::string path = std::string (getcwd(buf, FILENAME_MAX)) + "/traces/"
			+name+"_delay.txt";
	m_traceDelayFile.open(path.c_str(), std::fstream::out);     
}
void WebrtcTrace::OpenTraceFeedbackFile(std::string name){
	char buf[FILENAME_MAX];
	memset(buf,0,FILENAME_MAX);
	std::string path = std::string (getcwd(buf, FILENAME_MAX)) + "/traces/"
			+name+"_feedback.txt";
	m_traceFeedback.open(path.c_str(), std::fstream::out);    
}
}
