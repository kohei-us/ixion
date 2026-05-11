/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>
#include <deque>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_set>

namespace ixion { namespace detail {

/**
 * String pool for inline-string cells.
 */
class inline_string_pool
{
    mutable std::shared_mutex m_mtx;
    std::deque<std::string> m_strings;
    std::unordered_set<std::string_view> m_intern_set;

public:
    std::string_view intern(std::string_view s);

    std::size_t size() const;
};

}} // namespace ixion::detail

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
