#include "processmodule.h"
#include "ns3/log.h"
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ProcessModule");
ProcessModule::ProcessModule(){}
ProcessModule::~ProcessModule(){}
void ProcessModule::Start()
{
	m_timer=Simulator::ScheduleNow(&ProcessModule::Process,this);
}
void ProcessModule::Stop()
{
	m_timer.Cancel();
}
void ProcessModule::Process()
{
	int64_t now = rtc::TimeMillis();
	NS_LOG_FUNCTION(now<<" "<<Simulator::Now().GetMilliSeconds());
	//int64_t next_checkpoint = now + (1000 * 60);
	for(ModuleCallback& m: m_modules)
	{
	      if (m.next_callback == 0)
        	m.next_callback = GetNextCallbackTime(m.module, now);
	      if (m.next_callback <= now ||
          	m.next_callback ==-1) {
		m.module->Process();	
				}

        int64_t new_now = rtc::TimeMillis();
        m.next_callback = GetNextCallbackTime(m.module, new_now);
	NS_LOG_INFO(m.location.ToString());
	//if (m.next_callback < next_checkpoint)
        //next_checkpoint = m.next_callback;
	}
	Time next=MilliSeconds(m_next_checkpoint);
	m_timer=Simulator::Schedule(next,&ProcessModule::Process,this);
}
int64_t ProcessModule::GetNextCallbackTime(webrtc::Module* module, int64_t time_now)
{
  int64_t interval = module->TimeUntilNextProcess();
  NS_LOG_INFO("interval "<<interval);
  if (interval < 0) {
    // Falling behind, we should call the callback now.
    return time_now;
  }
  return time_now + interval;
}
void ProcessModule::RegisterModule(webrtc::Module* module, const rtc::Location& from)
{
	for(const ModuleCallback& mc : m_modules)
	{
		if(mc.module==module)
		{
		NS_LOG_FUNCTION("already registed");
		return ;	
		}

	}
	m_modules.push_back(ModuleCallback(module,from));
}
void ProcessModule::DeRegisterModule(webrtc::Module* module)
{
	m_modules.remove_if([&module](const ModuleCallback& m)
	{
		return m.module==module;
	});
}

}
