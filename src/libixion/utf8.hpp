/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>
#include <string>

namespace ixion { namespace detail {

/**
 * Obtains the positions of the first bytes of the unicode characters.
 *
 * @param s input string encoded in utf-8.
 *
 * @return a sequence of the positions.  The size of the sequence equals the
 *         logical length of the utf-8 string.
 */
std::vector<std::size_t> calc_utf8_byte_positions(const std::string& s);

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
