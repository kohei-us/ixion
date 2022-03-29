/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "utf8.hpp"

#include <ixion/exceptions.hpp>
#include <sstream>

namespace ixion { namespace detail {

namespace {

constexpr uint8_t invalid_utf8_byte_length = std::numeric_limits<uint8_t>::max();

uint8_t calc_utf8_byte_length(uint8_t c1)
{
    if ((c1 & 0x80) == 0x00)
        // highest bit is not set.
        return 1;

    if ((c1 & 0xE0) == 0xC0)
        // highest 3 bits are 110.
        return 2;

    if ((c1 & 0xF0) == 0xE0)
        // highest 4 bits are 1110.
        return 3;

    if ((c1 & 0xFC) == 0xF0)
        // highest 5 bits are 11110.
        return 4;

    return invalid_utf8_byte_length;
}

}

std::vector<std::size_t> calc_utf8_byte_positions(const std::string& s)
{
    const char* p = s.data();
    const char* p0 = p; // head position
    const char* p_end = p + s.size();

    std::vector<std::size_t> positions;

    while (p < p_end)
    {
        positions.push_back(std::distance(p0, p));

        uint8_t n = calc_utf8_byte_length(*p);

        if (n == invalid_utf8_byte_length)
        {
            std::ostringstream os;
            os << "invalid utf8 byte length in string '" << s << "'";
            throw general_error(os.str());
        }

        p += n;
    }

    return positions;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
