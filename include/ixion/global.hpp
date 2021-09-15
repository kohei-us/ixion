/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_GLOBAL_HPP
#define INCLUDED_IXION_GLOBAL_HPP

#include "ixion/types.hpp"
#include "ixion/env.hpp"

#include <memory>
#include <string>

namespace ixion {

class IXION_DLLPUBLIC global
{
public:
    /**
     * Get current time in seconds since epoch.  Note that the value
     * representing a time may differ from platform to platform.  Use this
     * value only to measure relative time.
     *
     * @return current time in seconds since epoch.
     */
    static double get_current_time();

    static double to_double(const char* p, size_t n);

    static bool to_bool(const char* p, size_t n);
private:
    global();
    global(const global& r);
    ~global();
};

// ============================================================================


}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
