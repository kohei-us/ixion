/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_DOCUMENT_HPP
#define INCLUDED_IXION_DOCUMENT_HPP

#include "ixion/types.hpp"

#include <memory>

namespace ixion {

struct abs_address_t;

/**
 * Higher level document representation designed to handle both cell value
 * storage as well as formula cell calculations.
 */
class IXION_DLLPUBLIC document
{
    struct impl;
    std::unique_ptr<impl> mp_impl;
public:
    document();
    ~document();

    struct IXION_DLLPUBLIC cell_pos
    {
        enum class cp_type { string, address };
        cp_type type;

        union
        {
            struct { const char* str; size_t n; };
            struct { sheet_t sheet; row_t row; col_t column; };

        } value;

        cell_pos(const char* p);
        cell_pos(const char* p, size_t n);
        cell_pos(const abs_address_t& addr);
    };

    void append_sheet(std::string name);

    void set_numeric_cell(cell_pos pos, double val);

    double get_numeric_value(cell_pos pos) const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
