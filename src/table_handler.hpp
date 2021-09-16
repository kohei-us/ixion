/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IXION_TABLE_HANDLER_HPP
#define IXION_TABLE_HANDLER_HPP

#include "ixion/interface/table_handler.hpp"
#include "ixion/types.hpp"
#include "ixion/address.hpp"

#include <vector>
#include <map>
#include <memory>

namespace ixion {

class table_handler : public iface::table_handler
{
public:

    /** single table entry */
    struct entry
    {
        string_id_t name;
        abs_range_t range;
        std::vector<string_id_t> columns;
        row_t totals_row_count;

        entry();
    };

    typedef std::map<string_id_t, std::unique_ptr<entry>> entries_type;

    virtual ~table_handler();

    virtual abs_range_t get_range(
        const abs_address_t& pos, string_id_t column_first, string_id_t column_last, table_areas_t areas) const;
    virtual abs_range_t get_range(
        string_id_t table, string_id_t column_first, string_id_t column_last, table_areas_t areas) const;

    void insert(std::unique_ptr<entry>& p);

private:
    abs_range_t get_column_range(
        const entry& e, string_id_t column_first, string_id_t column_last,
        table_areas_t areas) const;

    entries_type m_entries;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
