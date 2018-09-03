//#define NDEBUG 
#include "rtc_base/logging.h"
#include "test/field_trial.h"
#include "system_wrappers/include/field_trial.h"
#include<stdio.h>
#include <string>
#include<iostream>

class LogSinkConsole:public rtc::LogSink 
{
public:
    void OnLogMessage(const std::string &message) override
	{
	std::cout<<"log concole message"<<' '<<message;
	}
  bool Init()
	{
	rtc::LogMessage::AddLogToStream(this,rtc::LS_INFO);
    	return true;
	}
};
int main()
{
	
	LogSinkConsole log;
	log.Init();
	webrtc::test::ScopedFieldTrials override_field_trials(
      "WebRTC-SendSideBwe-WithOverhead/Enabled/");
	bool ssb=webrtc::field_trial::IsEnabled("WebRTC-SendSideBwe-WithOverhead");
	if(ssb)
	{
		RTC_LOG(LS_INFO)<<"yes enable";
	}else{
		RTC_LOG(LS_INFO)<<"what the fuck";
	}
	return 0;
}
