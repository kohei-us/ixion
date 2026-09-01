/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "sheet_sort.hpp"
#include "calc_status.hpp"
#include "formula_groups.hpp"
#include "sheet_store.hpp"

#include <ixion/address.hpp>
#include <ixion/cell.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/matrix.hpp>
#include <ixion/model_context.hpp>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <iterator>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace ixion { namespace detail {

namespace {

constexpr auto wait_policy = formula_result_wait_policy_t::throw_exception;

/** The values encode the ascending cross-type order. */
enum class sort_value_type : std::uint8_t { numeric, string, boolean_false, boolean_true, error, empty };

/**
 * Cell value reduced to what the key comparison needs.  An empty value
 * sorts last in both directions.  The string is a view into storage that
 * stays untouched until the cells get moved: the string pool, the memory
 * an inline string references, or a cached formula result.
 */
struct sort_value
{
    sort_value_type type = sort_value_type::empty;
    double numeric = 0.0;
    std::string_view str;
};

sort_value to_sort_value(const matrix::element& elem)
{
    sort_value v;

    switch (elem.type)
    {
        case matrix::element_type::numeric:
            v.type = sort_value_type::numeric;
            v.numeric = std::get<double>(elem.value);
            break;
        case matrix::element_type::boolean:
            v.type = std::get<bool>(elem.value) ? sort_value_type::boolean_true : sort_value_type::boolean_false;
            break;
        case matrix::element_type::string:
            v.type = sort_value_type::string;
            v.str = std::get<std::string_view>(elem.value);
            break;
        case matrix::element_type::error:
            v.type = sort_value_type::error;
            break;
        case matrix::element_type::empty:
            break;
    }

    return v;
}

sort_value to_sort_value(const formula_cell& cell, row_t row)
{
    // The raw result reference keeps the string values viewable; the sliced
    // result of get_result_cache() would be a temporary.
    const formula_result* res;

    try
    {
        res = &cell.get_raw_result_cache(wait_policy);
    }
    catch (const formula_error&)
    {
        // An uncalculated formula cell displays nothing; sort it like an
        // empty cell.
        return {};
    }

    sort_value v;

    switch (res->get_type())
    {
        case formula_result::result_type::boolean:
        {
            v.type = res->get_boolean() ? sort_value_type::boolean_true : sort_value_type::boolean_false;
            break;
        }
        case formula_result::result_type::value:
        {
            v.type = sort_value_type::numeric;
            v.numeric = res->get_value();
            break;
        }
        case formula_result::result_type::string:
        {
            v.type = sort_value_type::string;
            v.str = res->get_string();
            break;
        }
        case formula_result::result_type::error:
        {
            v.type = sort_value_type::error;
            break;
        }
        case formula_result::result_type::matrix:
        {
            const matrix& m = res->get_matrix();

            if (!cell.get_group_properties().grouped)
                // A non-grouped cell displays the top-left value of its
                // matrix result.
                return to_sort_value(m.get(0, 0));

            // A grouped cell owns the value at its offset from the top of
            // the group.
            row_t offset = row - cell.get_parent_position(abs_address_t(0, row, 0)).row;

            if (offset >= row_t(m.row_size()))
            {
                // offset goes beyond the group size
                v.type = sort_value_type::error;
                break;
            }

            return to_sort_value(m.get(offset, 0));
        }
    }

    return v;
}

bool sort_value_less(const sort_value& a, const sort_value& b, bool ascending)
{
    if (a.type == sort_value_type::empty)
        // An empty cell always comes last in either direction.
        return false;

    if (b.type == sort_value_type::empty)
        // See above.
        return true;

    if (a.type != b.type)
        return ascending ? a.type < b.type : b.type < a.type;

    switch (a.type)
    {
        case sort_value_type::numeric:
            return ascending ? a.numeric < b.numeric : b.numeric < a.numeric;
        case sort_value_type::string:
            return ascending ? a.str < b.str : b.str < a.str;
        default:
            return false;
    }
}

/**
 * Walk down the blocks of a column one block at a time within a row range,
 * invoking func once per block.  The first and last blocks may be referenced
 * partially if the row boundaries occur mid-block.
 *
 * @param col Column whose blocks to walk.
 * @param row1 First row of the walked range.
 * @param row2 Last row of the walked range, inclusive.
 * @param func Invoked with the block, the offset of the walked segment
 *             within it, the segment length, and the row position of the
 *             segment's first element.
 */
template<typename FuncT>
void walk_blocks(const column_store_t& col, row_t row1, row_t row2, FuncT func)
{
    auto pos = col.position(row1);
    auto it = pos.first;
    std::size_t offset = pos.second;

    for (row_t row = row1; row <= row2; ++it, offset = 0)
    {
        auto n = std::min<std::size_t>(it->size - offset, row2 - row + 1);
        func(*it, offset, n, row);
        row += n;
    }
}

/**
 * Extract the values of a key column into a plain vector, one entry per
 * row, so that the sort comparison never reads from the column store.
 *
 * @param cxt Model context whose string pool resolves the string values.
 * @param col Column to extract the key values from.
 * @param row1 First row of the extracted rows.
 * @param row2 Last row of the extracted rows, inclusive.
 */
std::vector<sort_value> fetch_key_column_values(
    const model_context& cxt, const column_store_t& col, row_t row1, row_t row2)
{
    std::vector<sort_value> values;
    values.reserve(row2 - row1 + 1);

    // walk down each block and collect cell values.
    walk_blocks(col, row1, row2, [&](const auto& blk, std::size_t offset, std::size_t n, row_t row)
    {
        for (std::size_t i = offset; i < offset + n; ++i)
        {
            sort_value v;

            switch (blk.type)
            {
                case element_type_boolean:
                {
                    auto bit = boolean_element_block::cbegin(*blk.data);
                    std::advance(bit, i);
                    v.type = *bit ? sort_value_type::boolean_true : sort_value_type::boolean_false;
                    break;
                }
                case element_type_numeric:
                {
                    v.type = sort_value_type::numeric;
                    v.numeric = numeric_element_block::at(*blk.data, i);
                    break;
                }
                case element_type_string:
                {
                    v.type = sort_value_type::string;
                    const std::string* p = cxt.get_string(
                        string_id_t{string_element_block::at(*blk.data, i)});
                    if (p)
                        v.str = *p;
                    break;
                }
                case element_type_inline_string:
                {
                    v.type = sort_value_type::string;
                    v.str = inline_string_element_block::at(*blk.data, i).view;
                    break;
                }
                case element_type_formula:
                {
                    v = to_sort_value(
                        *formula_element_block::at(*blk.data, i),
                        row + static_cast<row_t>(i - offset)
                    );
                    break;
                }
                default:;
            }

            values.push_back(std::move(v));
        }
    });

    return values;
}

/**
 * Create a standalone formula cell from a grouped cell.
 */
formula_cell* make_single_cell(const formula_cell& src)
{
    auto cell = std::make_unique<formula_cell>(src.get_tokens());

    try
    {
        cell->set_result_cache(src.get_result_cache(wait_policy));
    }
    catch (const formula_error&)
    {
        // No cached result to carry over.
    }

    return cell.release();
}

/**
 * Content of a single cell in transit between two rows.  A formula cell
 * stays owned by its column until the release_range() call that follows
 * the extraction.
 */
using cell_slot = std::variant<
    std::monostate, bool, double, std::uint32_t, string_view_store, formula_cell*>;

/**
 * Get the cached result of a standalone formula cell, or null when the
 * cell has no cached result or the result is a matrix.
 */
const formula_result* fetch_usable_result(const formula_cell& cell)
{
    try
    {
        const formula_result& res = cell.get_raw_result_cache(wait_policy);

        if (res.get_type() == formula_result::result_type::matrix)
            // a matrix result cannot fill a slot of a group result matrix
            return nullptr;

        return &res;
    }
    catch (const formula_error&)
    {
        return nullptr;
    }
}

/**
 * Regroup maximal runs of adjacent standalone formula cells sharing one
 * token store within the given rows.  A cell with no cached result, or
 * with a matrix result, cannot join a run since its result cannot fill
 * its slot of the new group result matrix.
 */
void regroup_column(column_store_t& col, row_t row1, row_t row2)
{
    struct regroup_run
    {
        row_t row;
        formula_tokens_store_ptr_t tokens;
        // member results, pointing into the cells that get replaced below
        std::vector<const formula_result*> results;
    };

    std::vector<regroup_run> runs;

    // Collect the runs first; replacing the cells below invalidates the
    // entries.
    std::vector<formula_group_entry> entries = get_formula_groups(col);
    std::size_t i = 0;

    while (i < entries.size())
    {
        const formula_group_entry& group = entries[i];

        if (group.size != 1 || group.row < row1 || row2 < group.row)
        {
            // this group survived intact, or outside the sorted range - skip it
            ++i;
            continue;
        }

        const formula_result* head_res = fetch_usable_result(*group.cells[0]);

        if (!head_res)
        {
            // this cell cannot join any run
            ++i;
            continue;
        }

        const formula_tokens_store_ptr_t& ts = group.cells[0]->get_tokens();

        std::vector<const formula_result*> results;
        results.push_back(head_res);

        std::size_t run_end = i + 1;

        // extend the run for as long as the next entry can join it
        while (run_end < entries.size())
        {
            const formula_group_entry& e = entries[run_end];

            if (e.size != 1)
                // an intact group cannot get merged into a new group
                break;

            if (row2 < e.row)
                // the run must not grow past the scanned rows
                break;

            if (e.row != entries[run_end-1].row + 1)
                // not the next row of the previous row
                break;

            if (e.cells[0]->get_tokens().get() != ts.get())
                // the cell does not share the same token as the previous one
                break;

            const formula_result* res = fetch_usable_result(*e.cells[0]);

            if (!res)
                // the cell has no result to fill its slot of the group result
                break;

            results.push_back(res);
            ++run_end;
        }

        if (run_end - i > 1)
            // mark this run for re-grouping
            runs.push_back({group.row, ts, std::move(results)});

        i = run_end;
    }

    mdds::mtv::position_hint hint;

    for (const regroup_run& run : runs)
    {
        row_t size = run.results.size();
        matrix res_mtx(size, 1);

        for (row_t k = 0; k < size; ++k)
        {
            const formula_result& res = *run.results[k];

            switch (res.get_type())
            {
                case formula_result::result_type::boolean:
                    res_mtx.set(k, 0, res.get_boolean());
                    break;
                case formula_result::result_type::value:
                    res_mtx.set(k, 0, res.get_value());
                    break;
                case formula_result::result_type::string:
                    res_mtx.set(k, 0, res.get_string());
                    break;
                case formula_result::result_type::error:
                    res_mtx.set(k, 0, res.get_error());
                    break;
                case formula_result::result_type::matrix:
                    assert(!"matrix results are excluded when collecting the runs");
                    break;
            }
        }

        calc_status_ptr_t cs(new calc_status({size, 1}));
        cs->result = std::make_unique<formula_result>(std::move(res_mtx));

        std::vector<formula_cell*> cells;
        cells.reserve(size);

        for (row_t k = 0; k < size; ++k)
            cells.push_back(new formula_cell(k, 0, cs, run.tokens));

        // set the grouped formula cells to overwrite the individual cells
        hint = col.set(hint, run.row, cells.begin(), cells.end());
    }
}

/**
 * Sort the rows of a range in phases: determine the new row order once,
 * then for each column find the formula groups the sort breaks, ungroup
 * them, move the cells to their sorted positions, and regroup afterwards.
 * A pre-computed row order can get applied in place of the sorting phase.
 */
class range_sorter
{
    /** Formula group to get ungrouped before the move. */
    struct group_span
    {
        row_t row;
        row_t size;
    };

    /** Formula group changes required in one column. */
    struct group_changes
    {
        std::vector<group_span> to_ungroup;

        // Rows to scan for regrouping afterwards. A group crossing the boundary
        // of the sorted range widens it to cover its members outside the range.
        row_t regroup_row1;
        row_t regroup_row2;
    };

    // Model context and sort keys; they stay null when a pre-computed row
    // order gets applied instead of sorting.
    const model_context* m_cxt;
    const sort_keys_t* m_keys;

    sheet_store& m_store;

    row_t m_row1;
    row_t m_row2;
    col_t m_col1;
    col_t m_col2;
    row_t m_n_rows;

    // permutation of the sorted rows: element i stores the source row that
    // lands at row1 + i after the sort
    std::vector<row_t> m_sorted_rows;

    // dest_rows stores the destination position of a source row within sorted range,
    // it stores absolute row IDs.
    std::vector<row_t> m_dest_rows;

public:
    range_sorter(
        const model_context& cxt, sheet_store& store,
        const abs_rc_range_t& range, const sort_keys_t& keys);

    range_sorter(sheet_store& store, const abs_rc_range_t& range);

    std::vector<row_t> sort();

    /**
     * Move the rows of the range into a pre-computed order instead of sorting.
     *
     * @param sorted_rows Row order to apply, in the same form as the return
     *                    value of sort(): element i holds the source row of the
     *                    cells that end up at row1 + i.  It must be a
     *                    permutation of the rows of the range.
     */
    void apply(std::vector<row_t> sorted_rows);

private:
    /**
     * Determine the new row order after sorting by the key columns.
     *
     * Note that this does NOT apply sorting to the store.
     *
     * On return, m_sorted_rows stores the source row landing at each row of the
     * range in sorted order, and m_dest_rows stores the inverse: the row each
     * source row lands at.  Both store absolute row positions and are indexed
     * by row offset within the range.
     */
    void compute_row_order();

    /**
     * Build the inverse of m_sorted_rows: the destination row of each source
     * row of the range, indexed by source row offset.
     */
    void build_dest_rows();
    void move_rows();

    /**
     * @return Row position of a source row after the sort.
     */
    row_t dest_row_of(row_t row) const;

    void sort_column(col_t col_pos);

    /**
     * Find the formula groups the sort breaks, which need to get ungrouped
     * before the move, and the rows to scan for regrouping afterwards.
     */
    group_changes collect_group_changes(const column_store_t& col) const;

    /**
     * Replace the members of the given groups with individual cells.
     */
    void ungroup(column_store_t& col, const std::vector<group_span>& to_ungroup) const;

    /**
     * Collect all cells in the sorted range into slots indexed by source row
     * offset.
     */
    std::vector<cell_slot> extract_cells(const column_store_t& col) const;

    /**
     * Write the cells back in their sorted order, one ranged set per run of
     * cells of the same type.
     */
    void place_cells_sorted(column_store_t& col, std::vector<cell_slot>& slots) const;
};

range_sorter::range_sorter(
    const model_context& cxt, sheet_store& store,
    const abs_rc_range_t& range, const sort_keys_t& keys) :
    m_cxt(&cxt), m_keys(&keys), m_store(store),
    m_row1(range.first.row), m_row2(range.last.row),
    m_col1(range.first.column), m_col2(range.last.column),
    m_n_rows(m_row2 - m_row1 + 1)
{
}

range_sorter::range_sorter(sheet_store& store, const abs_rc_range_t& range) :
    m_cxt(nullptr), m_keys(nullptr), m_store(store),
    m_row1(range.first.row), m_row2(range.last.row),
    m_col1(range.first.column), m_col2(range.last.column),
    m_n_rows(m_row2 - m_row1 + 1)
{
}

std::vector<row_t> range_sorter::sort()
{
    compute_row_order();

    if (std::is_sorted(m_sorted_rows.begin(), m_sorted_rows.end()))
        // Nothing moves; leave the columns untouched to preserve any
        // copy-on-write sharing.
        return m_sorted_rows;

    move_rows();

    return m_sorted_rows;
}

void range_sorter::apply(std::vector<row_t> sorted_rows)
{
    assert(row_t(sorted_rows.size()) == m_n_rows);
    m_sorted_rows = std::move(sorted_rows);

    if (std::is_sorted(m_sorted_rows.begin(), m_sorted_rows.end()))
        // Nothing moves; leave the columns untouched to preserve any
        // copy-on-write sharing.
        return;

    build_dest_rows();
    move_rows();
}

void range_sorter::move_rows()
{
    for (col_t col_pos = m_col1; col_pos <= m_col2; ++col_pos)
        sort_column(col_pos);
}

void range_sorter::compute_row_order()
{
    assert(m_cxt && m_keys);

    const sheet_store& cstore = m_store;
    const sort_keys_t& keys = *m_keys;

    // Extract the key column values and argsort the rows.
    std::vector<std::vector<sort_value>> key_col_values;
    key_col_values.reserve(keys.size());

    for (const sort_key_t& key : keys)
        key_col_values.push_back(
            fetch_key_column_values(*m_cxt, cstore[key.column], m_row1, m_row2));

    // write incremental sequence starting at row1
    m_sorted_rows.resize(m_n_rows);
    std::iota(m_sorted_rows.begin(), m_sorted_rows.end(), m_row1);

    // do the sort
    auto row_less = [&keys, &key_col_values, this](row_t a, row_t b)
    {
        for (std::size_t k = 0; k < keys.size(); ++k)
        {
            const sort_value& va = key_col_values[k][a - m_row1];
            const sort_value& vb = key_col_values[k][b - m_row1];

            if (sort_value_less(va, vb, keys[k].ascending))
                return true; // a < b

            if (sort_value_less(vb, va, keys[k].ascending))
                return false; // b < a

            // tie, move to the next key column
        }

        return false;
    };

    std::stable_sort(m_sorted_rows.begin(), m_sorted_rows.end(), row_less);

    // at this point, 'm_sorted_rows' stores the permutation of the sorted rows

    build_dest_rows();
}

void range_sorter::build_dest_rows()
{
    m_dest_rows.resize(m_n_rows);

    for (row_t i = 0; i < m_n_rows; ++i)
    {
        // from absolute row ID (m_sorted_rows[i]) to offset within sorted range (pos)
        auto pos = m_sorted_rows[i] - m_row1;
        m_dest_rows[pos] = m_row1 + i;
    }
}

row_t range_sorter::dest_row_of(row_t row) const
{
    return (m_row1 <= row && row <= m_row2) ? m_dest_rows[row - m_row1] : row;
}

void range_sorter::sort_column(col_t col_pos)
{
    column_store_t& col = m_store[col_pos];

    group_changes changes = collect_group_changes(col);

    col.detach();

    // Ungroup the affected groups first before applying the sort
    ungroup(col, changes.to_ungroup);

    std::vector<cell_slot> slots = extract_cells(col);

    // The slots now own the formula cells fetched above; they leak if
    // the write-back below throws before placing them all back.
    col.release_range(m_row1, m_row2);

    place_cells_sorted(col, slots);

    regroup_column(col, changes.regroup_row1, changes.regroup_row2);

    // The moves invalidated whatever hint the column had.
    m_store.get_pos_hint(col_pos) = mdds::mtv::position_hint{};
}

range_sorter::group_changes range_sorter::collect_group_changes(const column_store_t& col) const
{
    group_changes changes;
    changes.regroup_row1 = m_row1;
    changes.regroup_row2 = m_row2;

    for (const formula_group_entry& e : get_formula_groups(col))
    {
        if (e.size < 2 || m_row2 < e.row || e.row + e.size - 1 < m_row1)
            continue; // skip non-grouped cells and groups outside the sorted range

        // check if this group stays unchanged after the sort even if its
        // position shifts.
        bool intact = true;

        for (row_t k = 1; k < e.size && intact; ++k)
            intact = dest_row_of(e.row + k) == dest_row_of(e.row) + k;

        if (!intact)
        {
            changes.to_ungroup.push_back({e.row, e.size});
            changes.regroup_row1 = std::min(changes.regroup_row1, e.row);
            changes.regroup_row2 = std::max(changes.regroup_row2, e.row + e.size - 1);
        }
    }

    return changes;
}

void range_sorter::ungroup(column_store_t& col, const std::vector<group_span>& to_ungroup) const
{
    mdds::mtv::position_hint hint;

    for (const group_span& g : to_ungroup)
    {
        // The members of a group are contiguous within one block.
        auto pos = std::as_const(col).position(g.row);
        assert(pos.first->type == element_type_formula);
        formula_cell* const* cells =
            &formula_element_block::at(*pos.first->data, pos.second);

        std::vector<formula_cell*> single_cells;
        single_cells.reserve(g.size);

        for (row_t k = 0; k < g.size; ++k)
            single_cells.push_back(make_single_cell(*cells[k]));

        // overwrite the group with the individual cells
        hint = col.set(hint, g.row, single_cells.begin(), single_cells.end());
    }
}

std::vector<cell_slot> range_sorter::extract_cells(const column_store_t& col) const
{
    std::vector<cell_slot> slots(m_n_rows);

    walk_blocks(col, m_row1, m_row2,
        [&slots, this](const auto& blk, std::size_t offset, std::size_t n, row_t row)
    {
        for (std::size_t i = 0; i < n; ++i)
        {
            cell_slot& slot = slots[row - m_row1 + i];

            switch (blk.type)
            {
                case element_type_boolean:
                {
                    auto bit = boolean_element_block::cbegin(*blk.data);
                    std::advance(bit, offset + i);
                    slot = bool(*bit);
                    break;
                }
                case element_type_numeric:
                {
                    slot = numeric_element_block::at(*blk.data, offset + i);
                    break;
                }
                case element_type_string:
                {
                    slot = string_element_block::at(*blk.data, offset + i);
                    break;
                }
                case element_type_inline_string:
                {
                    slot = inline_string_element_block::at(*blk.data, offset + i);
                    break;
                }
                case element_type_formula:
                {
                    slot = formula_element_block::at(*blk.data, offset + i);
                    break;
                }
                default:;
            }
        }
    });

    return slots;
}

void range_sorter::place_cells_sorted(column_store_t& col, std::vector<cell_slot>& slots) const
{
    // returns the slot holding the source cell given the destination row offset (0-based)
    auto src_slot = [&slots, this](row_t dst) -> cell_slot&
    {
        auto src_row = m_sorted_rows[dst];
        return slots[src_row - m_row1];
    };

    mdds::mtv::position_hint hint;

    for (row_t run_start = 0; run_start < m_n_rows;)
    {
        row_t run_end = run_start + 1;

        // determine the length of the run with the same type after the sort
        while (run_end < m_n_rows && src_slot(run_end).index() == src_slot(run_start).index())
            ++run_end;

        row_t row = m_row1 + run_start;

        if (std::holds_alternative<bool>(src_slot(run_start)))
        {
            std::deque<bool> buf;
            for (row_t k = run_start; k < run_end; ++k)
                buf.push_back(std::get<bool>(src_slot(k)));

            hint = col.set(hint, row, buf.begin(), buf.end());
        }
        else if (std::holds_alternative<double>(src_slot(run_start)))
        {
            std::vector<double> buf;
            buf.reserve(run_end - run_start);

            for (row_t k = run_start; k < run_end; ++k)
                buf.push_back(std::get<double>(src_slot(k)));

            hint = col.set(hint, row, buf.begin(), buf.end());
        }
        else if (std::holds_alternative<std::uint32_t>(src_slot(run_start)))
        {
            std::vector<std::uint32_t> buf;
            buf.reserve(run_end - run_start);

            for (row_t k = run_start; k < run_end; ++k)
                buf.push_back(std::get<std::uint32_t>(src_slot(k)));

            hint = col.set(hint, row, buf.begin(), buf.end());
        }
        else if (std::holds_alternative<string_view_store>(src_slot(run_start)))
        {
            std::vector<string_view_store> buf;
            buf.reserve(run_end - run_start);

            for (row_t k = run_start; k < run_end; ++k)
                buf.push_back(std::get<string_view_store>(src_slot(k)));

            hint = col.set(hint, row, buf.begin(), buf.end());
        }
        else if (std::holds_alternative<formula_cell*>(src_slot(run_start)))
        {
            std::vector<formula_cell*> buf;
            buf.reserve(run_end - run_start);

            for (row_t k = run_start; k < run_end; ++k)
                buf.push_back(std::get<formula_cell*>(src_slot(k)));

            hint = col.set(hint, row, buf.begin(), buf.end());
        }

        // NB: an empty run needs no explicit handling

        run_start = run_end;
    }
}

/**
 * Ensure that a range lies within a sheet store, and throw a
 * std::invalid_argument exception if it does not.
 */
void check_range_in_store(const sheet_store& store, const abs_rc_range_t& range)
{
    if (range.first.row < 0 || range.first.column < 0 ||
        range.last.row < range.first.row || range.last.column < range.first.column)
        throw std::invalid_argument("invalid sort range");

    if (std::size_t(range.last.column) >= store.size())
        throw std::invalid_argument("sort range extends past the last column");

    if (std::size_t(range.last.row) >= store[range.first.column].size())
        throw std::invalid_argument("sort range extends past the last row");
}

} // anonymous namespace

std::vector<row_t> sort_range(
    const model_context& cxt, sheet_store& store,
    const abs_rc_range_t& range, const sort_keys_t& keys)
{
    check_range_in_store(store, range);

    if (keys.empty())
        throw std::invalid_argument("sort requires at least one key");

    for (const sort_key_t& key : keys)
    {
        if (key.column < range.first.column || key.column > range.last.column)
            throw std::invalid_argument("sort key column outside the sort range");
    }

    return range_sorter(cxt, store, range, keys).sort();
}

void reorder_range(
    sheet_store& store, const abs_rc_range_t& range, const std::vector<row_t>& row_order)
{
    check_range_in_store(store, range);

    row_t row1 = range.first.row;
    row_t row2 = range.last.row;

    if (row_t(row_order.size()) != row2 - row1 + 1)
        throw std::invalid_argument("row order size differs from the row count of the range");

    // Ensure that the row order is a permutation of the rows of the range.
    std::vector<bool> seen(row_order.size(), false);

    for (const row_t row : row_order)
    {
        if (row < row1 || row2 < row)
            throw std::invalid_argument("row order contains a row outside the range");

        if (seen[row - row1])
            throw std::invalid_argument("row order contains a duplicate row");

        seen[row - row1] = true;
    }

    range_sorter sorter(store, range);
    sorter.apply(row_order);
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
