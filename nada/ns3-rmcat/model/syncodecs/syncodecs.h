/******************************************************************************
 * Copyright 2014-2017 cisco Systems, Inc.                                    *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
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
 * Syncodecs header file.
 *
 * Syncodecs is a family of synthetic codecs typically used to generate synthetic
 * video traffic. They are useful in the context of evaluation real-time media congestion
 * controllers (see <a href="https://datatracker.ietf.org/wg/rmcat/">RMCAT IETF group</a>).
 *
 * This header file is the API to interact with the syncodecs family of codecs. The different
 * codecs are declared as classes with inheritance relationships. Class #syncodecs::Codec is the
 * superclass of all codecs.
 *
 * Synthetic codecs range from the simplest (perfect codec #syncodecs::PerfectCodec) to more
 * sophisticated ones (e.g., trace-based codec #syncodecs::TraceBasedCodec). The structure of
 * the syncodecs' API is extensible: other codecs with extra functionality can be added in future
 * versions by subclassing the existing ones.
 *
 * See the documentation of each of the classes in this file for further information on the
 * particular features of the different synthetic codecs.
 *
 * @version 0.1.0
 * @author Sergio Mena de la Cruz (semena@cisco.com)
 * @author Stefano D'Aronco (stefano.daronco@epfl.ch)
 * @author Xiaoqing Zhu (xiaoqzhu@cisco.com)
 * @copyright Apache v2 License
 */

#ifndef SYNCODECS_H_
#define SYNCODECS_H_

#include "traces-reader.h"
#include <iterator>
#include <map>
#include <vector>
#include <utility>
#include <cstdint>
#include <memory>
#include "ns3/callback.h"
/**
 * @defgroup TraceBasedCodecConst These are defined as constants for the moment. Later on, they
 * could be considered as parameters of #syncodecs::TraceBasedCodec and subclasses.
 */
/*@{*/

#define TRACE_MIN_BITRATE 100 /**< Minimum bitrate when scanning a trace file directory (kbps). */
#define TRACE_MAX_BITRATE 6000 /**< Maximum bitrate when scanning a trace file directory (kbps). */
#define TRACE_BITRATE_STEP 100 /**< Step used when scanning a trace file directory (kbps). */
#define N_FRAMES_EXCLUDED 20 /**< Number of initial frames to exclude when trace wraps around. */

/*@}*/
namespace ns3
{
namespace syncodecs {

typedef std::pair<std::vector<uint8_t>, /* packet/frame contents */
                  double                /* time (secs) to next packet/frame */
                  > PacketOrFrameRecord;

/**
 * This abstract class is the super class of all synthetic codecs. Congestion control algorithms
 * can use polymorphic pointers/references to this class to operate with any synthetic codec.
 *
 * Synthetic codecs are implemented as STL-like iterators. This is a popular interface among C++
 * programmers and is platform-agnostic. As a result, the syncodecs family can be used both in
 * simulators (ns2, ns3) and in real testbeds.
 *
 * These are the basic steps to use a synthetic codec in your code:
 *
 * <ol>
 *   <li> Create a codec object from a subclass of #syncodecs::Codec
 *        (e.g., #syncodecs::PerfectCodec). Once created, the codec points to the first frame.</li>
 *   <li> To access the current frame record, dereference the codec object with
 *        operators "*" or "->" (as you would do with any other iterator). The frame record
 *        (#syncodecs::PacketOrFrameRecord) contains a pair (frame contents, time to next frame):
 *        <ul>
 *          <li> The first item is a vector of bytes and represents the payload (encoded frame).
 *               It typically contains garbage (hence the name "synthetic codec"), however the
 *               size of the vector is relevant. For instance, trace-based codecs will output
 *               frames containing zeros, but the frame sizes will reflect the information in the
 *               trace file (see #syncodecs::TraceBasedCodec and its subclasses).
 *               The reason for the pair #syncodecs::PacketOrFrameRecord to contain a vector,
 *               rather than just a scalar holding the size, is that more advanced codecs (or
 *               packetizers) may need to store some information in the payload.</li>
 *          <li> The second item is a real number denoting the number of seconds that the
 *                congestion controller needs to wait before advancing to the next frame.
 *                The main advantage of this interface design is that the syncodecs classes
 *                are totally detached from the threading model used by the congestion
 *                controller's application.</li>
 *        </ul>
 *   <li> To advance to the next frame, use the prepended increment operator ("++"); just as you
 *        would do with an iterator.</li>
 *   <li> The codec has a target bitrate that it will try to output. At any time, you can read
 *        and set the target bitrate using functions #getTargetRate and #setTargetRate .</li>
 *   <li> At any moment, you can cast the codec to a boolean to know if it is in a valid state.
 *        If the codec is in invalid state, it will be false when cast to a boolean. It will be
 *        true if it is in valid state.
 *        Most of the time you do not need worry about this, but some advanced codecs may need
 *        some initialization data, not provided by the constructor, in order to start working
 *        properly. Until this initialization is performed, the codec will be in invalid state.
 *        Another example would be a codec that can provide only a finite number of frames.
 *        When the codec is incremented past the last frame, it becomes invalid.</li>
 * </ol>
 */
class Codec : public std::iterator<std::input_iterator_tag, PacketOrFrameRecord>{
public:
    typedef Callback<void,float> RateCallback;
    void SetRateCallback(RateCallback cb){m_rateCb=cb;}
    /** Class constructor. Subclasses will call this class in their initialization list */
    Codec();

    /** Class destructor. Called after the subclasses' destructor is called */
    virtual ~Codec();

    /**
     * Accesses the current frame's data as a pair, which consists of the current frame's
     * contents ("first" member) and the seconds to wait before advancing to the next
     * frame ("second" member).
     *
     * See the class description above for further details.
     *
     * @retval #std::pair containing the frame contents and the seconds to wait for next frame.
     */
    virtual const value_type operator*() const;

    /**
     * Accesses the current frame's individual pair members: the current frame's
     * contents ("first" member) and the seconds to wait before advancing to the next
     * frame ("second" member).
     *
     * See the class description above for further details.
     *
     * @retval direct access to member "first" or "second" of the current frame.
     */
    virtual const value_type* operator->() const;

    /**
     * Increment operator. Advances to the next frame.
     *
     * @retval Reference to the updated codec.
     */
    virtual Codec& operator++();

    /**
     * Boolean cast.
     *
     * Evaluates to true if the codec is in valid state, i.e. its current frame
     * can be accessed and it can advance to the next frame.
     *
     * Evaluates to false otherwise.
     */
    virtual operator bool() const;

    /**
     * Obtain the codec's current target bitrate. The way the codec's implementation strives
     * to achieve the target bitrate depends on the particular implementation (codec subclass).
     *
     * @retval Target bitrate value in bits per second (bps).
     */
    virtual float getTargetRate() const;

    /**
     * Set the codec's current target bitrate. From now on, the codec's implementation will
     * strive to achieve the new target bitrate. The value must be greater than 0. The
     * function returns the target rate that will be used from now on. This return value
     * is useful to detect whether the codec could adopt the new target rate.
     *
     * @param [in] newRateBps New target bitrate in bits per second (bps).
     * @retval The new target rate (bps) at which the codec will operate from now on. If all went
     *         well it should be equal to the input parameter.
     */
    virtual float setTargetRate(float newRateBps);

protected:
    /**
     * Internal implementation of the class's boolean cast. Can be extended by subclasses.
     *
     * @retval true if the codec is in valid state, false otherwise.
     */
    virtual bool isValid() const;

    /**
     * Internal implementation of how to obtain the next frame. To a large extent, the
     * implementation of this method (together with initialization performed in the constructors)
     * is what differentiates the codecs in the syncodec's family.
     */
    virtual void nextPacketOrFrame() = 0;

    float m_targetRate; /**< Target bitrate value in bits per second (bps). */
    PacketOrFrameRecord m_currentPacketOrFrame; /**< Pair containing the current frame's info. */
    RateCallback m_rateCb;
};

/**
 * This abstract class is the superclass of all codecs that use the notion of frames per second.
 *
 * Typically, codecs that inherit from this class will return a number of seconds to wait
 * for next frame, based on the reference frame rate stored in #m_fps .
 *
 * Additionally, this class contains a number of protected members that generate some random
 * numbers/distributions. These can be used as noise model for the size of frames, and
 * frame intervals.
 *
 */
class CodecWithFps : virtual public Codec {
public:
    typedef double (*AddNoiseFunc)(double); /**< Typedef used for noise modeling callbacks. */
    /**
     * Class constructor.
     *
     * @param [in] fps The number of frames per second at which the codec is to operate.
     * @param [in] addFrSizeNoise Callback implementation modeling the noise of frame sizes
     *                            that can be provided to override the default implementation
     *                            based on a laplacian distribution. If set to NULL, no noise
     *                            model is applied.
     * @param [in] addFrInterNoise Callback implementation modeling the noise of frame intervals
     *                             that can be provided to override the default implementation
     *                             based on a laplacian distribution. If set to NULL, no noise
     *                             model is applied.
     */
    CodecWithFps(double fps,
                 AddNoiseFunc addFrSizeNoise,
                 AddNoiseFunc addFrInterNoise);

    /** Class destructor. Called after the subclasses' destructor is called */
    virtual ~CodecWithFps();

protected:
    /** Default laplacian-based implementation of the noise function applied to frame sizes */
    static double addLaplaceSize(double);
    /** Default laplacian-based implementation of the noise function applied to frame intervals */
    static double addLaplaceInter(double);
    /** Returns a random number between low and high, following a uniform distribution */
    static double uniform(double low, double high);
    /**
     * Returns a random number following a laplacian distribution (double exponential) with mean
     * and scale parameters given by input parameters mu and b
     */
    static double laplace(double mu, double b);

    float m_fps; /**< Current value of the reference frame rate (in fps). */
    AddNoiseFunc m_addFrSizeNoise; /**< Callback to add noise to frame sizes. Can be NULL. */
    AddNoiseFunc m_addFrInterNoise; /**< Callback to add noise to frame intervals. Can be NULL. */
private:
    static double addLaplaceNoise(double value, double mu, double b);
};

/**
 * This abstract class is the superclass of a group of codecs that are known as
 * <i>packetizers</i>: they not only encode the frames, but also split them into the sequence
 * of fragments that are to be shipped in the payload of individual RTP packets to be sent
 * over the network. In this class and all its subclasses the terms <i>frame</i> and <i>packet</i>
 * can be used interchangeably. For the sake of consistency, we will refer to them as
 * packets/frames when describing this class and its subclasses.
 *
 * Subclasses of this class need to know the maximum size of the payload so that the resulting
 * packets are not greater than the network's MTU. As a result, subclasses of this class should
 * never return a packet/frame contents with a size greater than member #m_payloadSize .
 */
class Packetizer : virtual public Codec {
public:
    /**
     * Class constructor.
     *
     * @param [in] payloadSize The maximum size of the payload that the codec can return
     *                         for a packet/frame.
     */
    Packetizer(unsigned long payloadSize);

    /** Class destructor. Called after the subclasses' destructor is called */
    virtual ~Packetizer();

protected:
    unsigned long m_payloadSize; /**< Maximum size of the payload returned by the codec (bytes). */
};


// End of abstract classes


/**
 * This class implements the smoothest form of synthetic codec. It is a subclass of #Packetizer ,
 * and is thus initialized with a maximum packet payload.
 *
 * The codec outputs packets/frames of a constant size, matching the configured maximum payload.
 * The interval at which the packets/frames are provided is constant and adapts when the target
 * bitrate is changed by the user.
 *
 * The name "perfect" comes from the fact that, as long as the target bitrate is stable, the
 * codec (a) produces no bursts or noise in the size of packets/frames, and (b) produces a
 * packet/frame sequence that accurately fits the target bitrate.
 */
class PerfectCodec : public Packetizer {
public:
    /**
     * Class constructor.
     *
     * @param [in] payloadSize The maximum size of the payload that the codec can return
     *                         for a packet/frame (bytes).
     */
    PerfectCodec(unsigned long payloadSize);
    /** Class destructor. Called after the subclasses' destructor is called */
    virtual ~PerfectCodec();

protected:
    /** Internal implementation of the perfect codec. */
    virtual void nextPacketOrFrame();
};



/**
 * This simplistic codec implementation provides a sequence of frames delivered at a
 * particular interval. If parameter #addFrInterNoise is NULL, the interval is constant.
 * Else, the codec adds some noise to the reference interval according to a the distribution
 * function specified by #addFrInterNoise .
 *
 * Optionally, a further distribution function can be configured as the noise model for the
 * frame sizes.
 *
 * When needed, the codec adapts the size of the frames to achieve the currently configured
 * target bitrate.
 *
 * @note This class delivers raw frames or a possibly big size. Therefore, the
 *       output frames might need to be split before they can be shipped in RTP
 *       packets. See #ShapedPacketizer .
 */
class SimpleFpsBasedCodec : public CodecWithFps {
public:
    /**
     * Class constructor.
     *
     * @param [in] fps The number of frames per second at which the codec is to operate.
     * @param [in] addFrSizeNoise Callback implementation modeling the noise of frame sizes.
     * @param [in] addFrInterNoise Callback implementation modeling the noise of frame intervals.
     */
    SimpleFpsBasedCodec(double fps = 25.,
                        AddNoiseFunc addFrSizeNoise = NULL,
                        AddNoiseFunc addFrInterNoise = &addLaplaceInter);

    /** Class destructor. Called after the subclasses' destructor is called */
    virtual ~SimpleFpsBasedCodec();

protected:
    /** Internal implementation of the simple codec. */
    virtual void nextPacketOrFrame();
};



/**
 * This codec is an advanced synthetic codec implementation in the syncodecs family.
 * It produces a sequence of frames with realistic sizes. The sequence of frame sizes correspond
 * to real codec output from a video sequence obtained offline.
 *
 * Upon initialization, the codec parses a group of video trace files and loads them in memory.
 * Each trace file contains information on the sequence of frames produced by a real codec.
 * Each line of the file corresponds to a frame record, where several fields can be
 * found (see #FrameDataIterator for further information on the format of the trace file).
 * This codec implementation only uses the "frame size" field (other fields such as frame type
 * or PSNR, included in the trace file, are not used by this codec's algorithm).
 *
 * The codec takes as parameter the path to a directory containing a number of trace files.
 * All trace files in that directory refer to the same raw video sequence, so all files should
 * contain the same number of frame records. Each file contains the traces resulting from encoding
 * the whole raw video sequence with a fixed resolution and a fixed target bitrate. The names of
 * the trace files must follow the following format:
 *
 * <prefix>_<resolution>_<target-bitrate>.txt
 *
 * where:
 * <ul>
 *   <li> <i>prefix</i> is an arbitrary string, but the same for all files. </li>
 *   <li> <i>resolution</i> is the fixed output resolution configured to encode the video
 *        sequence offline. It must be one of the following strings:
 *        "90p", "180p", "240p", "360p", "540p", "720p", and "1080p";
 *        which correspond respectively to the following 16:9 pixel resolutions:
 *        160x90, 320x180, 426x240, 640x360, 960x540, 1280x720, and 1920x1080. </li>
 *   <li> <i>target-bitrate</i> is the fixed target bitrate configured in the real video codec
 *        when encoding the video sequence offline. It is an integer denoting kilobits per
 *        second (kbps). The current implementation requires the target bitrate value to be
 *        contained within the range [#TRACE_MIN_BITRATE , #TRACE_MAX_BITRATE ], and
 *        divisible by #TRACE_BITRATE_STEP . </li>
 * </ul>
 *
 * For example, file <i>myAwesomeVideo_720p_1200.txt</i> contains the video
 * traces (i.e., the frame records) obtained when encoding the original raw video with a pixel
 * resolution of 1280x720 and a fixed target bitrate of 1200 kpbs.
 *
 * @note Ideally, the video trace directory should contain the Cartesian product of a set of
 *       resolutions and a set of target bitrates.
 *
 * Once the codec is set up, and so the video traces have been loaded in memory, the codec
 * transitions to valid state (the codec object will evaluate to true if cast to boolean) and
 * can henceforth be used.
 *
 * This codec's implementation mimics the operation of a real adaptive bitrate codec (ABR).
 * It contains two modes: fixed and variable resolution. For the sake of simplicity in
 * this explanation, let us first focus on the fixed resolution mode. In this mode the codec only
 * uses video traces from a fixed resolution, stored in #m_fixedResIt .
 *
 * The codec looks up all video traces that fulfill the following constraints:
 * <ul>
 *   <li> They have the fixed resolution #m_fixedResIt. </li>
 *   <li> They have a target bitrate that is less than the codec's own target
 *        bitrate (i.e., #m_targetRate , which is set using #setTargetRate ). </li>
 * </ul>
 *
 * The codec then chooses the video trace with the highest bitrate among those found. It will use
 * the chosen video trace to output its successive frame sizes.
 *
 * When the codec is incremented (operator "++"), an internal index, #m_currentFrameIdx , is
 * incremented to point to the next frame in the video trace currently in use. When the frame
 * info is accessed using operators "*" or "->", the size of the frame reported by the codec
 * matches the size of the frame pointed to by the internal index.
 *
 * When the internal index reaches the last frame in the video trace, it will wrap to the
 * beginning. However, the index will not wrap to the first frame (which is likely to be an
 * I-frame), but to frame number #N_FRAMES_EXCLUDED . This mechanism avoids having an
 * I-frame reported periodically in the synthetic codec's frame sequence. The reason is that
 * the typical behavior of most video-conferencing applications is to have an I-frame as the
 * first frame, and then only encoding other I-frames at the application's demand, when important
 * losses are observed.
 *
 * When the user sets a different target bitrate, the codec will re-run the lookup mechanism
 * explained above, and may end up choosing a different the video trace. However, when the
 * choice of the video trace is changed, the internal index pointing to the current frame in the
 * sequence is not modified. As a result, although the video trace is changed, the current frame
 * still refers to the same frame from the original raw video.
 *
 * Let us now see how the variable resolution mode extends the codec's behavior. When the codec is
 * in variable mode the resolution used in the lookup mechanism is not fixed, but evolves over
 * time. The current resolution is stored in #m_currentResIt . Every time the codec advances to
 * the next frame, an algorithm based of the <i>bits per pixel</i> concept is used.
 * In a nutshell, the codec calculates how many bits of the encoded frame are needed on
 * average for each pixel in the raw frame. If the result is less than a low threshold,
 * #m_lowBppThresh , it means that too few bits are available on average to encode a pixel and the
 * resolution is decreased to the next available (i.e., the next for which the codec has video
 * traces). Likewise, when the bits available on average to encode a pixel are too many
 * (above #m_highBppThresh ) the resolution is increased to the next one containing video traces.
 *
 * The bits per pixel idea works well for resolutions smaller than 540p, for bigger resolutions the
 * codec uses the power of .75 rule, proposed by Ben Waggoner. This rule calculates the bits per
 * pixel of resolution X by taking the bits per pixel results for 540p and applying the following
 * scaling factor: ( # of pixels in a resolution X frame / # of pixels in a 540p frame )^.75
 *
 * @note For an extensive description on the algorithm behind the trace-based codec,
 *       see IETF draft draft-ietf-rmcat-video-traffic-model.
 * @note For further details on the bits per pixel concept and how one can use it in order
 *       to guide the choice of resolution for an encoded frame, see Jan L. Ozer's book
 *       on video compression (ISBN-13:978-0976259503).
 */
class TraceBasedCodec : virtual public CodecWithFps {
    typedef std::string ResLabel;
    typedef std::pair<unsigned int, /* height */
                      unsigned int  /* width */
                      > Resolution;
    //Can't use a map because we want the keys ordered by their insertion
    typedef std::vector<std::pair<ResLabel, Resolution> > Labels2Res;

public:
    /**
     * Class constructor.
     *
     * @param [in] path The path to the directory where the files containing video traces are
     *                  located.
     * @param [in] filePrefix The common prefix that all video trace files must have.
     * @param [in] fps The number of frames per second at which the codec is to operate.
     * @param [in] fixed Boolean denoting whether the codec should start in fixed or variable
     *             resolution mode. If false, the codec starts in variable resolution; if true the
     *             codec starts in fixed resolution mode.
     */
    TraceBasedCodec(const std::string& path,
                    const std::string& filePrefix,
                    double fps,
                    bool fixed = false);

    /** Class destructor. Called after the subclasses' destructor is called */
    virtual ~TraceBasedCodec();

    /**
     * Set the mode to fixed or variable resolution, depending on the input parameter.
     *
     * If the mode set is fixed resolution, then the current resolution is changed to the one
     * previously set with #setResolutionForFixedMode . If no fixed resolution was previously set,
     * the middle resolution will be used. See #setResolutionForFixedMode for further details on
     * the middle resolution.
     *
     * If the mode set is variable resolution, then the current resolution is not changed, but from now
     * on it is free to evolve according to the resolution change algorithm used by the codec.
     *
     * @param [in] fixed If true, set the codec in fixed resolution mode. Otherwise set the
     *                   codec in variable resolution mode.
     */
    void setFixedMode(bool fixed);

    /**
     * Obtain the mode in which the codec is currently operating.
     *
     * @retval true if the codec is in fixed resolution mode, false if the codec is in variable
     *         resolution mode.
     */
    bool getFixedMode() const;

    /**
     * Set the resolution at which the codec will operate when in fixed mode to the
     * middle resolution.
     *
     * The middle resolution is the one that sits at index floor(n/2), where n denotes the number
     * of different resolutions for which the codec contains video traces.
     */
    void setResolutionForFixedMode();

    /**
     * Set the resolution at which the codec will operate when in fixed mode.
     *
     * @param [in] res The resolution to operate on fixed mode. This resolution has to be one of
     *                 the ones for which the codec has video traces, otherwise the codec does not
     *                 accept the new resolution.
     * @retval true if the codec accepts the resolution (i.e., it has video traces for it), false
     *         otherwise. In the latter case, no effect in the codec.
     */
    bool setResolutionForFixedMode(ResLabel res);

protected:
    typedef unsigned long Bitrate;
    typedef std::vector<LineRecord> FrameSequence;
    typedef std::map<Bitrate, FrameSequence> BitrateMap;
    typedef std::map<ResLabel, BitrateMap> ResolutionMap;

    /**
     * Internal implementation of the class's boolean cast. It extends its superclass's behavior
     * and can be extended by subclasses.
     *
     * @retval true if the codec is in valid state, false otherwise.
     */
    virtual bool isValid() const;

    /**
     * Internal utility function that returns the size in bytes of the current frame in the video
     * trace currently used.
     *
     * @param [in] rate The bitrate of the current video trace, used for searching the video
     *                  trace in the bitrate map.
     */
    unsigned long getFrameBytes(Bitrate rate);

    /** Internal implementation of the trace based codec. */
    virtual void nextPacketOrFrame();

    /**
     * Return the current bits per pixel value for the bitrate of the currently chosen video trace.
     *
     * @retval The bits per pixel value.
     */
    virtual double getCurrentBpp() const;

    /**
     * Adjust the current resolution of the video traces if the codec is operating in variable
     * resolution mode.
     */
    void adjustResolution();

    /**
     * Internal function that calculates the number of pixels in a frame and the scaling factor
     * to be used when calculating the current bits per pixel value.
     *
     * @param [out] scalingFactor The scaling factor returned.
     * @param [out] targetPixelsPerFrame The number of pixels in a frame of the current
     *                                   resolution. If the current resolution is bigger than
     *                                   540p, the result for 540p is returned instead.
     */
    void getBppData(double& scalingFactor, double& targetPixelsPerFrame) const;

    /** Prints the current resolution and bitrate for debugging purposes. */
    virtual void printResolutionAndBitrate() const;

    /** Implementation of the lookup mechanism explained in the class description. */
    virtual void matchBitrate();

    /**
     * Internal function that calculates the pixels contained in a frame of the resolution
     * passed as parameter.
     *
     * @param [in] resolution The resolution for which the number of pixels is sought.
     * @retval The number of pixels contained in a frame. Its type is double for convenience.
     */
    static double getPixelsPerFrame(ResLabel resolution);

    bool m_fixedModeEnabled; /**< true if currently in fixed resolution mode. */
    ResolutionMap m_traceData; /**< data structure that holds all video traces in memory. */
    size_t m_currentFrameIdx; /**< Internal pointer to the current frame of the video trace. */
    /**
     * Number of pixels per frame for the resolution above which Waggoner's rule applies.
     */
    double m_limitPixelsPerFrame;
    /**
     * Resolutions for which the codec has at least a video trace.
     */
    std::vector<ResLabel> m_resolutions;
    std::vector<ResLabel>::const_iterator m_currentResIt; /**< Points to the current resolution */
    std::vector<ResLabel>::const_iterator m_fixedResIt; /**< Points to resolution for fixed mode */

private:
    static Labels2Res m_labels2Res;
    static const float m_lowBppThresh;
    static const float m_highBppThresh;
    void decreaseResolution();
    void increaseResolution();
    bool traceDataIsValid() const;
    void readTraceDataFromDir(const std::string& path, const std::string& filePrefix);
    void readTraceDataFromFile(const std::string& filename,
                               const ResLabel& resolution,
                               Bitrate bitrate);

    Bitrate m_matchedRate;
};



/**
 * This codec offers extended functionality with respect to its superclass, #TraceBasedCodec .
 *
 * The #TraceBasedCodec uses the video trace data as is. It does not scale or interpolate it.
 * This class implements a scaling and interpolation algorithm that provides smoother results
 * in terms of frame sizes when the target bitrate undergoes small variations. Note that, when
 * the target bitrate variations are small, the #TraceBasedCodec tends to output <i>the same</i>
 * sequence of frame sizes.
 *
 * This is a short description of the scaling and interpolation algorithm. When the codec advances
 * to the next frame, the bitrate immediately below (same as #TraceBasedCodec ) and the bitrate
 * immediately above the current target bitrate are looked up in the video traces of the
 * current resolution. There are three cases:
 * <ul>
 *   <li> The current target bitrate is neither less than the minimum bitrate nor greater
 *        than the maximum bitrate of all video traces of the current resolution. In this case,
 *        the resulting frame size is calculated by linear interpolation of the current frame
 *        sizes for the below and above video traces. </li>
 *   <li> The current target bitrate is less than the minimum bitrate of all video traces of the
 *        current resolution. In this case the resulting frame size is calculated by
 *        scaling the frame size for the below video trace, with respect to the current
 *        target bitrate. </li>
 *   <li> The current target bitrate is greater than the maximum bitrate of all video traces of
 *        the current resolution. In this case the resulting frame size is calculated by
 *        scaling the frame size for the above video trace, with respect to the current
 *        target bitrate. </li>
 * </ul>
 *
 * @note For further details on the algorithm used for scaling and interpolating, please see
 *       IETF draft draft-ietf-rmcat-video-traffic-model.
 */

class TraceBasedCodecWithScaling : public TraceBasedCodec {
public:
    /**
     * Class constructor.
     *
     * @param [in] path The path to the directory where the files containing video traces are
     *                  located.
     * @param [in] filePrefix The common prefix that all video trace files must have.
     * @param [in] fps The number of frames per second at which the codec is to operate.
     * @param [in] fixed Boolean denoting whether the codec should start in fixed or variable
     *                   resolution mode. If false, the codec starts in variable resolution;
     *                   if true the codec starts in fixed resolution mode.
     */
    TraceBasedCodecWithScaling(const std::string& path,
                               const std::string& filePrefix,
                               double fps,
                               bool fixed = false);

    /** Class destructor. Called after the subclasses' destructor is called */
    virtual ~TraceBasedCodecWithScaling();

protected:
    /** Internal implementation of the trace based codec with scaling and interpolation. */
    virtual void nextPacketOrFrame();

    /**
     * Return the current bits per pixel value for the exact target bitrate.
     *
     * @retval The bits per pixel value.
     */
    virtual double getCurrentBpp() const;

    /** Prints the current resolution and bitrate for debugging purposes. */
    virtual void printResolutionAndBitrate() const;

    /**
     * Implementation of the extended lookup mechanism whereby the bitrate immediately above and
     * the one immediately below are chosen.
     */
    virtual void matchBitrate();

private:
    Bitrate m_lowRate;
    Bitrate m_highRate;
};


/**
 * This codec is part of the group of packetizers. It is aware of the maximum payload that it
 * should output.
 *
 * The #ShapedPacketizer is not a full-fledged codec, but a wrapper of other codecs. Its
 * constructor takes another codec as parameter: the inner codec. The idea behind the shaped
 * packetizer is that it extracts frames from the inner codec, and then splits them and
 * delivers those fragments as its own packets/frames. Obviously, the best inner codec candidates
 * are those that are not subclasses of #Packetizer .
 *
 * Another aspect of the shaped packetizer is that it performs a mild shaping of the
 * packets/frames. Rather than delivering all fragments of an inner frame as soon as the
 * inner frame is available, it shapes the delivery of the fragments: it spreads their delivery
 * throughout the "seconds to next frame" value of the inner codec.
 *
 * For example, consider an inner codec that has just advanced to its next frame (i.e.,
 * the shaped packetizer's implementation has just called the "++" operator in the inner codec).
 * The size of the new inner frame is 3500 bytes, and the inner "seconds to next frame" value
 * is 40ms. The user has configured the shaped packetizer with 1000 bytes as maximum payload size
 * and 0 as per-packet overhead. In this situation, the shaped packetizer will output the next
 * 4 packets/frames at 10-ms-long intervals, the first 3 of those 4 packets/frames will be
 * 1000 bytes long and the 4th will be 500 bytes long. Finally, when the shaped packetizer advances
 * beyond the 4th packet, it will advance the inner codec and process the new inner frame.
 *
 * The codec supports a per-packet overhead to be configured. The meaning of this value is how
 * many bytes are going to added to every packet/frame when it is sent over the network. The codec
 * uses this information to throttle the inner codec's target bitrate. The goal is that
 * the target bitrate set on the shaped packetizer is as close as possible to the actual bitrate
 * sent over the network. This is typically useful when one is using a rather simple
 * codec (e.g. #SimpleFpsBasedCodec) to test a congestion control algorithm and does not want
 * the network per-packet overhead (e.g. IP+UDP+RTP headers) to affect the result plots.
 *
 * If you do not need to care about your network's per-packet overhead, you can just set its
 * value to 0.
 */
class ShapedPacketizer : public Packetizer {
public:
    /**
     * Class constructor.
     *
     * @param [in] innerCodec A pointer to the inner codec. Once the constructor is called, the
     *                        class retains ownership of the object, and will free it in the
     *                        destructor.
     * @param [in] payloadSize The maximum size in bytes of the payload that the codec can return
     *                         for a packet/frame.
     * @param [in] perPacketOverhead The amount of bytes that every frame/packet is expected to
     *                         be added before hitting the wire (header sizes of IP, UDP, etc.)
     */
    ShapedPacketizer(Codec* innerCodec, unsigned long payloadSize, unsigned int perPacketOverhead = 0);

    /** Class destructor. Called after the subclasses' destructor is called */
    virtual ~ShapedPacketizer();

protected:
    /**
     * Internal implementation of the class's boolean cast. It extends its superclass's behavior
     * and can be extended by subclasses.
     *
     * @retval true if the codec is in valid state, false otherwise.
     */
    virtual bool isValid() const;

    /** Internal implementation of the shaped packetizer. */
    virtual void nextPacketOrFrame();

    /**
     * Holds a pointer to the inner codec passed in the constructor. This class takes ownership
     * of the pointed object, and will free the memory upon destruction.
     */
    std::auto_ptr<Codec> m_innerCodec;
    unsigned int m_overhead; /**< Stores the per-packet overhead. */
    std::vector<uint8_t> m_bytesToSend; /**< Bytes not yet sent from current inner frame. */
    double m_secsToNextFrame; /**< Seconds left until the next inner frame. */
    double m_lastOverheadFactor; /**< Overhead ratio for last inner frame's packets/frames. */

private:
    // For the moment we disable copying objects of this class
    ShapedPacketizer(const ShapedPacketizer&);
    ShapedPacketizer& operator=(const ShapedPacketizer&);
};


/**
 * This synthetic codec mimics the operation of a real codec by implementing a statistical
 * model. This model has two phases: the steady phase and the transient phase.
 *
 * The codec is in the steady phase as long as the changes in the target rate are not
 * substantial. A change is substantial when the ratio new rate/old rate is greater
 * than  #m_bigChangeRatio (this threshold is configurable in the class's constructor).
 * While in steady state, the codec creates a sequence of frames, whose size is chosen
 * to fit the target rate given that the frames are sent at  #m_fps frames per second.
 *
 * When there is a substantial change in the target rate, the codec enters transient phase.
 * The transient phase has fixed duration #m_transientLength expressed in frames. In the
 * transient phase, the first frame is an I-frame, and is modeled as a frame whose size
 * is #m_iFrameSize bytes. This I-frame size tends to be much bigger than the reference frame
 * size at the current target rate. This value has been observed to depend on the codec type,
 * resolution and/or video sequence, but not on the current target bitrate. The size of the
 * remaining frames in the transient phase are made smaller to compensate for the I-frame's
 * size, so that, at the end of the transient period the average bitrate still fits the
 * target rate. In any case, these remaining frames in the transient phase will never be smaller
 * than 0.2 times the size of a steady frame, so if the I-frame is very big, or the transient
 * period is very short, the average bitrate of the transient period might overshoot the
 * target rate.
 *
 * Whether in steady state or transient state, the last step before delivering the frame
 * is to modify its size to simulate noise. This size is modified by applying a
 * function to the frame size that (slightly) modifies it. The function to apply is stored
 * as a callback in #m_addFrSizeNoise , which can be set in via the constructor of the class
 * to any callback function that the user may provide. If the user does not provide a function
 * the default callback, #addLaplaceSize , is used. The default callback modifies the size
 * of each frame by enlarging/shrinking according to a laplacian random distribution.
 * As mentioned above, to model the noise in frame size the user can provide a different
 * implementation as a callback in the constructor.
 *
 * There is another callback provided to add noise to the (otherwise constant) frame interval
 * configured. The default callback also implements a laplacian random distribution.
 *
 * Both default noise models have been chosen according to observations made during our study
 * of the behavior of Firefox's h264 codec implementation.
 *
 * There is a limit to how much the current target rate can be changed in one shot: the
 * ratio between the old and the new value for the target rate cannot be bigger
 * than #m_maxUpdateRatio . There is one exception: it the ratio old/new value for target
 * rate is bigger than #m_bigChangeRatio (substantial change), the limit does not apply,
 * the target rate changes to new value, and (as explained above) the codec enters in
 * transient phase.
 *
 * Finally, when the user updates the target rate, the codec will refuse any further update
 * for the next #m_updateInterval seconds.
 *
 * @note For further details on this algorithm and the default noise functions, please see
 *       IETF draft draft-ietf-rmcat-video-traffic-model.
 */

class StatisticsCodec : virtual public CodecWithFps {
public:
    /**
     * Class constructor.
     *
     * @param [in] fps The number of frames per second at which the codec is to operate.
     * @param [in] maxUpdateRatio The limit in the target rate update in one shot, expressed
     *                            in terms of ratio old/new target rate. 0 to disable this limit.
     * @param [in] updateInterval The interval in seconds that needs to elapse after a successful
     *                            target rate update before the codec accepts again a new update
     *                            to the target rate.
     * @param [in] bigChangeRatio The threshold to consider a target rate update as substantial,
     *                            thereby triggering a new transient phase.
     * @param [in] transientLength Length of the transient phase in frames.
     * @param [in] iFrameSize Reference size of an I-frame in bytes.
     * @param [in] addFrSizeNoise Callback implementation modeling the noise of frame sizes.
     * @param [in] addFrInterNoise Callback implementation modeling the noise of frame intervals.
     */
    StatisticsCodec(double fps,
                    float maxUpdateRatio = .1, // 10%
                    double updateInterval = .1, // 100 ms
                    float bigChangeRatio = .5, // 50%
                    unsigned int transientLength = 10, // frames
                    unsigned long iFrameSize = 4.17 * 1024, // bytes
                    AddNoiseFunc addFrSizeNoise = &addLaplaceSize,
                    AddNoiseFunc addFrInterNoise = &addLaplaceInter);

    /** Class destructor. Called after the subclasses' destructor is called */
    virtual ~StatisticsCodec();

    /**
     * Set the codec's current target bitrate. From now on, the codec's implementation will
     * strive to achieve the new target bitrate. The value must be greater than 0. Other
     * rules also apply, such as (1) the limit in the target rate change, and (2) the interval
     * after an update during which further updates are rejected (both are explained in the
     * class description).
     *
     * The function returns the target rate that will be used from now on. This return value
     * is useful to detect what new target rate the codec ended up adopting.
     *
     * @param [in] newRateBps New target bitrate in bits per second (bps).
     * @retval The new target rate (bps) at which the codec will operate from now on. If all went
     *         well it should be equal to the input parameter.
     *
     * @note In future versions, we are considering promoting rules (1) and (2) mentioned above
     *       to the #setTargetRate implementation of abstract class #CodecWithFps. This way,
     *       other codecs (not packetizers, as it does not make sense for them) can benefit from
     *       this extended behavior.
     */
    virtual float setTargetRate(float newRateBps);

protected:
    /** Internal implementation of the statistics-based codec. */
    virtual void nextPacketOrFrame();

    float m_maxUpdateRatio; /**< Maximum ratio of up/down target rate change. 0 to disable. */
    double m_updateInterval; /**< Interval in seconds between two consecutive rate updates. */
    float m_bigChangeRatio; /**< Minimum ratio old/new target rate to trigger transient phase. */
    unsigned int m_transientLength; /**< Length of a transient phase in terms of # of frames. */
    unsigned long m_iFrameSize; /**< Reference size in bytes of I-frame . */
    double m_timeToUpdate; /**< Time remaining until next target rate update will be accepted. */
    unsigned int m_remainingBurstFrames; /** # of frames left in current transient phase. */
};

/**
 * This synthetic codec simulates the frame sizes that result from encoding a content sharing video,
 * e.g., a slide deck being shared in a meeting with remote participants.
 *
 * The frame rate is typically very low (default: 5 fps) although its value can be configured.
 *
 * There are two types of frames. The first type are small frames, called "no change" frames, which
 * are typically P-frames that represent frames being encoded when there is no change in the content
 * being shared. The second type of frames are big frames, which are typically I-frames that represent
 * a change in the shared content (e.g., the user changed the current slide). The probability for any
 * frame to be a big (I-)frame should be low (default: 0.05) although it is configurable through
 * #m_bigFrameProb . In any case, the first frame in the sequence is always a big frame.
 *
 * The maximum size of small frames is #m_noChangeMaxSize . If the current target rate is very low
 * the size of small frames will be shrunk so that the codec conforms to the target rate.
 *
 * The size of big frames is calculated as follows. A first size, <i>s_0</i>, is calculated as though it
 * was a small frame (see previous paragraph). Then a multiplier <i>m</i> is obtained from the uniform
 * random distribution [#m_bigFrameRatioMin , #m_bigFrameRatioMax ]. Finally, multiplier <i>m</i>
 * is applied to the initial size <i>s_0</i>.
 */

class SimpleContentSharingCodec : public CodecWithFps {
public:
    /**
     * Class constructor.
     *
     * @param [in] fps The number of frames per second at which the codec is to operate.
     * @param [in] noChangeMaxSize The maximum size of a ("no change") frame regardless of the
     *                             target rate.
     *                         (P-frame) produced while in steady state.
     * @param [in] bigFrameProb Probability of a frame to be a big (I-)frame
     * @param [in] bigFrameRatioMin Minimum size of a big (I-)frame in terms of ratio to a
     *                              ("no change") frame produced when content did not change.
     * @param [in] bigFrameRatioMax Maximum size of a big (I-)frame in terms of ratio to a
     *                              ("no change") frame produced when content did not change.
     */
    SimpleContentSharingCodec(double fps = 5., // default fps for content sharing
                              unsigned long noChangeMaxSize = 1000, // bytes
                              float bigFrameProb = .05,
                              float bigFrameRatioMin = 20.,
                              float bigFrameRatioMax = 200.);

    /** Class destructor. Called after the subclasses' destructor is called */
    virtual ~SimpleContentSharingCodec();

protected:
    /** Internal implementation of the statistics-based codec. */
    virtual void nextPacketOrFrame();

    unsigned long m_noChangeMaxSize; /**< Maximum size of a "no change" frame. */
    float m_bigFrameProb; /**< Probability for a frame to be big. */
    float m_bigFrameRatioMin; /**< Minimum ratio of a big frame to a "no change" frame. */
    float m_bigFrameRatioMax; /**< Maximum ratio of a big frame to a "no change" frame. */
    float m_first; /**< Denotes whether the current frame is the first in the sequence. */
};

/**
 * This synthetic codec combines the behavior of its two parent classes: the statistics
 * codec (#StatisticsCodec ) and the trace based codec with scaling and interpolation
 * (#TraceBasedCodecWithScaling ). It is an attempt to get "the best of both worlds".
 *
 * The codec reuses the two phases defined in the statistics codec: the steady phase and
 * the transient phase. The way the codec transitions from the steady to the transient
 * phase matches the statistics codec: the transition occurs when the target rate undergoes
 * a substantial change. A rate change is substantial when the ratio new rate/old rate is
 * greater than #m_bigChangeRatio . The length of the transient phase is also taken from the
 * statistics codec: the transient phase has fixed duration #m_transientLength expressed in
 * frames.
 *
 * When in the steady phase, the codec behaves according to its trace based parent class,
 * providing frame sizes according to the traces loaded in memory when the codec's C++
 * object was instantiated. In the transient phase, the codec sends a big (I-)frame followed
 * by smaller frames, just as the statistics codec does. The sizes of all frames in the
 * transient phase are modified by the noise function #m_addFrSizeNoise ; just like the
 * statistics codec.
 *
 * Whether in steady or transient phase, the codec will apply the noise function
 * #m_addFrInterNoise (if not NULL) to the frame interval of all frames. Also in both
 * phases, the time damped response to rate changes (defined by the statistics codec)
 * is also applied: after accepting a target rate update, the codec will refuse any further
 * updates for the next #m_updateInterval seconds.
 *
 * @note For further details on how this codec combines the statistics and trace based
 *       algorithms, please see IETF draft draft-ietf-rmcat-video-traffic-model.
 */
class HybridCodec : public StatisticsCodec, public TraceBasedCodecWithScaling {
public:
    /**
     * Class constructor.
     *
     * @param [in] path The path to the directory where the files containing video traces are
     *                  located.
     * @param [in] filePrefix The common prefix that all video trace files must have.
     * @param [in] fps The number of frames per second at which the codec is to operate.
     * @param [in] fixed Boolean denoting whether the codec should start in fixed or variable
     *                   resolution mode. If false, the codec starts in variable resolution;
     *                   if true the codec starts in fixed resolution mode.
     * @param [in] maxUpdateRatio The limit in the target rate update in one shot, expressed
     *                            in terms of ratio old/new target rate. 0 to disable this limit.
     * @param [in] updateInterval The interval in seconds that needs to elapse after a successful
     *                            target rate update before the codec accepts again a new update
     *                            to the target rate.
     * @param [in] bigChangeRatio The threshold to consider a target rate update as substantial,
     *                            thereby triggering a new transient phase.
     * @param [in] transientLength Length of the transient phase in frames.
     * @param [in] iFrameSize Reference size of an I-frame in bytes.
     * @param [in] addFrSizeNoise Callback implementation modeling the noise of frame sizes.
     * @param [in] addFrInterNoise Callback implementation modeling the noise of frame intervals.
     */

    HybridCodec(const std::string& path,
                const std::string& filePrefix,
                double fps,
                bool fixed = false,
                float maxUpdateRatio = .05, // 10%
                double updateInterval = .05, // 50 ms
                float bigChangeRatio = .1, // 10%
                unsigned int transientLength = 10, // frames
                unsigned long iFrameSize = 4.17 * 1024, // bytes
                AddNoiseFunc addFrSizeNoise = &addLaplaceSize,
                AddNoiseFunc addFrInterNoise = &addLaplaceInter);

    /** Class destructor. Called after the subclasses' destructor is called */
    virtual ~HybridCodec();

protected:
    /** Internal implementation of the hybrid codec. */
    virtual void nextPacketOrFrame();
};


} /* namespace syncodecs */

/**
 * @file
 *
 * Here are some examples on how to use the most relevant codecs of the syncodecs family.
 *
 * EXAMPLE 1.
 *
 * In the first example we replay the behavior of various codecs in slow motion. We
 * slow down 100 times to appreciate the effect of changing the target bitrate.
 *
 * Sample code:
 * @code
 * #include "syncodecs.h"
 * #include <memory>
 * #include <cassert>
 * #include <iostream>
 * #include <iomanip>
 * #include <unistd.h>
 *
 * #define MAX_PKT_SIZE 1000 // bytes
 *
 * void setRate(syncodecs::Codec& c, unsigned int rate) {
 *     std::cout << "    Setting target rate to ~" << rate << " Mbps" << std::endl;
 *     float result = c.setTargetRate(rate * 1e6f);
 *     assert(result == rate * 1e6f);
 * }
 *
 * void playCodec(syncodecs::Codec& myCodec, unsigned int framesPerRate, int nframes, unsigned int slowDown) {
 *     for (int i = 0; i < nframes; ++i) {
 *         if (i % framesPerRate == 0) {
 *             setRate(myCodec, i / framesPerRate + 1);
 *         }
 *         ++myCodec;
 *         std::cout << "      Time for frame/packet #" << i << ", size: "
 *                   << myCodec->first.size() << std::endl;
 *         std::cout << "        waiting " << std::setprecision(2) << myCodec->second * 1000.
 *                   << " ms..." << std::endl;
 *         usleep(myCodec->second * 1e6 * slowDown);
 *     }
 * }
 *
 * int main(int argc, char** argv) {
 *     std::cout << "Playing the behavior of various codecs in slow motion..." << std::endl;
 *     std::cout << std::fixed;
 *
 *     const unsigned int slow1 = 100;
 *     std::cout << "  Perfect codec. "
 *               << slow1 << "x slower:" << std::endl;
 *     std::auto_ptr<syncodecs::Codec> codec1(new syncodecs::PerfectCodec(MAX_PKT_SIZE));
 *     playCodec(*codec1, 10, 200, slow1);
 *
 *     const unsigned int slow2 = 50;
 *     std::cout << std::endl << std::endl;
 *     std::cout << "  Simple fps-based codec (unwrapped). "
 *               << slow2 << "x slower:" << std::endl;
 *     std::auto_ptr<syncodecs::Codec> codec2(new syncodecs::SimpleFpsBasedCodec(30.));
 *     playCodec(*codec2, 5, 20, slow2);
 *
 *     const unsigned int slow3 = 50;
 *     std::cout << std::endl << std::endl;
 *     std::cout << "  Simple fps-based codec (wrapped in the shaped packetizer). "
 *               << slow3 << "x slower:" << std::endl;
 *     syncodecs::Codec* inner3 = new syncodecs::SimpleFpsBasedCodec(30.);
 *     std::auto_ptr<syncodecs::Codec> codec3(new syncodecs::ShapedPacketizer(inner3, MAX_PKT_SIZE));
 *     playCodec(*codec3, 10, 200, slow3);
 *
 *     const unsigned int slow4 = 5;
 *     std::cout << std::endl << std::endl;
 *     std::cout << "  Simple content sharing codec (wrapped in the shaped packetizer). "
 *               << slow4 << "x slower:" << std::endl;
 *     syncodecs::Codec* inner4 = new syncodecs::SimpleContentSharingCodec(5., 800);
 *     std::auto_ptr<syncodecs::Codec> codec4(new syncodecs::ShapedPacketizer(inner4, MAX_PKT_SIZE));
 *     playCodec(*codec4, 50, 500, slow4);
 *
 *     return 0;
 * }
 * @endcode
 *
 *
 * EXAMPLE 2.
 *
 * In the second example, we demonstrate how one can simulate three different codecs, running
 * concurrently, with only one simulation thread. In this example we have chosen to use
 * a more advanced codec setup, in order to demonstrate the usage of the trace-based, the
 * statistics, and the hybrid codecs.
 * Sample code:
 * @code
 * #include "syncodecs.h"
 * #include <memory>
 * #include <cassert>
 * #include <iostream>
 * #include <iomanip>
 * #include <algorithm>
 * #include <iterator>
 *
 * #define MAX_PKT_SIZE 1000 // bytes
 * #define AUTOPTR(x) std::auto_ptr<syncodecs::Codec>((x))
 * #define TRACES_DIR_PATH "video_traces/chat_firefox_h264"
 * #define TRACES_FILE_PREFIX "chat"
 *
 * static void setRate(syncodecs::Codec& c, unsigned int codecN, unsigned int rate) {
 *     std::cout << "  Setting codec " << codecN << "'s target rate to "
 *               << rate << " Kbps";
 *     float result = c.setTargetRate(rate * 1024);
 *     std::cout << ". Accepted rate " << result / 1024 << " Kbps" << std::endl;
 * }
 *
 * static void processEarliestFrame(double& now, int idx, syncodecs::Codec& c,
 *                                  double& time, unsigned int& nFrame) {
 *     assert(now <= time);
 *     now += (time - now);
 *     time += c->second;
 *     std::cout << "    Time " << int(now * 1000.) << " ms: codec " << idx
 *               << "'s frame #" << nFrame << ", size: " << c->first.size()
 *               << ", next frame due @ " << int(time * 1000.) << " ms" << std::endl;
 *     ++c;
 *     ++nFrame;
 * }
 *
 * int main(int argc, char** argv) {
 *     syncodecs::Codec* inner0 = new syncodecs::TraceBasedCodecWithScaling(
 *                                     TRACES_DIR_PATH, TRACES_FILE_PREFIX, 25.);
 *     syncodecs::Codec* inner1 = new syncodecs::StatisticsCodec(25.);
 *     syncodecs::Codec* inner2 = new syncodecs::HybridCodec(
 *                                     TRACES_DIR_PATH, TRACES_FILE_PREFIX, 25.);
 *     std::auto_ptr<syncodecs::Codec> codecs[] = { AUTOPTR(inner0),
 *                                                  AUTOPTR(inner1),
 *                                                  AUTOPTR(inner2)
 *                                                };
 *     static const int n = sizeof(codecs) / sizeof(codecs[0]);
 *     double times[n] = {};
 *     unsigned int nFrames[n] = {};
 *     double now = 0.;
 *
 *     std::cout << "Simulating " << n << " codecs running in parallel "
 *               << "with one single thread" << std::endl;
 *
 *     for (int i = 0; i < 1000; ++i) {
 *         if (i % 10 == 0) {
 *             const unsigned int newRate = 500 + 10 * (i / 10);
 *             for (int j = 0; j < n; ++j) {
 *                 setRate(*(codecs[j]), j, newRate);
 *             }
 *         }
 *         int j = std::distance(&times[0], std::min_element(&times[0], &times[n]));
 *         processEarliestFrame(now, j, *(codecs[j]), times[j], nFrames[j]);
 *     }
 *
 *     return 0;
 * }
 * @endcode
 */
}
#endif  /* SYNCODECS_H_ */
