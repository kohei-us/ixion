/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "inline_string_pool.hpp"

#include <mutex>

namespace ixion { namespace detail {

std::string_view inline_string_pool::intern(std::string_view s)
{
    {
        std::shared_lock lock(m_mtx);
        if (auto it = m_intern_set.find(s); it != m_intern_set.end())
            return *it;
    }

    // re-check after acquiring the exclusive lock — another writer may have
    // inserted the same string while we released the shared lock
    std::unique_lock lock(m_mtx);
    if (auto it = m_intern_set.find(s); it != m_intern_set.end())
        return *it;

    // std::deque guarantees stable element addresses, so the view we hand
    // out remains valid for the pool's lifetime even as the deque grows
    m_strings.emplace_back(s);
    std::string_view view = m_strings.back();
    m_intern_set.insert(view);
    return view;
}

std::size_t inline_string_pool::size() const
{
    std::shared_lock lock(m_mtx);
    return m_strings.size();
}

}} // namespace ixion::detail

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
