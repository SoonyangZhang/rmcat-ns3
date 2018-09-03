As you could see in wscript under this file, I compile the original webrtc code on the part of congestion control.  
  conf.env.append_value("LINKFLAGS", ['-L/home/zsy/webrtc-ns3/webrtc/system_wrappers','-L/home/zsy/webrtc-ns3/webrtc/rtc_base','-L/home/zsy/webrtc-ns3/webrtc/api','-L/home/zsy/webrtc-ns3/webrtc/logging','-L/home/zsy/webrtc-ns3/webrtc/modules/utility','-L/home/zsy/webrtc-ns3/webrtc/modules/pacing','-L/home/zsy/webrtc-ns3/webrtc/modules/congestion_controller','-L/home/zsy/webrtc-ns3/webrtc/modules/bitrate_controller','-L/home/zsy/webrtc-ns3/webrtc/modules/remote_bitrate_estimator','-L/home/zsy/webrtc-ns3/webrtc/modules/rtp_rtcp'])   
If you intend to compile this code, you must put the webrtc code on the right file. In my VM, the codes on webrtc was put under the file /home/zsy/webrtc-ns3/webrtc. Rememeber to change the path on wscript.  
        
And change the environent variable also.   
1 sudo gedit /etc/profile
2 edit the LD_LIBRARY_PATH as following  
export LD_LIBRARY_PATH=/home/zsy/webrtc-ns3/webrtc/system_wrappers:/home/zsy/webrtc-ns3/webrtc/rtc_base:/home/zsy/webrtc-ns3/webrtc/api:/home/zsy/webrtc-ns3/webrtc/logging:/home/zsy/webrtc-ns3/webrtc/modules/utility:/home/zsy/webrtc-ns3/webrtc/modules/pacing:/home/zsy/webrtc-ns3/webrtc/modules/congestion_controller:/home/zsy/webrtc-ns3/webrtc/modules/bitrate_controller:/home/zsy/webrtc-ns3/webrtc/modules/remote_bitrate_estimator:/home/zsy/webrtc-ns3/webrtc/modules/rtp_rtcp:$LD_LIBRARY_PATH   
