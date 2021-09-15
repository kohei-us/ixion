/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_GLOBAL_HPP
#define INCLUDED_IXION_GLOBAL_HPP

#include "types.hpp"
#include "env.hpp"

#include <memory>
#include <string>

namespace ixion {

/**
 * Get current time in seconds since epoch.  Note that the value
 * representing a time may differ from platform to platform.  Use this
 * value only to measure relative time.
 *
 * @return current time in seconds since epoch.
 */
IXION_DLLPUBLIC double get_current_time();

IXION_DLLPUBLIC double to_double(std::string_view s);

IXION_DLLPUBLIC bool to_bool(const char* p, size_t n);

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
