/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/formula.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula_function_opcode.hpp>
#include <ixion/cell.hpp>
#include <ixion/dirty_cell_tracker.hpp>
#include <ixion/types.hpp>
#include <ixion/model_context.hpp>

#include "formula_lexer.hpp"
#include "formula_parser.hpp"
#include "formula_functions.hpp"
#include "debug.hpp"

#include <sstream>
#include <algorithm>

namespace ixion {

namespace {

#if IXION_LOGGING

[[maybe_unused]] std::string debug_print_formula_tokens(const formula_tokens_t& tokens)
{
    std::ostringstream os;

    for (const formula_token& t : tokens)
    {
        os << std::endl << "  * " << t;
    }

    return os.str();
}

#endif

void print_token(
    const print_config& config, const model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_token& token, std::ostream& os)
{
    auto sheet_to_print = [&config, &pos](const address_t& ref_addr) -> bool
    {
        switch (config.display_sheet)
        {
            case display_sheet_t::always:
                return true;
            case display_sheet_t::never:
                return false;
            case display_sheet_t::only_if_different:
                return ref_addr.to_abs(pos).sheet != pos.sheet;
            case display_sheet_t::unspecified:
                break;
        }
        return false;
    };

    switch (token.opcode)
    {
        case fop_array_open:
            os << '{';
            break;
        case fop_array_close:
            os << '}';
            break;
        case fop_close:
            os << ')';
            break;
        case fop_divide:
            os << '/';
            break;
        case fop_minus:
            os << '-';
            break;
        case fop_multiply:
            os << '*';
            break;
        case fop_exponent:
            os << '^';
            break;
        case fop_concat:
            os << '&';
            break;
        case fop_open:
            os << '(';
            break;
        case fop_plus:
            os << '+';
            break;
        case fop_value:
            os << std::get<double>(token.value);
            break;
        case fop_sep:
            os << cxt.get_config().sep_function_arg;
            break;
        case fop_array_row_sep:
            os << cxt.get_config().sep_matrix_row;
            break;
        case fop_function:
        {
            auto fop = std::get<formula_function_t>(token.value);
            os << formula_functions::get_function_name(fop);
            break;
        }
        case fop_error:
        {
            auto err = std::get<formula_error_t>(token.value);
            os << get_formula_error_name(err);
            break;
        }
        case fop_single_ref:
        {
            const address_t& addr = std::get<address_t>(token.value);
            bool sheet_name = sheet_to_print(addr);
            os << resolver.get_name(addr, pos, sheet_name);
            break;
        }
        case fop_range_ref:
        {
            const range_t& range = std::get<range_t>(token.value);
            bool sheet_name = sheet_to_print(range.first);
            os << resolver.get_name(range, pos, sheet_name);
            break;
        }
        case fop_table_ref:
        {
            const table_ref_t& tbl = std::get<table_ref_t>(token.value);
            os << resolver.get_name(tbl);
            break;
        }
        case fop_string:
        {
            os << '"' << std::get<std::string_view>(token.value) << '"';
            break;
        }
        case fop_equal:
            os << "=";
            break;
        case fop_not_equal:
            os << "<>";
            break;
        case fop_less:
            os << "<";
            break;
        case fop_greater:
            os << ">";
            break;
        case fop_less_equal:
            os << "<=";
            break;
        case fop_greater_equal:
            os << ">=";
            break;
        case fop_named_expression:
            os << std::get<std::string>(token.value);
            break;
        case fop_unknown:
        default:
        {
            std::ostringstream repr;
            repr << token;
            IXION_DEBUG(
                "token not printed (repr='" << repr.str()
                << "'; name='" << get_formula_opcode_name(token.opcode)
                << "'; opcode='" << get_formula_opcode_string(token.opcode)
                << "')");
        }
    }
}

}

formula_tokens_t parse_formula_string(
    model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, std::string_view formula)
{
    IXION_TRACE("pos=" << pos.get_name() << "; formula='" << formula << "'");
    lexer_tokens_t lxr_tokens;
    formula_lexer lexer(cxt.get_config(), formula.data(), formula.size());
    lexer.tokenize();
    lexer.swap_tokens(lxr_tokens);

    IXION_TRACE("lexer tokens: " << print_tokens(lxr_tokens, true));

    formula_tokens_t tokens;
    formula_parser parser(lxr_tokens, cxt, resolver);
    parser.set_origin(pos);
    parser.parse();
    parser.get_tokens().swap(tokens);

    IXION_TRACE("formula tokens (string): " << print_formula_tokens(cxt, pos, resolver, tokens));
    IXION_TRACE("formula tokens (individual): " << debug_print_formula_tokens(tokens));

    return tokens;
}

formula_tokens_t create_formula_error_tokens(
    model_context& cxt, std::string_view src_formula,
    std::string_view error)
{
    formula_tokens_t tokens;
    tokens.emplace_back(fop_invalid_formula);
    tokens.back().value = string_id_t{2u};

    tokens.emplace_back(cxt.intern_string(src_formula));
    tokens.emplace_back(cxt.intern_string(error));

    return tokens;
}

std::string print_formula_tokens(
    const model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_tokens_t& tokens)
{
    print_config config;
    config.display_sheet = display_sheet_t::only_if_different;
    return print_formula_tokens(config, cxt, pos, resolver, tokens);
}

std::string print_formula_tokens(
    const print_config& config, const model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_tokens_t& tokens)
{
    std::ostringstream os;

    if (!tokens.empty() && tokens[0].opcode == fop_invalid_formula)
        // Let's not print anything on error tokens.
        return std::string();

    for (const formula_token& token : tokens)
        print_token(config, cxt, pos, resolver, token, os);

    return os.str();
}

std::string print_formula_token(
    const model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_token& token)
{
    print_config config;
    config.display_sheet = display_sheet_t::only_if_different;
    return print_formula_token(config, cxt, pos, resolver, token);
}

std::string print_formula_token(
    const print_config& config, const model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_token& token)
{
    std::ostringstream os;
    print_token(config, cxt, pos, resolver, token, os);
    return os.str();
}

abs_address_set_t query_dirty_cells(model_context& cxt, const abs_address_set_t& modified_cells)
{
    abs_range_set_t modified_ranges;
    for (const abs_address_t& mc : modified_cells)
        modified_ranges.insert(mc);

    const dirty_cell_tracker& tracker = cxt.get_cell_tracker();
    abs_range_set_t dirty_ranges = tracker.query_dirty_cells(modified_ranges);

    // Convert a set of ranges to a set of addresses.
    abs_address_set_t dirty_cells;
    for (const abs_range_t& r : dirty_ranges)
        dirty_cells.insert(r.first);
    return dirty_cells;
}

std::vector<abs_range_t> query_and_sort_dirty_cells(
    model_context& cxt, const abs_range_set_t& modified_cells,
    const abs_range_set_t* dirty_formula_cells)
{
    const dirty_cell_tracker& tracker = cxt.get_cell_tracker();
    return tracker.query_and_sort_dirty_cells(modified_cells, dirty_formula_cells);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
