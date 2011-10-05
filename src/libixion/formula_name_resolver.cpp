/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_functions.hpp"
#include "ixion/interface/model_context.hpp"

#include <iostream>
#include <sstream>

#define DEBUG_NAME_RESOLVER 0

using namespace std;

namespace ixion {

namespace {

bool resolve_function(const char* p, size_t n, formula_name_type& ret)
{
    formula_function_t func_oc = formula_functions::get_function_opcode(p, n);
    if (func_oc != func_unknown)
    {
        // This is a built-in function.
        ret.type = formula_name_type::function;
        ret.func_oc = func_oc;
        return true;
    }
    return false;
}

/**
 * Check if the name is a built-in function, or else it's considered a named
 * expression.
 *
 * @param name name to be resolved
 * @param ret resolved name type
 */
void resolve_function_or_name(const char* p, size_t n, formula_name_type& ret)
{
    if (resolve_function(p, n, ret))
        return;

    // Everything else is assumed to be a named expression.
    ret.type = formula_name_type::named_expression;
}

enum resolver_parse_mode {
    resolver_parse_column,
    resolver_parse_row
};

void append_column_name_a1(ostringstream& os, col_t col)
{
    const col_t div = 26;
    string col_name;
    while (true)
    {
        col_t rem = col % div;
        char c = 'A' + rem;
        col_name.push_back(c);
        if (col < div)
            break;

        col -= rem;
        col /= div;
        col -= 1;
    }

    reverse(col_name.begin(), col_name.end());
    os << col_name;
}

enum parse_address_result
{
    invalid,
    valid_address,
    range_expected
};

void parse_sheet_name(const ixion::interface::model_context& cxt, const char sep, const char*& p, const char* p_last, sheet_t& sheet)
{
    const char* p_old = p;
    size_t len = 0;

    // parse until we hit the sheet-address separator.
    while (true)
    {
        if (*p == sep)
        {
            sheet = cxt.get_sheet_index(p_old, len);
            if (p != p_last)
                ++p;
            return;
        }

        if (p == p_last)
            break;

        ++p;
        ++len;
    }

    p = p_old;
}

parse_address_result parse_address(
    const ixion::interface::model_context* cxt,
    const char*& p, const char* p_last, sheet_t& sheet, row_t& row, col_t& col, bool& abs_sheet, bool& abs_row, bool& abs_col)
{
    sheet = 0;
    row = 0;
    col = 0;
    abs_sheet = false;
    abs_row = false;
    abs_col = false;

    if (cxt)
        parse_sheet_name(*cxt, '!', p, p_last, sheet);

    resolver_parse_mode mode = resolver_parse_column;

    while (true)
    {
        char c = *p;
        if ('a' <= c && c <= 'z')
        {
            // Convert to upper case.
            c -= 'a' - 'A';
        }

        if ('A' <= c && c <= 'Z')
        {
            // Column digit
            if (mode != resolver_parse_column)
                return invalid;

            if (col)
                col *= 26;
            col += static_cast<col_t>(c - 'A' + 1);
        }
        else if ('0' <= c && c <= '9')
        {
            if (mode == resolver_parse_column)
            {
                // First digit of a row.
                if (c == '0')
                    // Leading zeros not allowed.
                    return invalid;

                mode = resolver_parse_row;
            }

            if (row)
                row *= 10;

            row += static_cast<row_t>(c - '0');
        }
        else if (c == ':')
        {
            if (mode == resolver_parse_row)
            {
                --row;
                --col;
                return range_expected;
            }
            else
                return invalid;
        }
        else
            return invalid;

        if (p == p_last)
            // last character reached.
            break;
        ++p;
    }

    --row;
    --col;
    return valid_address;
}

string abs_or_rel(bool _abs)
{
    return _abs ? "(abs)" : "(rel)";
}

string _to_string(parse_address_result res)
{
    switch (res)
    {
        case invalid:
            return "invalid";
        case range_expected:
            return "range expected";
        case valid_address:
            return "valid address";
        default:
            ;
    }
    return "unknown parse address result";
}

string _to_string(const formula_name_type::address_type& addr)
{
    std::ostringstream os;
    os << "[sheet=" << addr.sheet << abs_or_rel(addr.abs_sheet) << ",row="
        << addr.row << abs_or_rel(addr.abs_row) << ",column="
        << addr.col << abs_or_rel(addr.abs_col) << "]";

    return os.str();
}

void to_relative_address(sheet_t& sheet, row_t& row, col_t& col, bool abs_sheet, bool abs_row, bool abs_col, const abs_address_t& pos)
{
    if (!abs_sheet)
        sheet -= pos.sheet;
    if (!abs_row)
        row -= pos.row;
    if (!abs_col)
        col -= pos.column;
}

} // anonymous namespace

formula_name_type::formula_name_type() : type(invalid) {}

string formula_name_type::to_string() const
{
    std::ostringstream os;

    switch (type)
    {
        case cell_reference:
            os << "cell reference: " << _to_string(address);
            break;
        case function:
            os << "function";
            break;
        case invalid:
            os << "invalid";
            break;
        case named_expression:
            os << "named expression";
            break;
        case range_reference:
            os << "range raference: first: " << _to_string(range.first) << "  last: "
                << _to_string(range.last) << endl;
            break;
        default:
            os << "unknown foromula name type";
    }

    return os.str();
}

formula_name_resolver::formula_name_resolver() {}
formula_name_resolver::~formula_name_resolver() {}

formula_name_resolver_simple::formula_name_resolver_simple() :
    formula_name_resolver() {}

formula_name_resolver_simple::~formula_name_resolver_simple() {}

formula_name_type formula_name_resolver_simple::resolve(
    const char* p, size_t n, const abs_address_t& pos) const
{
    formula_name_type ret;
    resolve_function_or_name(p, n, ret);
    return ret;
}

string formula_name_resolver_simple::get_name(const address_t& addr, const abs_address_t& pos, bool sheet_name) const
{
    return addr.get_name();
}

string formula_name_resolver_simple::get_name(const range_t& range, const abs_address_t& pos, bool sheet_name) const
{
    // TODO: to be implemented.
    return string();
}

string formula_name_resolver_simple::get_name(const abs_address_t& addr, bool sheet_name) const
{
    // TODO: to be implemented.
    return string();
}

string formula_name_resolver_simple::get_name(const abs_range_t& range, bool sheet_name) const
{
    // TODO: to be implemented.
    return string();
}

formula_name_resolver_a1::formula_name_resolver_a1() :
    formula_name_resolver(), mp_cxt(NULL) {}

formula_name_resolver_a1::formula_name_resolver_a1(const interface::model_context* cxt) :
    formula_name_resolver(), mp_cxt(cxt) {}

formula_name_resolver_a1::~formula_name_resolver_a1() {}

formula_name_type formula_name_resolver_a1::resolve(const char* p, size_t n, const abs_address_t& pos) const
{
#if DEBUG_NAME_RESOLVER
    __IXION_DEBUG_OUT__ << "name=" << name << "; origin=" << pos.get_name() << endl;
#endif
    formula_name_type ret;
    if (resolve_function(p, n, ret))
        return ret;

    if (!n)
        return ret;

    const char* p_last = p;
    std::advance(p_last, n -1);
    col_t col = 0;
    row_t row = 0;
    sheet_t sheet = 0;
    bool abs_col = false;
    bool abs_row = false;
    bool abs_sheet = false;

    parse_address_result parse_res =
        parse_address(mp_cxt, p, p_last, sheet, row, col, abs_sheet, abs_row, abs_col);

#if DEBUG_NAME_RESOLVER
    __IXION_DEBUG_OUT__ << "parse address result: " << _to_string(parse_res) << endl;
#endif

    if (parse_res == valid_address)
    {
        // This is a single cell address.
        to_relative_address(sheet, row, col, abs_sheet, abs_row, abs_col, pos);

#if DEBUG_NAME_RESOLVER
        string abs_row_s = abs_row ? "abs" : "rel";
        string abs_col_s = abs_col ? "abs" : "rel";
        cout << "resolve: " << name << "=(row=" << row << " [" << abs_row_s << "]; column=" << col << " [" << abs_col_s << "])" << endl;
#endif
        ret.type = formula_name_type::cell_reference;
        ret.address.sheet = sheet;
        ret.address.row = row;
        ret.address.col = col;
        ret.address.abs_sheet = abs_sheet;
        ret.address.abs_row = abs_row;
        ret.address.abs_col = abs_col;
        return ret;
    }

    if (parse_res == range_expected)
    {
        if (p == p_last)
            // ':' occurs as the last character.  This is not allowed.
            return ret;

        ++p; // skip ':'

        to_relative_address(sheet, row, col, abs_sheet, abs_row, abs_col, pos);

        ret.range.first.sheet = sheet;
        ret.range.first.row = row;
        ret.range.first.col = col;
        ret.range.first.abs_sheet = abs_sheet;
        ret.range.first.abs_row = abs_row;
        ret.range.first.abs_col = abs_col;

        // For now, we assume the sheet index of the end address is identical
        // to that of the begin address.
        parse_res = parse_address(NULL, p, p_last, sheet, row, col, abs_sheet, abs_row, abs_col);
        if (parse_res != valid_address)
            // The 2nd part after the ':' is not valid.
            return ret;

        to_relative_address(sheet, row, col, abs_sheet, abs_row, abs_col, pos);

        ret.range.last.sheet = ret.range.first.sheet; // re-use the sheet index of the begin address.
        ret.range.last.row = row;
        ret.range.last.col = col;
        ret.range.last.abs_sheet = abs_sheet;
        ret.range.last.abs_row = abs_row;
        ret.range.last.abs_col = abs_col;
        ret.type = formula_name_type::range_reference;
        return ret;
    }

    resolve_function_or_name(p, n, ret);
    return ret;
}

string formula_name_resolver_a1::get_name(const address_t& addr, const abs_address_t& pos, bool sheet_name) const
{
    ostringstream os;
    col_t col = addr.column;
    row_t row = addr.row;
    sheet_t sheet = addr.sheet;
    if (!addr.abs_column)
        col += pos.column;
    if (!addr.abs_row)
        row += pos.row;
    if (!addr.abs_sheet)
        sheet += pos.sheet;

    if (sheet_name && mp_cxt)
        os << mp_cxt->get_sheet_name(sheet) << '!';

    append_column_name_a1(os, col);
    os << (row + 1);
    return os.str();
}

string formula_name_resolver_a1::get_name(const range_t& range, const abs_address_t& pos, bool sheet_name) const
{
    // For now, sheet index of the end-range address is ignored.

    ostringstream os;
    col_t col = range.first.column;
    row_t row = range.first.row;
    sheet_t sheet = range.first.sheet;
    if (!range.first.abs_column)
        col += pos.column;
    if (!range.first.abs_row)
        row += pos.row;
    if (!range.first.abs_sheet)
        sheet += pos.sheet;

    if (sheet_name && mp_cxt)
        os << mp_cxt->get_sheet_name(sheet) << '!';

    append_column_name_a1(os, col);
    os << (row + 1);
    os << ":";

    col = range.last.column;
    row = range.last.row;
    if (!range.last.abs_column)
        col += pos.column;
    if (!range.last.abs_row)
        row += pos.row;
    append_column_name_a1(os, col);
    os << (row + 1);
    return os.str();
}

string formula_name_resolver_a1::get_name(const abs_address_t& addr, bool sheet_name) const
{
    ostringstream os;
    if (sheet_name && mp_cxt)
        os << mp_cxt->get_sheet_name(addr.sheet) << '!';

    append_column_name_a1(os, addr.column);
    os << (addr.row + 1);
    return os.str();
}

string formula_name_resolver_a1::get_name(const abs_range_t& range, bool sheet_name) const
{
    // For now, sheet index of the end-range address is ignored.

    ostringstream os;
    if (sheet_name && mp_cxt)
        os << mp_cxt->get_sheet_name(range.first.sheet) << '!';

    col_t col = range.first.column;
    row_t row = range.first.row;
    append_column_name_a1(os, col);
    os << (row + 1);
    os << ":";

    col = range.last.column;
    row = range.last.row;
    append_column_name_a1(os, col);
    os << (row + 1);
    return os.str();
}

}
