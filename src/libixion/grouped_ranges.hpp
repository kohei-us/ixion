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

#include <mdds/flat_segment_tree.hpp>
#include <memory>
#include <vector>

namespace ixion {

class model_context;

class IXION_DLLPUBLIC grouped_range_error : public general_error
{
public:
    grouped_range_error(const std::string& msg);
    virtual ~grouped_range_error() throw() override;
};

class grouped_ranges
{
    using ranges_type = mdds::flat_segment_tree<rc_t, uintptr_t>;

    struct sheet_type
    {
        mutable ranges_type rows;
        mutable ranges_type columns;

        sheet_type(sheet_size_t ss);

        void build_trees() const;
    };

    const model_context& m_cxt;
    std::vector<std::unique_ptr<sheet_type>> m_sheets;

public:
    grouped_ranges(const model_context& cxt);
    ~grouped_ranges();

    void add(sheet_t sheet, const abs_rc_range_t& range, uintptr_t identifier);

    uintptr_t remove(sheet_t sheet, const abs_rc_range_t& range);

    abs_rc_address_t move_to_origin(sheet_t sheet, const abs_rc_address_t& pos) const;

private:
    sheet_type& fetch_sheet_store(sheet_t sheet);
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
