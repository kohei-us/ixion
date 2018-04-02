/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "grouped_ranges.hpp"

#include "ixion/global.hpp"
#include "ixion/model_context.hpp"

namespace ixion {

grouped_range_error::grouped_range_error(const std::string& msg) : general_error(msg) {}
grouped_range_error::~grouped_range_error() throw() {}

grouped_ranges::grouped_ranges() {}
grouped_ranges::~grouped_ranges() {}

void grouped_ranges::add(sheet_t sheet, const abs_rc_range_t& range, uintptr_t identity)
{
    sheet_type& store = fetch_sheet_store(sheet);

    // TODO : ensure that the new range will not overlap with any of the
    // existing ranges.
    store.ranges.insert(range.first.column, range.first.row, range.last.column+1, range.last.row+1, identity);
    store.map.insert(std::make_pair(identity, range));
}

void grouped_ranges::remove(sheet_t sheet, uintptr_t identity)
{
    if (size_t(sheet) >= m_sheets.size())
        throw grouped_range_error("No grouped ranges stored on specified sheet.");

    sheet_type& store = *m_sheets[sheet];
    store.ranges.remove(identity);
    auto it = store.map.find(identity);
    if (it != store.map.end())
        store.map.erase(it);
}

abs_rc_address_t grouped_ranges::move_to_origin(sheet_t sheet, const abs_rc_address_t& pos) const
{
    if (size_t(sheet) >= m_sheets.size())
        return pos;

    const sheet_type& store = *m_sheets[sheet];
    ranges_type::search_result res = store.ranges.search(pos.column, pos.row); // x then y
    size_t n = res.size();
    assert(n <= 1); // there should never be an overlap.
    if (!n)
        // There is no group range at the specified position.
        return pos;

    uintptr_t identity = *res.begin();
    auto it = store.map.find(identity);
    if (it == store.map.end())
        return pos;

    const abs_rc_range_t& gr = it->second;
    assert(gr.first.column <= pos.column && pos.column <= gr.last.column);
    assert(gr.first.row <= pos.row && pos.row <= gr.last.row);

    return gr.first;
}

grouped_ranges::sheet_type& grouped_ranges::fetch_sheet_store(sheet_t sheet)
{
    if (size_t(sheet) < m_sheets.size())
        return *m_sheets[sheet];

    // Add enough sheet stores so that the specified sheet index becomes
    // within range.

    size_t new_size = sheet + 1;
    size_t added = new_size - m_sheets.size();
    m_sheets.reserve(new_size);

    for (size_t i = 0; i < added; ++i)
        m_sheets.emplace_back(ixion::make_unique<sheet_type>());

    assert(size_t(sheet) < m_sheets.size());
    return *m_sheets[sheet];
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
