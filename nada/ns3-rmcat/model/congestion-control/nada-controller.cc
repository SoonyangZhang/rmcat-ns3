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
 * NADA controller implementation for rmcat ns3 module.
 *
 * Reference implementation of the congestion control scheme
 * named Network-Assisted Dynamic Adaptation (NADA), as
 * documented in the following IETF draft (rmcat-nada):
 *
 * NADA: A Unified Congestion Control Scheme for Real-Time Media
 * https://tools.ietf.org/html/draft-ietf-rmcat-nada-05
 *
 * @version 0.1.0
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#include "nada-controller.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <cmath>


/*
 * Default parameter values of the NADA algorithm,
 * corresponding to Figure 3 in the rmcat-nada draft
 */
/* default parameters for core algorithm (gradual rate update) */

const float NADA_PARAM_PRIO = 1.0;  /**< Weight of priority of the flow  */
const float  NADA_PARAM_XREF = 10.0; /**< Reference congestion level (in ms) */

const float NADA_PARAM_KAPPA = 0.5; /**< Scaling parameter for gradual rate update calculation (dimensionless) */
const float NADA_PARAM_ETA = 2.0; /**< Scaling parameter for gradual rate update calculation (dimensionless) */
const float NADA_PARAM_TAU = 500.; /**< Upper bound of RTT (in ms) in gradual rate update calculation */

/**
 * Target interval for receiving feedback from receiver
 * or update rate calculation (in ms)
 */
const uint64_t NADA_PARAM_DELTA = 100;

/* default parameters for accelerated ramp-up */

/**  Threshold for allowed queuing dealy build up at receiver during accelerated ramp-up mode */
const uint64_t NADA_PARAM_QEPS = 10;
const uint64_t NADA_PARAM_DFILT = 120; /**< Bound on filtering delay (in ms) */
/** Upper bound on rate increase ratio in accelerated ramp-up mode (dimensionless) */
const float NADA_PARAM_GAMMA_MAX = 0.5;
/** Upper bound on self-inflicted queuing delay during ramp up (in ms) */
const float NADA_PARAM_QBOUND = 50.;

/* default parameters for non-linear warping of queuing delay */

/** multiplier of observed average loss intervals, as a measure
 * of tolerance of missing observed loss events (dimensionless)
 */
const float NADA_PARAM_MULTILOSS = 7.;

const float NADA_PARAM_QTH = 50.; /**< Queuing delay threshold for invoking non-linear warping (in ms) */
const float NADA_PARAM_LAMBDA = 0.5; /**< Exponent of the non-linear warpming (dimensionless) */

/* default parameters for calculating aggregated congestion signal */

/**
 * Reference delay penalty (in ms) in terms of value
 * of congestion price when packet loss ratio is at PLRREF
 */
const float NADA_PARAM_DLOSS = 10.;
const float NADA_PARAM_PLRREF = 0.01; /**> Reference packet loss ratio (dimensionless) */
const float NADA_PARAM_XMAX = 500.; /**> Maximum value of aggregate congestion signal (in ms) */

/** Smoothing factor in exponential smoothing of packet loss and marking ratios */
const float NADA_PARAM_ALPHA = 0.1;

namespace rmcat {

NadaController::NadaController() :
    SenderBasedController{},
    m_ploss{0},
    m_plr{0.f},
    m_warpMode{false},
    m_lastTimeCalc{0},
    m_lastTimeCalcValid{false},
    m_currBw{m_initBw},
    m_Qdelay{0},
    m_Rtt{0},
    m_Xcurr{0.f},
    m_Xprev{0.f},
    m_RecvR{0.f},
    m_avgInt{0.f},
    m_currInt{0},
    m_lossesSeen{false} {}

NadaController::~NadaController() {}

void NadaController::setCurrentBw(float newBw) {
    m_currBw = newBw;
}

/**
 * Implementation of the #reset API: reset all state variables
 * to default values
 */
void NadaController::reset() {
    m_ploss = 0;
    m_plr = 0.f;
    m_warpMode = false;
    m_lastTimeCalc = 0;
    m_lastTimeCalcValid = false;
    m_currBw = m_initBw;
    m_Qdelay = 0;
    m_Rtt = 0;
    m_Xcurr = 0.f;
    m_Xprev = 0.f;
    m_RecvR = 0.f;
    m_avgInt = 0.f;
    m_currInt = 0;
    m_lossesSeen = false;
    SenderBasedController::reset();
}

/**
 * Implementation of the #processFeedback API
 * in the SenderBasedController class
 *
 * TODO: (deferred) Add support for ECN marking
 */
bool NadaController::processFeedback(uint64_t now,
                                     uint32_t sequence,
                                     uint64_t rxTimestamp,
                                     uint8_t ecn) {
    /* First of all, call the superclass */
    const bool res = SenderBasedController::processFeedback(now,
                                                            sequence,
                                                            rxTimestamp,
                                                            ecn);

    /* Update calculation of reference rate (r_ref)
     * if last calculation occurred more than NADA_PARAM_DELTA
     * (target update interval in ms) ago
     */
    if (!m_lastTimeCalcValid) {
        /* First time receiving a feedback message */
        m_lastTimeCalc = now;
        m_lastTimeCalcValid = true;
        return res;
    }

    assert(lessThan(m_lastTimeCalc, now + 1));
    /* calculate time since last update */
    uint64_t delta = now - m_lastTimeCalc; // subtraction will wrap correctly
    if (delta >= NADA_PARAM_DELTA) {
        /* log & update rate calculation */
        updateMetrics(now);
        updateBw(now, delta);
        //logStats(now);

        m_lastTimeCalc = now;
    }
    return res;
}

/**
 * Implementation of the #getBandwidth API
 * in the SenderBasedController class: simply
 * returns the calculated reference rate
 * (r_ref in rmcat-nada)
 */
float NadaController::getBandwidth(uint64_t now) const {
    return m_currBw;
}


/**
 * The following implements the core congestion
 * control algorithm as specified in the rmcat-nada
 * draft (see Section 4)
 */
void NadaController::updateBw(uint64_t now, uint64_t delta) {

    int rmode = getRampUpMode();
    if (rmode == 0) {
        calcAcceleratedRampUp();
    } else {
        calcGradualRateUpdate(delta);
    }

    /* clip final rate within range */
    m_currBw = std::min(m_currBw, m_maxBw);
    m_currBw = std::max(m_currBw, m_minBw);
}

/**
 * The following function retrieves updated
 * estimates of delay, loss, and receiving
 * rate from the base class SenderBasedController
 * and saves them to local member variables.
 */
void NadaController::updateMetrics(uint64_t now) {

    /* Obtain packet stats in terms of loss and delay */
    uint64_t qdelay = 0;
    bool qdelayOK = getCurrentQdelay(qdelay);
    if (qdelayOK) m_Qdelay = qdelay;

    uint64_t rtt = 0;
    bool rttOK = getCurrentRTT(rtt);
    if (rttOK) m_Rtt = rtt;

    float rrate = 0.f;
    bool rrateOK = getCurrentRecvRate(rrate);
    if (rrateOK) m_RecvR = rrate;

    float plr = 0.f;
    uint32_t nLoss = 0;
    bool plrOK = getPktLossInfo(nLoss, plr);
    if (plrOK) {
        m_ploss = nLoss;
        // Exponential filtering of loss stats
        m_plr += NADA_PARAM_ALPHA * (plr - m_plr);
    }

    float avgInt;
    uint32_t currentInt;
    float avgIntOK = getLossIntervalInfo(avgInt, currentInt);
    m_lossesSeen = avgIntOK;
    if (avgIntOK) {
        m_avgInt = avgInt;
        m_currInt = currentInt;
    }

    /* update aggregate congestion signal */
    m_Xprev = m_Xcurr;
    if (qdelayOK) updateXcurr(now);

}

void NadaController::logStats(uint64_t now) const {

    std::ostringstream os;
    os << std::fixed;
    os.precision(RMCAT_LOG_PRINT_PRECISION); 
    double time=1.0*now/1000;
    /* log packet stats: including common stats
     * (e.g., receiving rate, loss, delay) needed
     * by all controllers and algorithm-specific
     * ones (e.g., xcurr for NADA) */
    /*os << " algo:nada " << m_id
       << " ts: "     << now
       << " loglen: " << m_packetHistory.size()
       << " qdel: "   << m_Qdelay
       << " rtt: "    << m_Rtt
       << " ploss: "  << m_ploss
       << " plr: "    << m_plr
       << " xcurr: "  << m_Xcurr
       << " rrate: "  << m_RecvR
       << " srate: "  << m_currBw
       << " avgint: " << m_avgInt
       << " curint: " << m_currInt;*/
    os<<time<<" srate "<<m_currBw/1000<<" rrate "<<m_RecvR/1000;
    logMessage(os.str());
}

/**
 * Function implementing the calculation of the
 * non-linear warping of queuing delay to d_tilde
 * as specified in rmcat-nada. See Section 4.2, Eq. (1).
 *
 *            / d_queue,           if d_queue<QTH;
 *            |
 * d_tilde = <                                           (1)
 *            |                  (d_queue-QTH)
 *            \ QTH exp(-LAMBDA ---------------), otherwise.
 *                                    QTH
 */
float NadaController::calcDtilde() const {
    const float qDelay = float(m_Qdelay);
    float xval = qDelay;

    if (m_Qdelay > NADA_PARAM_QTH) {
        float ratio = (qDelay - NADA_PARAM_QTH) / NADA_PARAM_QTH;
        ratio = NADA_PARAM_LAMBDA * ratio;
        xval = float(NADA_PARAM_QTH * exp(-ratio));
    }

    return xval;
}

/**
 * Function implementing calculation of the
 * aggregate congestion signal x_curr in
 * rmcat-nada draft. The criteria for
 * invoking the non-linear warping of queuing
 * delay is described in Sec. 4.2 of the draft.
 */
void NadaController::updateXcurr(uint64_t now) {

    float xdel = float(m_Qdelay);           // pure delay-based
    float xtilde = calcDtilde();          // warped version
    const float currInt = float(m_currInt);

    /* Criteria for determining whether to perform
     * non-linear warping of queuing delay. The
     * time window for last observed loss self-adapts
     * with previously observed loss intervals
     * */
    if (m_lossesSeen && currInt < NADA_PARAM_MULTILOSS * m_avgInt) {
        /* last loss observed within the time window
         * NADA_PARAM_MULTILOSS * m_avgInt; allowing us to
         * miss up to NADA_PARAM_MULTILOSS-1 loss events
         */
        m_Xcurr = xtilde;
        m_warpMode = true;
     } else if (m_lossesSeen) {
         /* loss recently expired: slowly transition back
          * to non-warped queuing delay over the course
          * of one average packet loss interval (m_avgInt)
          */
        if (currInt < (NADA_PARAM_MULTILOSS + 1.f) * m_avgInt) {
            /* transition period: linearly blending
             * warped and non-warped values for congestion
             * price */
            const float alpha = (currInt - NADA_PARAM_MULTILOSS * m_avgInt) / m_avgInt;
            m_Xcurr = alpha * xdel + (1.f - alpha) * xtilde;
        } else {
            /* after transition period: switch completely
             * to non-warped queuing delay */
            m_Xcurr = xdel;
            m_warpMode = false;
        }
    } else {
        /* no loss recently observed, stick with
         * non-warped queuing delay */
        m_Xcurr = xdel;
        m_warpMode = false;
    }

    /* Add additional loss penalty for the aggregate
     * congestion signal, following Eq.(2) in Sec.4.2 of
     * rmcat-nada draft */
    float plr0 = m_plr / NADA_PARAM_PLRREF;
    m_Xcurr += NADA_PARAM_DLOSS * plr0 * plr0;

    /* Clip final congestion signal within range */
    if (m_Xcurr > NADA_PARAM_XMAX) {
        m_Xcurr = NADA_PARAM_XMAX;
    }

}

/**
 * This function implements the calculation of reference
 * rate (r_ref) in the gradual update mode, following
 * Eq. (5)-(7) in Section 4.3 of the rmcat-nada draft:
 *
 *
 * x_offset = x_curr - PRIO*XREF*RMAX/r_ref          (5)
 *
 * x_diff   = x_curr - x_prev                        (6)
 *
 *                         delta    x_offset
 * r_ref = r_ref - KAPPA*-------*------------*r_ref
 *                         TAU       TAU
 *
 *                            x_diff
 *               - KAPPA*ETA*---------*r_ref         (7)
 *                              TAU
 */
void NadaController::calcGradualRateUpdate(uint64_t delta) {

    float x_curr = m_Xcurr;
    float x_prev = m_Xprev;

    float x_offset = x_curr;
    float x_diff = x_curr - x_prev;
    float r_offset = m_currBw;
    float r_diff = m_currBw;

    x_offset -= NADA_PARAM_PRIO * NADA_PARAM_XREF * m_maxBw / m_currBw;

    r_offset *= NADA_PARAM_KAPPA;
    r_offset *= float(delta) / NADA_PARAM_TAU;
    r_offset *= x_offset / NADA_PARAM_TAU;

    r_diff *= NADA_PARAM_KAPPA;
    r_diff *= NADA_PARAM_ETA;
    r_diff *= x_diff / NADA_PARAM_TAU;

    m_currBw = m_currBw - r_offset - r_diff;
}

/**
 * This function calculates the reference rate r_ref in the
 * accelarated ramp-up mode, following Eq. (3)-(4) in Section 4.3
 * of the rmcat-nada draft:
 *
 *                            QBOUND
 * gamma = min(GAMMA_MAX, ------------------)     (3)
 *                         rtt+DELTA+DFILT
 *
 * r_ref = max(r_ref, (1+gamma) r_recv)           (4)
 */
void NadaController::calcAcceleratedRampUp( ) {

    float gamma = 1.0;

    uint64_t denom = m_Rtt;
    denom += NADA_PARAM_DELTA;
    denom += NADA_PARAM_DFILT;

    gamma = NADA_PARAM_QBOUND / float(denom);

    if (gamma > NADA_PARAM_GAMMA_MAX) {
        gamma = NADA_PARAM_GAMMA_MAX;
    }

    float rnew = (1.f + gamma) * m_RecvR;
    if (m_currBw < rnew) m_currBw = rnew;
}

/**
 * This function determins whether the congestion control
 * algorithm should operate in the accelerated ramp up mode
 *
 * The criteria for operating in accelerated ramp-up mode are
 * discussed at the end of Section 4.2 of the rmcat-nada draft,
 * as follows:
 *
 * o No recent packet losses within the observation window LOGWIN; and
 *
 * o No build-up of queuing delay: d_fwd-d_base < QEPS for all previous
 *   delay samples within the observation window LOGWIN.
 */
int NadaController::getRampUpMode() {
    int rmode = 0;

    /* If losses are observed, stay with gradual update */
    if (m_ploss > 0) rmode = 1;

    /* check all raw queuing delay samples in
     * packet history log */
    for (auto rit = m_packetHistory.rbegin();
              rit != m_packetHistory.rend() && rmode == 0;
              ++rit) {

        const uint64_t qDelayCurrent = rit->owd - m_baseDelay;
        if (qDelayCurrent > NADA_PARAM_QEPS ) {
            rmode = 1;  /* Gradual update if queuing delay exceeds threshold*/
        }
    }
    return rmode;
}

}
