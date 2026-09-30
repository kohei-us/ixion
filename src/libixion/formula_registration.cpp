/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_registration.hpp"
#include "model_context_impl.hpp"
#include "debug.hpp"

#include <ixion/cell.hpp>
#include <ixion/dirty_cell_tracker.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/formula_function_opcode.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/model_cell_range.hpp>
#include <ixion/model_context.hpp>
#include <ixion/types.hpp>

#include <format>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ixion { namespace detail {

namespace {

// A function added here must also get 'volatile: true' in
// misc/function-specs.yaml.
bool is_volatile(formula_function_t func)
{
    switch (func)
    {
        case formula_function_t::func_now:
        case formula_function_t::func_rand:
        case formula_function_t::func_today:
            return true;
        default:
            ;
    }
    return false;
}

bool has_volatile(const formula_tokens_t& tokens)
{
    for (const auto& t : tokens)
    {
        if (t.opcode != fop_function)
            continue;

        auto func = std::get<formula_function_t>(t.value);
        if (is_volatile(func))
            return true;
    }
    return false;
}

void check_sheet_or_throw(
    const char* func_name, sheet_t sheet, const model_context_impl& cxt,
    const abs_address_t& pos, const formula_cell& cell)
{
    if (is_valid_sheet(sheet))
        return;

    IXION_DEBUG("invalid range reference: func=" << func_name
        << "; pos=" << pos.get_name()
        << "; formula='" << detail::print_formula_expression(cxt.get_parent(), pos, cell)
        << "'");

    throw model_context_error(std::format(
        "{}: invalid sheet index in {}: formula='{}'",
        func_name, pos.get_name(), detail::print_formula_expression(cxt.get_parent(), pos, cell)),
        model_context_error::invalid_sheet_reference);
}

/**
 * Throw if a reference token points at an invalid sheet.  Tokens other than
 * references pass.
 */
void check_ref_sheet_or_throw(
    const char* func_name, const formula_token& token, const model_context_impl& cxt,
    const abs_address_t& pos, const formula_cell& cell)
{
    switch (token.opcode)
    {
        case fop_single_ref:
        {
            abs_address_t addr = std::get<address_t>(token.value).to_abs(pos);
            check_sheet_or_throw(func_name, addr.sheet, cxt, pos, cell);
            break;
        }
        case fop_range_ref:
        {
            abs_range_t range = std::get<range_t>(token.value).to_abs(pos);
            check_sheet_or_throw(func_name, range.first.sheet, cxt, pos, cell);
            break;
        }
        default:
            ; // ignore the rest.
    }
}

/**
 * Throw unless the position is that of the top-left cell of the formula
 * group the cell belongs to.  A non-grouped cell always passes.
 */
void check_group_parent_or_throw(
    const char* func_name, const abs_address_t& pos, const formula_cell& cell)
{
    abs_address_t parent = cell.get_parent_position(pos);
    if (parent == pos)
        return;

    throw model_context_error(std::format(
        "{}: {} is not the top-left cell of its formula group starting at {}",
        func_name, pos.get_name(), parent.get_name()),
        model_context_error::partial_formula_group);
}

/**
 * Get the range a formula cell occupies as a listener in the dependency
 * tracker.  For a grouped formula cell, it spans the whole group.
 */
abs_range_t to_listener_range(const abs_address_t& pos, const formula_cell& cell)
{
    abs_range_t range = pos;

    formula_group_t fg_props = cell.get_group_properties();
    if (fg_props.grouped)
    {
        // Expand the source range for grouped formula cells.
        range.last.column += fg_props.size.column - 1;
        range.last.row += fg_props.size.row - 1;
    }

    return range;
}

/**
 * Normalize a range reference into the form the dependency tracker
 * accepts: expand a whole-column or whole-row reference to the sheet
 * size, and order the corners.
 */
abs_range_t to_tracked_range(const model_context_impl& cxt, abs_range_t range)
{
    rc_size_t sheet_size = cxt.get_sheet_size();
    if (range.all_columns())
    {
        range.first.column = 0;
        range.last.column = sheet_size.column - 1;
    }
    if (range.all_rows())
    {
        range.first.row = 0;
        range.last.row = sheet_size.row - 1;
    }
    range.reorder();
    return range;
}

/** Check whether the inner range lies entirely inside the outer one. */
bool is_inside(const abs_range_t& inner, const abs_rc_range_t& outer)
{
    bool rows_inside =
        outer.first.row <= inner.first.row && inner.last.row <= outer.last.row;
    bool columns_inside =
        outer.first.column <= inner.first.column && inner.last.column <= outer.last.column;
    return rows_inside && columns_inside;
}

/** Formula cell paired with its top-left position. */
using formula_cell_entry = std::pair<abs_address_t, const formula_cell*>;

/**
 * Collect the formula cells of a range, one entry per group at its top-left
 * cell.  Throws if a group lies only partly inside the range.
 */
std::vector<formula_cell_entry> collect_formula_cells(
    const model_context_impl& cxt, sheet_t sheet, const abs_rc_range_t& range)
{
    std::vector<formula_cell_entry> entries;

    auto cells = cxt.iterate_cells(sheet, rc_direction_t::vertical, range);

    for (const auto& cell : cells)
    {
        if (cell.type != cell_t::formula)
            continue;

        const auto* fc = std::get<const formula_cell*>(cell.value);
        abs_address_t pos(sheet, cell.row, cell.col);
        abs_address_t parent = fc->get_parent_position(pos);

        abs_range_t group = to_listener_range(parent, *fc);
        if (!is_inside(group, range))
        {
            std::ostringstream os;
            os << "collect_formula_cells: formula group " << group
                << " lies only partly inside " << range;
            throw model_context_error(os.str(), model_context_error::partial_formula_group);
        }

        // Registering the top-left cell of a group registers the entire
        // group, so skip the other cells of the group.
        if (parent != pos)
            continue;

        entries.emplace_back(pos, fc);
    }

    return entries;
}

}

std::vector<const formula_token*> validate_formula_registration(
    const model_context_impl& cxt, const abs_address_t& pos, const formula_cell& cell)
{
    check_group_parent_or_throw("validate_formula_registration", pos, cell);

    std::vector<const formula_token*> ref_tokens = cxt.get_ref_tokens(cell, pos);

    for (const formula_token* p : ref_tokens)
        check_ref_sheet_or_throw("validate_formula_registration", *p, cxt, pos, cell);

    return ref_tokens;
}

void apply_formula_registration(
    model_context_impl& cxt, const abs_address_t& pos, const formula_cell& cell,
    const std::vector<const formula_token*>& ref_tokens)
{
#ifdef IXION_DEBUG_UTILS
    const formula_cell* check = std::as_const(cxt).get_formula_cell(pos);
    if (&cell != check)
    {
        throw std::runtime_error(
            "the cell instance passed to this call does not match "
            "the cell instance found at the specified position");
    }
#endif

    dirty_cell_tracker& tracker = cxt.get_cell_tracker();
    abs_range_t src_pos = to_listener_range(pos, cell);

    IXION_TRACE("pos=" << pos.get_name()
        << "; formula='" << detail::print_formula_expression(cxt.get_parent(), pos, cell)
        << "'");

    for (const formula_token* p : ref_tokens)
    {
        IXION_TRACE("ref token: " << detail::print_formula_token_repr(*p));

        switch (p->opcode)
        {
            case fop_single_ref:
            {
                abs_address_t addr = std::get<address_t>(p->value).to_abs(pos);
                tracker.add(src_pos, addr);
                break;
            }
            case fop_range_ref:
            {
                abs_range_t range = std::get<range_t>(p->value).to_abs(pos);
                tracker.add(src_pos, to_tracked_range(cxt, range));
                break;
            }
            case fop_table_ref:
            {
                abs_range_t range = cxt.get_table_range(std::get<table_ref_t>(p->value), pos);
                if (!range.valid())
                    // silently ignore unresolvable table references.
                    break;

                tracker.add(src_pos, range);
                break;
            }
            default:
                ; // ignore the rest.
        }
    }

    // Check if the cell is volatile.
    const formula_tokens_store_ptr_t& ts = cell.get_tokens();
    if (ts && has_volatile(ts->get()))
        tracker.add_volatile(pos);
}

void remove_formula_registration(
    model_context_impl& cxt, const abs_address_t& pos, const formula_cell& cell)
{
    check_group_parent_or_throw("remove_formula_registration", pos, cell);

    dirty_cell_tracker& tracker = cxt.get_cell_tracker();
    tracker.remove_volatile(pos);

    abs_range_t src_pos = to_listener_range(pos, cell);

    // Go through all its existing references, and remove itself as their
    // listener.  This step is important especially during partial
    // re-calculation.
    for (const formula_token* p : cxt.get_ref_tokens(cell, pos))
    {

        switch (p->opcode)
        {
            case fop_single_ref:
            {
                abs_address_t addr = std::get<address_t>(p->value).to_abs(pos);
                check_sheet_or_throw("remove_formula_registration", addr.sheet, cxt, pos, cell);
                tracker.remove(src_pos, addr);
                break;
            }
            case fop_range_ref:
            {
                abs_range_t range = std::get<range_t>(p->value).to_abs(pos);
                check_sheet_or_throw(
                    "remove_formula_registration", range.first.sheet, cxt, pos, cell);
                tracker.remove(src_pos, to_tracked_range(cxt, range));
                break;
            }
            case fop_table_ref:
            {
                abs_range_t range = cxt.get_table_range(std::get<table_ref_t>(p->value), pos);
                if (!range.valid())
                    // silently ignore unresolvable table references.
                    break;

                tracker.remove(src_pos, range);
                break;
            }
            default:
                ; // ignore the rest.
        }
    }
}

void register_formula_cells(model_context_impl& cxt, sheet_t sheet, const abs_rc_range_t& range)
{
    std::vector<formula_cell_entry> entries = collect_formula_cells(cxt, sheet, range);

    // Validate every cell before registering any of them, so that a
    // rejected cell leaves the tracker unchanged.
    std::vector<std::vector<const formula_token*>> ref_tokens;
    ref_tokens.reserve(entries.size());

    for (const auto& [pos, fc] : entries)
        ref_tokens.push_back(validate_formula_registration(cxt, pos, *fc));

    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        const auto& [pos, fc] = entries[i];
        apply_formula_registration(cxt, pos, *fc, ref_tokens[i]);
    }
}

void unregister_formula_cells(model_context_impl& cxt, sheet_t sheet, const abs_rc_range_t& range)
{
    std::vector<formula_cell_entry> entries = collect_formula_cells(cxt, sheet, range);

    for (const auto& [pos, fc] : entries)
        remove_formula_registration(cxt, pos, *fc);
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
