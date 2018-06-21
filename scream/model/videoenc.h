#ifndef VIDEO_ENC
#define VIDEO_ENC
#include "ns3/callback.h"
namespace ns3
{
class RtpQueue;
#define MAX_FRAMES 10000
class VideoEnc {
public:
    VideoEnc(RtpQueue* rtpQueue, float frameRate, char *fname, int ixOffset=0);

    int encode(float time);

    void setTargetBitrate(float targetBitrate);
	typedef Callback<void,float> RateCallback;
    void SetRateCallback(RateCallback cb){m_rateCb=cb;}
    RtpQueue* rtpQueue;
    float frameSize[MAX_FRAMES];
    int nFrames;
    float targetBitrate;
    float frameRate;
    float nominalBitrate;
    unsigned int seqNr;
    int ix;
    float m_fs{40};
    RateCallback m_rateCb;
};	
}
#endif
