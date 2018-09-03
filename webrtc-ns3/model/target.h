#ifndef TARGET_H
#define TARGET_H
#include<stdint.h>
namespace ns3
{
class Target
{
public:
	virtual ~Target(){}
	virtual void TargetReceive(uint32_t len){}
};
}
#endif
