/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_GROUPED_RANGES_HPP
#define INCLUDED_IXION_GROUPED_RANGES_HPP

#include "ixion/address.hpp"
#include "ixion/exceptions.hpp"

#include <mdds/rectangle_set.hpp>
#include <memory>
#include <vector>
#include <unordered_map>

namespace ixion {

class IXION_DLLPUBLIC grouped_range_error : public general_error
{
public:
    grouped_range_error(const std::string& msg);
    virtual ~grouped_range_error() throw() override;
};

class grouped_ranges
{
    using ranges_type = mdds::rectangle_set<rc_t, uintptr_t>;
    using id_to_range_map_type = std::unordered_map<uintptr_t, abs_rc_range_t>;

    struct sheet_type
    {
        mutable ranges_type ranges;
        id_to_range_map_type map;
    };

    std::vector<std::unique_ptr<sheet_type>> m_sheets;

public:
    grouped_ranges();
    ~grouped_ranges();

    void add(sheet_t sheet, const abs_rc_range_t& range, uintptr_t identity);

    void remove(sheet_t sheet, uintptr_t identity);

    abs_rc_address_t move_to_origin(sheet_t sheet, const abs_rc_address_t& pos) const;

private:
    sheet_type& fetch_sheet_store(sheet_t sheet);
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
