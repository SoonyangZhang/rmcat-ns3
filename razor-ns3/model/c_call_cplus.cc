#include "ns3/core-module.h"
#include "c_call_cplus.h"
using namespace ns3;
int64_t get_clock_time()
{
	return Simulator::Now ().GetMilliSeconds();
}
