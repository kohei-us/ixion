/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ixion/types.hpp>

#include <deque>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ixion { namespace detail {

/**
 * String pool for ID-based string cells.  This pool is only to store values
 * for the ID-based string cells.
 */
class string_id_pool
{
    using string_pool_type = std::deque<std::string>;
    using string_map_type = std::unordered_map<std::string_view, string_id_t>;

    mutable std::shared_mutex m_mtx;
    string_pool_type m_strings;
    string_map_type m_string_map;
    std::string m_empty_string;

    string_id_t append_string_unsafe(std::string_view s);

public:
    string_id_t append_string(std::string_view s);
    string_id_t add_string(std::string_view s);
    const std::string* get_string(string_id_t identifier) const;

    size_t size() const;
    void dump_strings() const;
    string_id_t get_identifier_from_string(std::string_view s) const;
};

}} // namespace ixion::detail

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
