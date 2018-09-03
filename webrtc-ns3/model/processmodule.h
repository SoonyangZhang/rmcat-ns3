#ifndef PROCESS_MODULE_H
#define PROCESS_MODULE_H
#define WEBRTC_POSIX
#define WEBRTC_LINUX
#include "webrtcconstants.h"
#include "modules/include/module.h"
#include "rtc_base/location.h"
#include "modules/pacing/packet_router.h"
#include<list>
#include "ns3/event-id.h"
#include "ns3/simulator.h"
namespace ns3
{
class ProcessModule
{
public:
	ProcessModule();
	~ProcessModule();
	void Start();
	void Stop();
	uint32_t GetSize(){return m_modules.size();}
	void RegisterModule(webrtc::Module* module, const rtc::Location& from);
	void DeRegisterModule(webrtc::Module* module);
private:
  struct ModuleCallback {
    ModuleCallback() = delete;
    ModuleCallback(ModuleCallback&& cb) = default;
    ModuleCallback(const ModuleCallback& cb) = default;
    ModuleCallback(webrtc::Module* module, const rtc::Location& location)
        : module(module), location(location) {}
    bool operator==(const ModuleCallback& cb) const {
      return cb.module == module;
    }

    webrtc::Module* const module;
    int64_t next_callback = 0;  // Absolute timestamp.
    const rtc::Location location;

   private:
    ModuleCallback& operator=(ModuleCallback&);
  };
private:
	void Process();
int64_t GetNextCallbackTime(webrtc::Module* module, int64_t time_now);
typedef std::list<ModuleCallback> ModuleList;
	EventId m_timer;
	int64_t m_next_checkpoint{5};//60 milliseconds;
	ModuleList m_modules;

};
	
}
#endif
