#include <unistd.h>
#include <memory.h>
#include <string>
#include<stdio.h>
#include "ns3/mytrace.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
using namespace std;
namespace ns3
{
NS_LOG_COMPONENT_DEFINE ("My-Trace");
MyTracer::MyTracer(){}
MyTracer::~MyTracer()
{
	if(m_traceRateFile.is_open())
		m_traceRateFile.close();
	if(m_traceDelayFile.is_open())
		m_traceDelayFile.close();
}
void MyTracer::OpenTraceFile(std::string filename)
{
	char buf[FILENAME_MAX];
	memset(buf,0,FILENAME_MAX);
	string delaypath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" + filename+"_delay.txt";
	memset(buf,0,FILENAME_MAX);
	string ratepath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" + filename+"_rate.txt";
	m_traceRateFile.open(ratepath.c_str(), fstream::out);
	m_traceDelayFile.open(delaypath.c_str(), fstream::out);
}
void MyTracer::CloseTraceFile()
{
	if(m_traceRateFile.is_open())
		m_traceRateFile.close();
	if(m_traceDelayFile.is_open())
		m_traceDelayFile.close();	
}
void MyTracer::RateTrace(float rate)
{
	char line [255];
	memset(line,0,255);
	double now=Simulator::Now().GetSeconds();
	sprintf (line, "%16f %16f",
			now,rate);
	m_traceRateFile<<line<<std::endl;		
}
void MyTracer::DelayTrace(uint64_t delay)
{
	char line [255];
	memset(line,0,255);
	double now=Simulator::Now().GetSeconds();
	//Time temp=MilliSenconds(delay);
	sprintf (line, "%16f %16u",
			now,delay);
	m_traceDelayFile<<line<<std::endl;
}
}
