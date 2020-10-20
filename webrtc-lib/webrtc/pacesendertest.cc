#include"modules/pacing/paced_sender.h"
#include "system_wrappers/include/clock.h"
#include "system_wrappers/include/field_trial.h"
#include <stdio.h>
#include<memory.h>
#include<string.h>
using namespace webrtc;
static const int kTargetBitrateBps = 800000;
constexpr unsigned kFirstClusterBps = 900000;
constexpr unsigned kSecondClusterBps = 1800000;

// The error stems from truncating the time interval of probe packets to integer
// values. This results in probing slightly higher than the target bitrate.
// For 1.8 Mbps, this comes to be about 120 kbps with 1200 probe packets.
constexpr int kBitrateProbingError = 150000;

const float kPaceMultiplier = 2.5f;

class MockPacedSenderCallback : public PacedSender::PacketSender 
{
public:
MockPacedSenderCallback(SimulatedClock *clock)
{
	clock_=clock;
}
bool TimeToSendPacket(uint32_t ssrc,
                                  uint16_t sequence_number,
                                  int64_t capture_time_ms,
                                  bool retransmission,
                                  const PacedPacketInfo& cluster_info)
{
	printf("send seq %u,%ld\n",sequence_number,clock_->TimeInMilliseconds());
}
size_t TimeToSendPadding(size_t bytes,
                                     const PacedPacketInfo& cluster_info)
									 {
											printf("sending padding size%u\n",bytes);
								 }
private:
SimulatedClock *clock_;	
};
class PaceSenderTest
{
public:
  PaceSenderTest() :clock_(123456),callback_(&clock_){
	
    srand(0);
    // Need to initialize PacedSender after we initialize clock.
    send_bucket_.reset(new PacedSender(&clock_, &callback_, nullptr));
    send_bucket_->CreateProbeCluster(kFirstClusterBps);
    send_bucket_->CreateProbeCluster(kSecondClusterBps);
    // Default to bitrate probing disabled for testing purposes. Probing tests
    // have to enable probing, either by creating a new PacedSender instance or
    // by calling SetProbingEnabled(true).
    send_bucket_->SetProbingEnabled(false);
    send_bucket_->SetEstimatedBitrate(kTargetBitrateBps);

    clock_.AdvanceTimeMilliseconds(send_bucket_->TimeUntilNextProcess());
  }
  void SendAndExpectPacket(PacedSender::Priority priority,
                           uint32_t ssrc,
                           uint16_t sequence_number,
                           int64_t capture_time_ms,
                           size_t size,
                           bool retransmission) {
    send_bucket_->InsertPacket(priority, ssrc, sequence_number, capture_time_ms,
                               size, retransmission);
						   }
	void Run()
	{
  uint16_t sequence_number = 1234;
  const uint32_t kSsrc = 12345;
  const size_t kSizeBytes = 250;
  const size_t kPacketToSend = 10;
  const int64_t kStartMs = clock_.TimeInMilliseconds();
  for (size_t i = 0; i < kPacketToSend; ++i) {
    SendAndExpectPacket(PacedSender::kNormalPriority, kSsrc, sequence_number++,
                        clock_.TimeInMilliseconds(), kSizeBytes, false);
	printf("time%ld\n",clock_.TimeInMilliseconds());
    send_bucket_->Process();
    clock_.AdvanceTimeMilliseconds(send_bucket_->TimeUntilNextProcess());
	}  
	}
private:
SimulatedClock clock_;
  MockPacedSenderCallback callback_;
  std::unique_ptr<PacedSender> send_bucket_;	
};

int main()
{
	PaceSenderTest test;
	test.Run();
	return 0;
}
