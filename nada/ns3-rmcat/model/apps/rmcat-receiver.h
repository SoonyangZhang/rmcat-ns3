/******************************************************************************
 * Copyright 2016-2017 Cisco Systems, Inc.                                    *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 *                                                                            *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 ******************************************************************************/

/**
 * @file
 * Receiver application interface for rmcat ns3 module.
 *
 * @version 0.1.0
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#ifndef RMCAT_RECEIVER_H
#define RMCAT_RECEIVER_H

#include "ns3/socket.h"
#include "ns3/application.h"
#include "ns3/callback.h"
namespace ns3 {

class RmcatReceiver: public Application
{
public:
    virtual ~RmcatReceiver ();

    void Setup (uint16_t port);
	typedef Callback<void,uint64_t> DelayCallback;
	void SetDelayCallback(DelayCallback cb){m_delayCb=cb;}
private:
    virtual void StartApplication ();
    virtual void StopApplication ();

    void RecvPacket (Ptr<Socket> socket);
    void SendFeedback (uint32_t sequence,
                       uint64_t recvTimestamp);

private:
    bool m_running;
    bool m_waiting;
    uint32_t m_srcId;
    Ipv4Address m_srcIp;
    uint16_t m_srcPort;
    Ptr<Socket> m_socket;
    DelayCallback m_delayCb;
};

}

#endif /* RMCAT_RECEIVER_H */
