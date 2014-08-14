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
#include <boost/ptr_container/ptr_map.hpp>

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

    typedef boost::ptr_map<string_id_t, entry> entries_type;

    virtual ~table_handler();

    void insert(entry* p);

private:
    entries_type m_entries;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
