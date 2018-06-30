# rmcat-ns3
A comparison of rmcat protocol, namely NADA, GCC and SCReAM on ns3 platform. The version of ns is 3.26.  
The simulation code of NADA can get from https://github.com/cisco/ns3-rmcat  
For, I just release the SCReAM code modified from https://github.com/EricssonResearch/scream  

the code in screamex.cc sendApp->SetTraceFilePath("/you_path/trace_key.txt"),is no use in experiment, but it should be configured in right path. trace_key.txt is from https://github.com/EricssonResearch/scream/tree/master/traces  

the mytrace module is used for data collecion. And a file named "trace" should be created first under the  path of ns-allinone-3.26/ns-3.26/  

the razor project is a c version of GCC release at https://github.com/yuanrongxi/razor. And I make some minor modification to get it running on ns3.  

In simulation(razor-example/gcc_tcp.cc), a point to point channel was created. From the time of 20s to 100s, A reno TCP flowing into the network. The first picture shows the kalman-gcc cannot complete fainness bandiwdth resource with tcp flow.
![image](https://github.com/sonyangchang/rmcat-ns3/blob/master/razor-example/gcc_razor_0remb.png)
![image](https://github.com/sonyangchang/rmcat-ns3/blob/master/razor-example/gcc_razor_1remb.png)  

Its the success importation of razor project on ns3 encourages me get the original GCC in webrtc running on ns3.

