/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "ixion/interface/table_handler.hpp"
#include "ixion/types.hpp"
#include "ixion/address.hpp"

#include <vector>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace ixion {

class table_handler : public iface::table_handler
{
public:

    /** single table entry */
    struct entry
    {
        std::string name;
        abs_range_t range;
        std::vector<std::string> columns;
        row_t totals_row_count;

        entry();
    };

    typedef std::map<std::string, std::unique_ptr<entry>, std::less<>> entries_type;

    virtual ~table_handler();

    virtual abs_range_t get_range(
        const abs_address_t& pos, std::string_view column_first, std::string_view column_last,
        table_areas_t areas) const;
    virtual abs_range_t get_range(
        std::string_view table, std::string_view column_first, std::string_view column_last,
        table_areas_t areas) const;

    void insert(std::unique_ptr<entry>& p);

private:
    abs_range_t get_column_range(
        const entry& e, std::string_view column_first, std::string_view column_last,
        table_areas_t areas) const;

    entries_type m_entries;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
