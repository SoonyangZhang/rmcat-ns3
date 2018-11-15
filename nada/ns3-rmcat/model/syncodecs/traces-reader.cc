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
 * Traces reader implementation file.
 *
 * @version 0.1.0
 * @author Sergio Mena
 * @author Stefano D'Aronco
 */

#include "traces-reader.h"
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
namespace ns3
{
namespace syncodecs {

template<>
void LineRecord::Field<char>::checkReadStrCompletely(const std::istringstream& str) const
{}

LineRecord::LineRecord(std::istringstream& stream) :
    m_frameNumber(stream), m_frameType(stream),
    m_qp(stream), m_ts(stream), m_size(stream) {}

FrameDataIterator::FrameDataIterator() : m_input(NULL), m_line() {}


FrameDataIterator::FrameDataIterator(std::istream& in) : m_input(&in), m_line() {
    findNonEmpty();
}


bool FrameDataIterator::isValid() const {
    return m_line.size() != 0;
}

const FrameDataIterator::value_type FrameDataIterator::operator*() const {
    assert(isValid());

    std::istringstream recStream(m_line);
    value_type data(recStream);
    return data;
}

template<char t>
bool isChar(char c) {
    return t == c;
}

void FrameDataIterator::findNonEmpty() {
    while (m_line.size() == 0 && *m_input) {
        getline(*m_input, m_line);
        //remove comments starting with #
        m_line.erase(std::find_if(m_line.begin(), m_line.end(), isChar<'#'>), m_line.end());
        //remove comments starting with %
        m_line.erase(std::find_if(m_line.begin(), m_line.end(), isChar<'%'>), m_line.end());
        //remove leading
        m_line.erase(m_line.begin(),
                    std::find_if(m_line.begin(), m_line.end(),
                                 std::not1(std::ptr_fun<int, int>(std::isspace))));
        //remove trailing
        m_line.erase(std::find_if(m_line.rbegin(), m_line.rend(),
                                 std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
                    m_line.end());
    }
}

FrameDataIterator& FrameDataIterator::operator++() {
    assert(isValid());
    m_line.clear();
    findNonEmpty();
    return *this;
}

bool FrameDataIterator::operator!=(const FrameDataIterator& rhs) const {
    return m_input != rhs.m_input;
}

bool FrameDataIterator::operator==(const FrameDataIterator& rhs) const {
    return !(*this != rhs);
}

FrameDataIterator::operator bool() const {
    return isValid();
}

}
}
