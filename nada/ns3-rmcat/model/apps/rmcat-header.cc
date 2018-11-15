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
 * Common header implementation for rmcat ns3 module.
 *
 * @version 0.1.0
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#include "rmcat-header.h"

namespace ns3 {

FeedbackHeader::~FeedbackHeader () {}

TypeId MediaHeader::GetTypeId (void)
{
    static TypeId tid = TypeId ("MediaHeader")
      .SetParent<Header> ()
      .AddConstructor<MediaHeader> ()
    ;
    return tid;
}

TypeId MediaHeader::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}

uint32_t MediaHeader::GetSerializedSize (void) const
{
    return sizeof (flow_id)    +
           sizeof (sequence)   +
           sizeof (send_tstmp) +
           sizeof (packet_size);
}

void MediaHeader::Serialize (Buffer::Iterator start) const
{
    start.WriteHtonU32 (flow_id);
    start.WriteHtonU32 (sequence);
    start.WriteHtonU64 (send_tstmp);
    start.WriteHtonU32 (packet_size);
}

uint32_t MediaHeader::Deserialize (Buffer::Iterator start)
{
    flow_id = start.ReadNtohU32 ();
    sequence = start.ReadNtohU32 ();
    send_tstmp = start.ReadNtohU64 ();
    packet_size = start.ReadNtohU32 ();
    return GetSerializedSize ();
}

void MediaHeader::Print (std::ostream &os) const
{
    os << "MediaHeader - flow_id = " << flow_id
       << ", sequence = " << sequence
       << ", timestamp = " << send_tstmp;
}

TypeId FeedbackHeader::GetTypeId (void)
{
    static TypeId tid = TypeId ("FeedbackHeader")
      .SetParent<Header> ()
      .AddConstructor<FeedbackHeader> ()
    ;
    return tid;
}

TypeId FeedbackHeader::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}

uint32_t FeedbackHeader::GetSerializedSize (void) const
{
    return sizeof (flow_id)      +
           sizeof (sequence)     +
           sizeof (receive_tstmp);
}

void FeedbackHeader::Serialize (Buffer::Iterator start) const
{
    start.WriteHtonU32 (flow_id);
    start.WriteHtonU32 (sequence);
    start.WriteHtonU64 (receive_tstmp);
}

uint32_t FeedbackHeader::Deserialize (Buffer::Iterator start)
{
    flow_id = start.ReadNtohU32 ();
    sequence = start.ReadNtohU32 ();
    receive_tstmp = start.ReadNtohU64 ();
    return GetSerializedSize ();
}

void FeedbackHeader::Print (std::ostream &os) const
{
    os << "FeedbackHeader - flow_id = " << flow_id
       << ", sequence = " << sequence
       << ",receive_tstmp = " << receive_tstmp;
}

}
