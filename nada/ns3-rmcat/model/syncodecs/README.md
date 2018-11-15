# syncodecs
Synthetic codecs for evaluation of RMCAT work

Syncodecs are a set of C++ classes implementing adaptive synthetic video codecs. "Synthetic" means that they do not encode real video; they provide a sequence of packets/frames whose size/frequency adapt to the configured target sending rate. This configured target sending rate can be changed over time.  The synthetic codecs can be used both in simulation environments (ns2, ns3), and in physical testbeds running real endpoints.

For further information, please look at the extensive Doxygen documentation and examples in the ".h" files.
