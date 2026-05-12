/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "string_id_pool.hpp"

#include <format>
#include <iostream>
#include <mutex>

namespace ixion { namespace detail {

string_id_t string_id_pool::append_string_unsafe(std::string_view s)
{
    string_id_t str_id{static_cast<string_id_t::value_type>(m_strings.size())};
    m_strings.push_back(std::string{s});
    s = m_strings.back();
    m_string_map.insert({s, str_id});
    return str_id;
}

string_id_t string_id_pool::append_string(std::string_view s)
{
    std::unique_lock lock(m_mtx);
    return append_string_unsafe(s);
}

string_id_t string_id_pool::add_string(std::string_view s)
{
    if (s.empty())
        // Never add an empty or invalid string.
        return empty_string_id;

    std::unique_lock lock(m_mtx);
    if (auto it = m_string_map.find(s); it != m_string_map.end())
        return it->second;

    return append_string_unsafe(s);
}

const std::string* string_id_pool::get_string(string_id_t identifier) const
{
    if (identifier == empty_string_id)
        return &m_empty_string;

    std::shared_lock lock(m_mtx);
    if (identifier.value >= m_strings.size())
        return nullptr;

    return &m_strings[identifier.value];
}

size_t string_id_pool::size() const
{
    std::shared_lock lock(m_mtx);
    return m_strings.size();
}

void string_id_pool::dump_strings() const
{
    std::shared_lock lock(m_mtx);
    std::cout << "string count: " << m_strings.size() << std::endl;
    string_id_t::value_type i = 0;
    for (const std::string& s : m_strings)
    {
        std::cout << "* " << string_id_t{i} << ": '" << s
             << "' (" << static_cast<const void*>(s.data()) << ")" << std::endl;
        ++i;
    }

    std::cout << "string map count: " << m_string_map.size() << std::endl;
    for (const auto& [key, value] : m_string_map)
    {
        std::cout << std::format("* key: '{}' ({}; {}), value: {}",
            key, static_cast<const void*>(key.data()), key.size(), value.value) << std::endl;
    }
}

string_id_t string_id_pool::get_identifier_from_string(std::string_view s) const
{
    std::shared_lock lock(m_mtx);
    string_map_type::const_iterator it = m_string_map.find(s);
    return it == m_string_map.end() ? empty_string_id : it->second;
}

}} // namespace ixion::detail

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
