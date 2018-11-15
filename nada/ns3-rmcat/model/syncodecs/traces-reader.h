/******************************************************************************
 * Copyright 2014-2015 cisco Systems, Inc.                                    *
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
 * Traces reader header file. The traces reader loads a file containing traces in a simple format
 * into a particular data structure in memory.
 *
 * The file format used includes the data for a frame in a line. So the file contains as many
 * lines as frames are in the trace, one frame per line, in chronological order.
 * Each line has the following fields: frame number, frame type (I: I-frame, P: P-frame,
 * B: B-frame), quantization parameter used for encoding the frame, PSNR obtained,
 * frame size in bytes. The fields are separated by a blank. See #syncodecs::LineRecord for
 * further details.
 *
 * When a comment character '#' or '%' is encountered, the rest of the
 * line is ignored. Empty or comment-only lines are allowed.
 *
 * What follows is an example of the beginning of a trace file.
 *
 * % Nframe | type | QP | PSNR | size (Bytes)
 * 0 I 26.67 40.23 24232
 * 1 P 28.73 39.79 1169 # This is the second frame
 * 2 P 29.52 39.69 2327
 * 3 P 29.95 39.74 3217
 * 4 P 29.31 39.81 3685
 *
 * The traces reader is implemented as an std-like iterator. See #syncodecs::FrameDataIterator for
 * further details.
 *
 * @version 0.1.0
 * @author Sergio Mena de la Cruz (semena@cisco.com)
 * @author Stefano D'Aronco (stefano.daronco@epfl.ch)
 * @copyright Apache v2 License
 */

#ifndef TRACES_READER_H_
#define TRACES_READER_H_

#include <iterator>
#include <sstream>
#include <cassert>
namespace ns3
{
namespace syncodecs {

/**
 * Contains a record of a line of the trace file. Each of the fields is kept in a member variable
 * of this class.
 */
class LineRecord {
public:
    template<typename FTYPE>
    class Field {
    public:
        Field(std::istringstream& stream);
        operator FTYPE const &() const;
        void checkReadStrCompletely(const std::istringstream& str) const;
    private:
        FTYPE m_data;
    };
    Field<unsigned int> m_frameNumber; /**< Frame number. */
    Field<char> m_frameType;           /**< Frame type. I = I-frame; P = P-frame; B = B-frame; U = Unknown. */
    Field<float> m_qp;                 /**< Quantization parameter used when encoding the frame. */
    Field<float> m_ts;                 /**< Timestamp when the frame was sent (in seconds). */
    Field<unsigned long> m_size;       /**< Frame size in bytes. */
    LineRecord(std::istringstream& stream);
};

template<typename FTYPE> LineRecord::Field<FTYPE>::Field(std::istringstream& stream) {
    std::string field;
    getline(stream, field, ' ');
    assert(stream);
    std::istringstream fStr(field);
    fStr >> m_data;
    checkReadStrCompletely(fStr);
    assert(fStr);
}

template<typename FTYPE> LineRecord::Field<FTYPE>::operator const FTYPE&() const {
    return m_data;
}

template<typename FTYPE>
void LineRecord::Field<FTYPE>::checkReadStrCompletely(const std::istringstream& str) const {
    assert(str.eof());
}
// Special case: cannot test for stream's eof is case of chars
template<>
void LineRecord::Field<char>::checkReadStrCompletely(const std::istringstream& str) const;


/**
 * This std-like iterator class walks through a trace file sequentially. At each iteration, the
 * value referenced is a line record (#syncodecs::LineRecord). As the class inherits
 * from #std::iterator, you can use #syncodecs::FrameDataIterator::value_type, rather than
 * #syncodecs::LineRecord as the type of the value referenced by the iterator.
 */
class FrameDataIterator : public std::iterator<std::input_iterator_tag, LineRecord>{
public:
    FrameDataIterator();
    FrameDataIterator(std::istream& in);

    const value_type operator*() const;
    FrameDataIterator& operator++();
    bool operator!=(const FrameDataIterator& rhs) const;
    bool operator==(const FrameDataIterator& rhs) const;
    operator bool() const;

private:
    bool isValid() const;
    void findNonEmpty();

    std::istream* m_input;
    std::string m_line;
};

} /* namespace syncodecs */
}//namespace ns3
/**
 * @file
 *
 * This code snippet is an example in how the traces reader is to be used.
 *
 * Sample code:
 * @code
 * #include "traces-reader.h"
 * #include <fstream>
 * #include <iostream>
 * #include <cassert>
 *
 * int main(int argc, char** argv) {
 *     const char * filename = "my_traces_file.txt";
 *     std::ifstream fin(filename);
 *     assert(fin);
 *
 *     std::cout << "Dumping contents of traces file " << filename << ":" << std::endl;
 *     syncodecs::FrameDataIterator it(fin);
 *
 *     while (it) {
 *         const syncodecs::FrameDataIterator::value_type& record = *it;
 *         std::cout << "  Frame #" << record.m_frameNumber
 *                   << "; Type: " << record.m_frameType
 *                   << "; QP: " << record.m_qp
 *                   << "; timestamp: " << record.m_ts
 *                   << "; Size (bytes): " << record.m_size
 *                   << std::endl;
 *         ++it;
 *     }
 *     return 0;
 * }
 * @endcode
 */

#endif /* TRACES_READER_H_ */
