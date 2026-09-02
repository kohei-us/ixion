/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/sheet_view.hpp>
#include <ixion/address.hpp>
#include <ixion/table.hpp>

#include "grid_dumper.hpp"
#include "model_context_impl.hpp"
#include "sheet_sort.hpp"
#include "sheet_store.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <format>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace ixion {

namespace {

/** One sort applied to a view, for refresh() to replay. */
struct sort_action
{
    abs_rc_range_t range;

    // permutation the engine returned for this sort, in the view rows of
    // that time
    std::vector<row_t> rows;
};

} // anonymous namespace

struct sheet_view::impl
{
    const detail::model_context_impl& cxt;
    sheet_t sheet;
    std::string name;

    // Snapshot of the base sheet taken when the view was created.  It is cloned
    // from the base sheet, so if COW is enabled this view and the base sheet
    // share their content.
    detail::sheet_store store;

    // Row mapping between this view and the base sheet, covering all the rows
    // of the sheet.  Both stay empty, meaning identity, until the first sort.
    std::vector<row_t> base_rows; // view row -> base row
    std::vector<row_t> view_rows; // base row -> view row

    // sorts applied to this view, in the order they were applied
    std::vector<sort_action> sorts;

    impl(const detail::model_context_impl& _cxt, sheet_t _sheet, std::string _name,
         const detail::sheet_store& base) :
        cxt(_cxt), sheet(_sheet), name(std::move(_name)), store(base.clone()) {}

    column_store_t::const_position_type get_cell_position(const abs_rc_address_t& pos) const
    {
        return store.at(pos.column).position(pos.row);
    }

    row_t get_row_count() const
    {
        // NB: all columns of a sheet have the same size
        return store.size() ? row_t(store[0].size()) : 0;
    }

    /**
     * Fold the permutation of a sort into the row mapping.
     *
     * @param row1 First row of the sorted range.
     * @param sorted_rows Permutation returned by the sort: element i holds
     *                    the view row, before the sort, of the cells now at
     *                    row1 + i.
     */
    void apply_permutation(row_t row1, const std::vector<row_t>& sorted_rows)
    {
        if (base_rows.empty())
        {
            base_rows.resize(get_row_count());
            std::iota(base_rows.begin(), base_rows.end(), 0);
        }

        std::vector<row_t> prev_base_rows = base_rows;

        for (std::size_t i = 0; i < sorted_rows.size(); ++i)
            base_rows[row1 + i] = prev_base_rows[sorted_rows[i]];

        // rebuild the inverse mapping
        view_rows.resize(base_rows.size());

        for (std::size_t view_row = 0; view_row < base_rows.size(); ++view_row)
            view_rows[base_rows[view_row]] = view_row;
    }
};

sheet_view::sheet_view(
    const detail::model_context_impl& cxt, sheet_t sheet, std::string name,
    const detail::sheet_store& base) :
    mp_impl(std::make_unique<impl>(cxt, sheet, std::move(name), base))
{
}

sheet_view::~sheet_view() = default;

sheet_t sheet_view::get_sheet() const
{
    return mp_impl->sheet;
}

std::string_view sheet_view::get_name() const
{
    return mp_impl->name;
}

cell_t sheet_view::get_celltype(const abs_rc_address_t& pos) const
{
    auto cell_pos = mp_impl->get_cell_position(pos);
    return detail::to_celltype(cell_pos.first->type);
}

double sheet_view::get_numeric_value(const abs_rc_address_t& pos) const
{
    auto cell_pos = mp_impl->get_cell_position(pos);
    return mp_impl->cxt.get_numeric_value(cell_pos);
}

bool sheet_view::get_boolean_value(const abs_rc_address_t& pos) const
{
    auto cell_pos = mp_impl->get_cell_position(pos);
    return mp_impl->cxt.get_boolean_value(cell_pos);
}

std::string_view sheet_view::get_string_value(const abs_rc_address_t& pos) const
{
    auto cell_pos = mp_impl->get_cell_position(pos);
    return mp_impl->cxt.get_string_value(cell_pos);
}

const formula_cell* sheet_view::get_formula_cell(const abs_rc_address_t& pos) const
{
    auto cell_pos = mp_impl->get_cell_position(pos);
    return detail::model_context_impl::get_formula_cell(cell_pos);
}

void sheet_view::sort(const abs_rc_range_t& range, const sort_keys_t& keys)
{
    std::vector<row_t> sorted_rows =
        detail::sort_range(mp_impl->cxt.get_parent(), mp_impl->store, range, keys);

    if (!std::is_sorted(sorted_rows.begin(), sorted_rows.end()))
    {
        // remember the sort for refresh() to replay
        mp_impl->sorts.push_back({range, sorted_rows});
    }

    mp_impl->apply_permutation(range.first.row, sorted_rows);
}

row_t sheet_view::to_base_row(row_t view_row) const
{
    if (mp_impl->base_rows.empty())
        return view_row;

    return mp_impl->base_rows.at(view_row);
}

row_t sheet_view::to_view_row(row_t base_row) const
{
    if (mp_impl->view_rows.empty())
        return base_row;

    return mp_impl->view_rows.at(base_row);
}

void sheet_view::sort_table(std::string_view table_name, std::string_view column, sort_order_t order)
{
    const table_t* tab = mp_impl->cxt.get_table(table_name);
    if (!tab)
        throw std::invalid_argument(std::format("no table named '{}'", table_name));

    if (tab->sheet != mp_impl->sheet)
        throw std::invalid_argument(
            std::format("table '{}' is not on the base sheet of this view", table_name));

    auto it = std::ranges::find(tab->columns, column);
    if (it == tab->columns.end())
        throw std::invalid_argument(
            std::format("table '{}' has no column named '{}'", table_name, column));

    col_t key_column = tab->range.first.column + std::distance(tab->columns.begin(), it);

    // all the columns of the data area, excluding the header and totals rows
    abs_range_t data_range = mp_impl->cxt.get_table_range(table_name, {}, {}, table_area_data);
    if (!data_range.valid())
        // the table has no data rows
        return;

    sort(data_range, {{key_column, order}});
}

void sheet_view::refresh()
{
    const detail::sheet_store* base = mp_impl->cxt.fetch_sheet(mp_impl->sheet);
    assert(base);

    // Take a fresh snapshot of the base sheet, then re-apply the sort order of
    // this view.
    mp_impl->store = base->clone();

    for (const sort_action& action : mp_impl->sorts)
        detail::reorder_range(mp_impl->store, action.range, action.rows);
}

abs_rc_range_t sheet_view::get_data_range() const
{
    return mp_impl->store.get_data_range();
}

void sheet_view::dump(
    std::ostream& os, sheet_dump_mode_t mode, const formula_name_resolver* resolver) const
{
    detail::grid_dumper(mp_impl->cxt.get_parent(), resolver).dump(os, *this, mode);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
