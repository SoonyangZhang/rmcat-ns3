#include "videoenc.h"
#include "rtpqueue.h"
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <math.h>
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/assert.h"
using namespace std;

static const int kMaxRtpSize = 1200;
static const int kRtpOverHead = 12;
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("VideoEnc");
VideoEnc::VideoEnc(RtpQueue* rtpQueue_, float frameRate_, char *fname, int ixOffset_) {
    rtpQueue = rtpQueue_;
    frameRate = frameRate_;
    ix = ixOffset_;
    nFrames = 0;
    seqNr = 0;
    nominalBitrate = 0.0;
    FILE *fp=nullptr;
	fp= fopen(fname,"r");
	NS_ASSERT_MSG(fp,"file open failed");
    char s[100];
    float sum = 0.0;
    while (fgets(s,99,fp)) {
        if (nFrames < MAX_FRAMES - 1) {
            float x = atof(s);
            frameSize[nFrames] = x;
            nFrames++;
            sum += x;
        }
    }
    float t = nFrames / frameRate;
    nominalBitrate = sum * 8 / t;
    fclose(fp);
}

void VideoEnc::setTargetBitrate(float targetBitrate_) {
	double now=Simulator::Now().GetSeconds();
	if(targetBitrate_>1e-10)
	{
	NS_LOG_INFO("enc "<<now<<" "<<targetBitrate_/1000);
	if(!m_rateCb.IsNull())
	{
	double kbps=targetBitrate_/1000;
	m_rateCb(kbps);
	}
    targetBitrate = targetBitrate_;}
}

int VideoEnc::encode(float time) {
    int rtpBytes = 0;
    //int bytes = (int) (frameSize[ix]/nominalBitrate*targetBitrate);
    //nominalBitrate = 0.95*nominalBitrate + 0.05*frameSize[ix] * frameRate * 8;
   // ix++; if (ix == nFrames) ix = 0;
    int bytes=ceil(0.001*m_fs*targetBitrate/8);
    //NS_LOG_INFO("frame size "<<bytes);
    while (bytes > 0) {
        int rtpSize = std::min(kMaxRtpSize, bytes);
        bytes -= rtpSize;
        rtpSize += kRtpOverHead;
        rtpBytes += rtpSize;
        rtpQueue->push(0, rtpSize, seqNr, time);
        seqNr++;
    }
    rtpQueue->setSizeOfLastFrame(rtpBytes);
    return rtpBytes;
}	
}


