# rmcat-ns3
A comparison of rmcat protocol, namely NADA, GCC and SCReAM on ns3 platform.  
The simulation code of NADA can get from https://github.com/cisco/ns3-rmcat  
For, I just release the SCReAM code modified from https://github.com/EricssonResearch/scream  

the code in screamex.cc sendApp->SetTraceFilePath("/you_path/trace_key.txt"),is no use in experiment, but it should be configured in right path. trace_key.txt is from https://github.com/EricssonResearch/scream/tree/master/traces  

