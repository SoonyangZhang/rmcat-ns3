As you could see in wscript under this file, I compile the original webrtc code on the part of congestion control.  
  conf.env.append_value("LINKFLAGS", ['-L/home/zsy/webrtc-ns3/webrtc/system_wrappers','-L/home/zsy/webrtc-ns3/webrtc/rtc_base','-L/home/zsy/webrtc-ns3/webrtc/api','-L/home/zsy/webrtc-ns3/webrtc/logging','-L/home/zsy/webrtc-ns3/webrtc/modules/utility','-L/home/zsy/webrtc-ns3/webrtc/modules/pacing','-L/home/zsy/webrtc-ns3/webrtc/modules/congestion_controller','-L/home/zsy/webrtc-ns3/webrtc/modules/bitrate_controller','-L/home/zsy/webrtc-ns3/webrtc/modules/remote_bitrate_estimator','-L/home/zsy/webrtc-ns3/webrtc/modules/rtp_rtcp'])   
If you intend to compile this code, you must put the webrtc code on the right file. In my VM, the codes on webrtc was put under the file /home/zsy/webrtc-ns3/webrtc. Rememeber to change the path on wscript.  
        
And change the environent variable also.   
1 sudo gedit /etc/profile
2 edit the LD_LIBRARY_PATH as following  
export LD_LIBRARY_PATH=/home/zsy/webrtc-ns3/webrtc/system_wrappers:/home/zsy/webrtc-ns3/webrtc/rtc_base:/home/zsy/webrtc-ns3/webrtc/api:/home/zsy/webrtc-ns3/webrtc/logging:/home/zsy/webrtc-ns3/webrtc/modules/utility:/home/zsy/webrtc-ns3/webrtc/modules/pacing:/home/zsy/webrtc-ns3/webrtc/modules/congestion_controller:/home/zsy/webrtc-ns3/webrtc/modules/bitrate_controller:/home/zsy/webrtc-ns3/webrtc/modules/remote_bitrate_estimator:/home/zsy/webrtc-ns3/webrtc/modules/rtp_rtcp:$LD_LIBRARY_PATH   

WARNNING:when the link capacity above 3Mbps, the webrtcsender tends to overuse the channel.   
But when the link was confifured with 5% packets loss, it can work in normal status. Quite weired and did not figure out why.  
Ways to avoid this problem is to add "random loss rate" at the receiver.  

```
#include "ns3/random-variable-stream.h"  
m_rand_=CreateObject<UniformRandomVariable>();
m_rand_->SetStream(54657594268);
// double fake_loss_rate_{0.05};
// in function  void WebrtcReceiver::RecvPacket
case PT_SEG:
if(m_rand_->GetValue()<fake_loss_rate_){
}else{
//for AWB >2.5Mbps
   m_controller->OnReceivedPacket(now,overhead,fakeHeader);
}
```
 
