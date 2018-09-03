#include "webrtclog.h"
#include <iostream>
namespace ns3
{
void LogSinkConsole::OnLogMessage(const std::string &message)
{
	std::cout<<message;
}
bool LogSinkConsole::Init()
{
	rtc::LogMessage::AddLogToStream(this,rtc::LS_INFO);
    	return true;
}
}
