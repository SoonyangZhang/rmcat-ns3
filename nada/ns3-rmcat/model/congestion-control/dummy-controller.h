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
 * Dummy controller (CBR) interface for rmcat ns3 module.
 *
 * @version 0.1.0
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#ifndef DUMMY_CONTROLLER_H
#define DUMMY_CONTROLLER_H

#include "sender-based-controller.h"

namespace rmcat {

/**
 * Simplistic implementation of a sender-based congestion controller. The
 * algorithm simply returns a constant, hard-coded bandwidth when queried.
 */
class DummyController: public SenderBasedController
{
public:
    /** Class constructor */
    DummyController();

    /** Class destructor */
    virtual ~DummyController();

    /**
     * Set the current bandwidth estimation. This can be useful in test environments
     * to temporarily disrupt the current bandwidth estimation
     *
     * @param [in] newBw Bandwidth estimation to overwrite the current estimation
     */
    virtual void setCurrentBw(float newBw);

    /**
     * Reset the internal state of the congestion controller
     */
    virtual void reset();

    /**
     * Simplistic implementation of feedback packet processing. It simply
     * prints calculated metrics at regular intervals
     */
    virtual bool processFeedback(uint64_t now,
                                 uint32_t sequence,
                                 uint64_t rxTimestamp,
                                 uint8_t ecn=0);
    /**
     * Simplistic implementation of bandwidth getter. It returns a hard-coded
     * bandwidth value in bits per second
     */
    virtual float getBandwidth(uint64_t now) const;

private:
    void updateMetrics(uint64_t now);
    void logStats(uint64_t now) const;

    uint64_t m_lastTimeCalc;
    bool m_lastTimeCalcValid;

    uint64_t m_Qdelay; /**< estimated queuing delay in ms */
    uint32_t m_ploss;  /**< packet loss count within configured window */
    float m_plr;       /**< packet loss ratio within packet history window */
    float m_RecvR;     /**< updated receiving rate in bps */
};

}

#endif /* DUMMY_CONTROLLER_H */
