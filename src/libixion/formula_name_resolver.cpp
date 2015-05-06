/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/formula_name_resolver.hpp"
#include "ixion/interface/formula_model_access.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/table.hpp"

#include "formula_functions.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>
#include <limits>

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
 * Table reference can be either one of:
 *
 * <ul>
 * <li>Table[Column]</li>
 * <li>[Column]</li>
 * <li>Table[[#Area],[Column]]</li>
 * <li>Table[[#Area1],[#Area2],[Column]]</li>
 * </ul>
 *
 * where the #Area (area specifier) can be one or more of
 *
 * <ul>
 * <li>#Header</li>
 * <li>#Data</li>
 * <li>#Totals</li>
 * <li>#All</li>
 * </ul>
 */
bool resolve_table(const iface::formula_model_access* cxt, const char* p, size_t n, formula_name_type& ret)
{
    if (!cxt)
        return false;

    short scope = 0;
    size_t last_column_pos = std::numeric_limits<size_t>::max();
    mem_str_buf buf;
    mem_str_buf table_name;
    vector<mem_str_buf> names;

    bool table_detected = false;

    const char* p_end = p + n;
    for (; p != p_end; ++p)
    {
        switch (*p)
        {
            case '[':
            {
                if (scope >= 2)
                    return false;

                table_detected = true;
                if (!buf.empty())
                {
                    if (scope != 0)
                        return false;

                    table_name = buf;
                    buf.clear();
                }

                ++scope;
            }
            break;
            case ']':
            {
                if (scope <= 0)
                    // non-matching brace.
                    return false;

                if (!buf.empty())
                {
                    names.push_back(buf);
                    buf.clear();
                }

                if (!scope)
                    // Non-matching brace detected.  Bail out.
                    return false;

                --scope;
            }
            break;
            case ',':
            {
                if (!buf.empty())
                    return false;
            }
            break;
            case ':':
            {
                if (scope != 1)
                    // allowed only inside the first scope.
                    return false;

                if (!buf.empty())
                    return false;

                if (names.empty())
                    return false;

                last_column_pos = names.size();
            }
            break;
            default:
                if (buf.empty())
                    buf.set_start(p);
                else
                    buf.inc();
        }
    }

    if (!buf.empty())
        return false;

    if (!table_detected)
        return false;

    if (names.empty())
        return false;

    ret.table.areas = table_area_none;
    ret.type = formula_name_type::table_reference;
    ret.table.name = table_name.get();
    ret.table.name_length = table_name.size();
    ret.table.column_first = NULL;
    ret.table.column_first_length = 0;
    ret.table.column_last = NULL;
    ret.table.column_last_length = 0;

    vector<mem_str_buf>::iterator it = names.begin(), it_end = names.end();
    for (; it != it_end; ++it)
    {
        buf = *it;
        assert(!buf.empty());
        if (buf[0] == '#')
        {
            // area specifier.
            buf.pop_front();
            if (buf.equals("Headers"))
                ret.table.areas |= table_area_headers;
            else if (buf.equals("Data"))
                ret.table.areas |= table_area_data;
            else if (buf.equals("Totals"))
                ret.table.areas |= table_area_totals;
            else if (buf.equals("All"))
                ret.table.areas = table_area_all;
        }
        else if (ret.table.column_first_length)
        {
            // This is a second column name.
            if (ret.table.column_last_length)
                return false;

            size_t dist = std::distance(names.begin(), it);
            if (dist != last_column_pos)
                return false;

            ret.table.column_last = buf.get();
            ret.table.column_last_length = buf.size();
        }
        else
        {
            // first column name.
            if (!ret.table.areas)
                ret.table.areas = table_area_data;

            ret.table.column_first = buf.get();
            ret.table.column_first_length = buf.size();
        }
    }

    return true;
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

void set_address(formula_name_type::address_type& dest, const address_t& addr)
{
    dest.sheet = addr.sheet;
    dest.row = addr.row;
    dest.col = addr.column;
    dest.abs_sheet = addr.abs_sheet;
    dest.abs_row = addr.abs_row;
    dest.abs_col = addr.abs_column;
}

void set_cell_reference(formula_name_type& ret, const address_t& addr)
{
    ret.type = formula_name_type::cell_reference;
    set_address(ret.address, addr);
}

enum resolver_parse_mode { column, row };

void write_sheet_name(ostringstream& os, const ixion::iface::formula_model_access& cxt, sheet_t sheet)
{
    string sheet_name = cxt.get_sheet_name(sheet);
    bool quote = sheet_name.find_first_of(' ') != string::npos;

    if (quote)
        os << '\'';

    os << sheet_name;

    if (quote)
        os << '\'';
}

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

void append_address_a1(
    ostringstream& os, const ixion::iface::formula_model_access* cxt,
    const address_t& addr, const abs_address_t& pos, char sheet_name_sep)
{
    col_t col = addr.column;
    row_t row = addr.row;
    sheet_t sheet = addr.sheet;
    if (!addr.abs_column)
        col += pos.column;
    if (!addr.abs_row)
        row += pos.row;
    if (!addr.abs_sheet)
        sheet += pos.sheet;

    if (sheet_name_sep && cxt)
    {
        write_sheet_name(os, *cxt, sheet);
        os << sheet_name_sep;
    }

    if (addr.abs_column)
        os << '$';
    append_column_name_a1(os, col);

    if (addr.abs_row)
        os << '$';
    os << (row + 1);
}

void append_name_string(ostringstream& os, const iface::formula_model_access* cxt, string_id_t sid)
{
    if (!cxt)
        return;

    const string* p = cxt->get_string(sid);
    if (p)
        os << *p;
}

char append_table_areas(ostringstream& os, const table_t& table)
{
    if (table.areas == table_area_all)
    {
        os << "[#All]";
        return 1;
    }

    bool headers = (table.areas & table_area_headers);
    bool data = (table.areas & table_area_data);
    bool totals = (table.areas & table_area_totals);

    char count = 0;
    if (headers)
    {
        os << "[#Headers]";
        ++count;
    }

    if (data)
    {
        if (count > 0)
            os << ',';
        os << "[#Data]";
        ++count;
    }

    if (totals)
    {
        if (count > 0)
            os << ',';
        os << "[#Totals]";
        ++count;
    }

    return count;
}

enum parse_address_result
{
    invalid = 0,
    valid_address,
    range_expected /// valid address followed by a ':'.
};

#if DEBUG_NAME_RESOLVER
const char* parse_address_result_names[] = {
    "invalid",
    "valid address",
    "range expected"
};
#endif

void parse_sheet_name_quoted(const ixion::iface::formula_model_access& cxt, const char sep, const char*& p, const char* p_last, sheet_t& sheet)
{
    const char* p_old = p;
    ++p; // skip the open quote.
    size_t len = 0;

    // parse until the closing quote is reached.
    while (true)
    {
        if (*p == '\'')
        {
            if (p == p_last || *(p+1) != sep)
                // the next char must be the separator.  Parse failed.
                break;

            const char* p1 = p_old + 1;
            sheet = cxt.get_sheet_index(p1, len);
            ++p; // skip the closing quote.
            if (p != p_last)
                ++p; // skip the separator.
            return;
        }

        if (p == p_last)
            break;

        ++p;
        ++len;
    }

    p = p_old;
}

void parse_sheet_name(const ixion::iface::formula_model_access& cxt, const char sep, const char*& p, const char* p_last, sheet_t& sheet)
{
    if (*p == '\'')
    {
        parse_sheet_name_quoted(cxt, sep, p, p_last, sheet);
        return;
    }

    const char* p_old = p;
    size_t len = 0;

    // parse until we hit the sheet-address separator.
    while (true)
    {
        if (*p == sep)
        {
            sheet = cxt.get_sheet_index(p_old, len);
            if (p != p_last)
                ++p; // skip the separator.
            return;
        }

        if (p == p_last)
            break;

        ++p;
        ++len;
    }

    p = p_old;
}

/**
 * If there is no number to parse, it returns 0 and the p will not
 * increment. Otherwise, p will point to the last digit of the number when
 * the call returns.
 */
template<typename T>
T parse_number(const char*&p, const char* p_last)
{
    T num = 0;

    bool sign = false;
    if (*p == '+')
        ++p;
    else if (*p == '-')
    {
        ++p;
        sign = true;
    }

    bool all_digits = false;
    while (is_digit(*p))
    {
        // Parse number.
        num *= 10;
        num += *p - '0';
        if (p == p_last)
        {
            all_digits = true;
            break;
        }
        ++p;
    }

    if (!all_digits)
        --p;

    if (sign)
        num *= -1;

    return num;
}

/**
 * Parse A1-style single cell address.
 *
 * @param p it must point to the first character of a cell address.
 * @param p_last it must point to the last character of a whole string
 *               sequence.  It doesn't have to be the last character of a
 *               cell address.
 * @param addr resolved cell address.
 *
 * @return parsing result.
 */
parse_address_result parse_address_a1(const char*& p, const char* p_last, address_t& addr)
{
    // NOTE: Row and column IDs are 1-based during parsing, while 0 is used as
    // the state of a value-not-set.  They are subtracted by one before
    // returning.

    resolver_parse_mode mode = resolver_parse_mode::column;

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
            if (mode != resolver_parse_mode::column)
                return invalid;

            if (addr.column)
                addr.column *= 26;
            addr.column += static_cast<col_t>(c - 'A' + 1);
        }
        else if (is_digit(c))
        {
            if (mode == resolver_parse_mode::column)
            {
                // First digit of a row.
                if (c == '0')
                    // Leading zeros not allowed.
                    return invalid;

                mode = resolver_parse_mode::row;
            }

            if (addr.row)
                addr.row *= 10;

            addr.row += static_cast<row_t>(c - '0');
        }
        else if (c == ':')
        {
            if (mode == resolver_parse_mode::row)
            {
                if (!addr.row)
                    return invalid;

                --addr.row;

                if (addr.column)
                    --addr.column;
                else
                    addr.column = column_unset;

                return range_expected;
            }
            else if (mode == resolver_parse_mode::column)
            {
                // row number is not given.
                if (addr.column)
                    --addr.column;
                else
                    // neither row number nor column number is given.
                    return invalid;

                addr.row = row_unset;
                return range_expected;
            }
            else
                return invalid;
        }
        else if (c == '$')
        {
            // Absolute position.
            if (mode == resolver_parse_mode::column)
            {
                if (addr.column)
                {
                    // Column position has been already parsed.
                    mode = resolver_parse_mode::row;
                    addr.abs_row = true;
                }
                else
                {
                    // Column position has not yet been parsed.
                    addr.abs_column = true;
                }
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

    if (mode == resolver_parse_mode::row)
    {
        if (!addr.row)
            return invalid;

        --addr.row;

        if (addr.column)
            --addr.column;
        else
            addr.column = column_unset;
    }
    else if (mode == resolver_parse_mode::column)
    {
        // row number is not given.
        if (addr.column)
            --addr.column;
        else
            // neither row number nor column number is given.
            return invalid;

        addr.row = row_unset;
    }
    return valid_address;
}

parse_address_result parse_address_r1c1(const char*& p, const char* p_last, address_t& addr)
{
    addr.row = row_unset;
    addr.column = column_unset;

    if (*p == 'R')
    {
        addr.row = 0;
        addr.abs_row = false;

        if (p == p_last)
            // Just 'R'.  Not sure if this is valid or invalid, but let's call it invalid for now.
            return parse_address_result::invalid;

        ++p;
        if (*p != 'C')
        {
            addr.abs_row = (*p != '[');
            if (!addr.abs_row)
            {
                // Relative row address.
                ++p;
                if (!is_digit(*p) && *p != '-' && *p != '+')
                    return parse_address_result::invalid;

                addr.row = parse_number<row_t>(p, p_last);
                ++p;
                if (p == p_last)
                    return (*p == ']') ? parse_address_result::valid_address : parse_address_result::invalid;
                ++p;
            }
            else if (is_digit(*p))
            {
                // Absolute row address.
                addr.row = parse_number<row_t>(p, p_last);
                if (addr.row <= 0)
                    // absolute address with 0 or negative value is invalid.
                    return parse_address_result::invalid;

                --addr.row; // 1-based to 0-based.

                if (p == p_last && is_digit(*p))
                    // 'R' followed by a number without 'C' is valid.
                    return parse_address_result::valid_address;
                ++p;
            }
        }
    }

    if (*p == 'C')
    {
        addr.column = 0;
        addr.abs_column = false;

        if (p == p_last)
        {
            if (addr.row == row_unset)
                // Just 'C'.  Row must be set.
                return parse_address_result::invalid;

            if (!addr.abs_row && addr.row == 0)
                // 'RC' is invalid as it references itself.
                return parse_address_result::invalid;

            return parse_address_result::valid_address;
        }

        ++p;
        addr.abs_column = (*p != '[');
        if (!addr.abs_column)
        {
            // Relative column address.
            ++p;
            if (!is_digit(*p) && *p != '-' && *p != '+')
                return parse_address_result::invalid;

            addr.column = parse_number<col_t>(p, p_last);
            ++p;
            if (p == p_last)
                return (*p == ']') ? parse_address_result::valid_address : parse_address_result::invalid;
            ++p;
        }
        else if (is_digit(*p))
        {
            // Absolute column address.
            addr.column = parse_number<col_t>(p, p_last);
            if (addr.column <= 0)
                // absolute address with 0 or negative value is invalid.
                return parse_address_result::invalid;

            --addr.column; // 1-based to 0-based.

            if (p == p_last)
                return parse_address_result::valid_address;
        }
    }

    return parse_address_result::invalid;
}

parse_address_result parse_address_excel_a1(
    const ixion::iface::formula_model_access* cxt, const char*& p, const char* p_last, address_t& addr)
{
    addr.row = 0;
    addr.column = 0;
    addr.abs_sheet = false;
    addr.abs_row = false;
    addr.abs_column = false;

    if (cxt)
        // Overwrite the sheet index *only when* the sheet name is parsed successfully.
        parse_sheet_name(*cxt, '!', p, p_last, addr.sheet);

    return parse_address_a1(p, p_last, addr);
}

parse_address_result parse_address_excel_r1c1(
    const iface::formula_model_access* cxt, const char*& p, const char* p_last, address_t& addr)
{
    addr.row = 0;
    addr.column = 0;
    addr.abs_sheet = false;
    addr.abs_row = false;
    addr.abs_column = false;

    if (cxt)
        // Overwrite the sheet index *only when* the sheet name is parsed successfully.
        parse_sheet_name(*cxt, '!', p, p_last, addr.sheet);

    return parse_address_r1c1(p, p_last, addr);
}

parse_address_result parse_address_odff(
    const ixion::iface::formula_model_access* cxt, const char*& p, const char* p_last, address_t& addr)
{
    addr.row = 0;
    addr.column = 0;
    addr.abs_sheet = false;
    addr.abs_row = false;
    addr.abs_column = false;

    if (*p == '.')
    {
        // This address doesn't contain sheet name.
        ++p;
    }
    else if (cxt)
    {
        // Overwrite the sheet index *only when* the sheet name is parsed successfully.
        parse_sheet_name(*cxt, '.', p, p_last, addr.sheet);
    }

    return parse_address_a1(p, p_last, addr);
}

string abs_or_rel(bool _abs)
{
    return _abs ? "(abs)" : "(rel)";
}

string _to_string(const formula_name_type::address_type& addr)
{
    std::ostringstream os;
    os << "[sheet=" << addr.sheet << abs_or_rel(addr.abs_sheet) << ",row="
        << addr.row << abs_or_rel(addr.abs_row) << ",column="
        << addr.col << abs_or_rel(addr.abs_col) << "]";

    return os.str();
}

void to_relative_address(address_t& addr, const abs_address_t& pos)
{
    if (!addr.abs_sheet)
        addr.sheet -= pos.sheet;
    if (!addr.abs_row)
        addr.row -= pos.row;
    if (!addr.abs_column)
        addr.column -= pos.column;
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
        case table_reference:
            os << "table reference";
            break;
        default:
            os << "unknown foromula name type";
    }

    return os.str();
}

address_t to_address(const formula_name_type::address_type& src)
{
    address_t addr;

    addr.sheet      = src.sheet;
    addr.abs_sheet  = src.abs_sheet;
    addr.row        = src.row;
    addr.abs_row    = src.abs_row;
    addr.column     = src.col;
    addr.abs_column = src.abs_col;

    return addr;
}

range_t to_range(const formula_name_type::range_type& src)
{
    range_t range;
    range.first = to_address(src.first);
    range.last = to_address(src.last);

    return range;
}

formula_name_resolver::formula_name_resolver() {}
formula_name_resolver::~formula_name_resolver() {}

namespace {

string get_column_name_a1(col_t col)
{
    ostringstream os;
    append_column_name_a1(os, col);
    return os.str();
}

class excel_a1 : public formula_name_resolver
{
public:
    excel_a1(const iface::formula_model_access* cxt) : formula_name_resolver(), mp_cxt(cxt) {}
    virtual ~excel_a1() {}

    virtual formula_name_type resolve(const char* p, size_t n, const abs_address_t& pos) const
    {
#if DEBUG_NAME_RESOLVER
        __IXION_DEBUG_OUT__ << "name=" << string(p,n) << "; origin=" << pos.get_name() << endl;
#endif
        formula_name_type ret;
        if (!n)
            return ret;

        if (resolve_function(p, n, ret))
            return ret;

        if (resolve_table(mp_cxt, p, n, ret))
            return ret;

        const char* p_last = p;
        std::advance(p_last, n-1);

        // Use the sheet where the cell is unless sheet name is explicitly given.
        address_t parsed_addr(pos.sheet, 0, 0, false, false, false);

        parse_address_result parse_res = parse_address_excel_a1(mp_cxt, p, p_last, parsed_addr);

#if DEBUG_NAME_RESOLVER
        __IXION_DEBUG_OUT__ << "parse address result: " << parse_address_result_names[parse_res] << endl;
#endif

        // prevent for example H to be recognized as column address
        if (parse_res == valid_address && parsed_addr.row != row_unset)
        {
            // This is a single cell address.
            to_relative_address(parsed_addr, pos);
            set_cell_reference(ret, parsed_addr);

#if DEBUG_NAME_RESOLVER
            string abs_row_s = parsed_addr.abs_row ? "abs" : "rel";
            string abs_col_s = parsed_addr.abs_column ? "abs" : "rel";
            cout << "resolve: " << string(p,n) << "=(row=" << parsed_addr.row
                << " [" << abs_row_s << "]; column=" << parsed_addr.column << " [" << abs_col_s << "])" << endl;
#endif
            return ret;
        }

        if (parse_res == range_expected)
        {
            if (p == p_last)
                // ':' occurs as the last character.  This is not allowed.
                return ret;

            ++p; // skip ':'

            to_relative_address(parsed_addr, pos);
            set_address(ret.range.first, parsed_addr);

            // For now, we assume the sheet index of the end address is identical
            // to that of the begin address.
            parse_res = parse_address_excel_a1(NULL, p, p_last, parsed_addr);
            if (parse_res != valid_address)
                // The 2nd part after the ':' is not valid.
                return ret;

            to_relative_address(parsed_addr, pos);
            set_address(ret.range.last, parsed_addr);
            ret.range.last.sheet = ret.range.first.sheet; // re-use the sheet index of the begin address.
            ret.type = formula_name_type::range_reference;
            return ret;
        }

        resolve_function_or_name(p, n, ret);
        return ret;
    }

    virtual string get_name(const address_t& addr, const abs_address_t& pos, bool sheet_name) const
    {
        ostringstream os;
        char sheet_name_sep = sheet_name ? '!' : 0;
        append_address_a1(os, mp_cxt, addr, pos, sheet_name_sep);
        return os.str();
    }

    virtual string get_name(const range_t& range, const abs_address_t& pos, bool sheet_name) const
    {
        // For now, sheet index of the end-range address is ignored.

        ostringstream os;
        col_t col = range.first.column;
        row_t row = range.first.row;
        sheet_t sheet = range.first.sheet;

        if (!range.first.abs_sheet)
            sheet += pos.sheet;

        if (sheet_name && mp_cxt)
        {
            write_sheet_name(os, *mp_cxt, sheet);
            os << '!';
        }

        if (col != column_unset)
        {
            if (range.first.abs_column)
                os << '$';
            else
                col += pos.column;
            append_column_name_a1(os, col);
        }

        if (row != row_unset)
        {
            if (range.first.abs_row)
                os << '$';
            else
                row += pos.row;
            os << (row + 1);
        }

        os << ":";
        col = range.last.column;
        row = range.last.row;

        if (col != column_unset)
        {
            if (range.last.abs_column)
                os << '$';
            else
                col += pos.column;
            append_column_name_a1(os, col);
        }

        if (row != row_unset)
        {
            if (range.last.abs_row)
                os << '$';
            else
                row += pos.row;
            os << (row + 1);
        }

        return os.str();
    }

    virtual string get_name(const table_t& table) const
    {
        ostringstream os;
        append_name_string(os, mp_cxt, table.name);

        if (table.column_first == empty_string_id)
        {
            // Area specifier(s) only.
            bool headers = (table.areas & table_area_headers);
            bool data = (table.areas & table_area_data);
            bool totals = (table.areas & table_area_totals);

            short count = 0;
            if (headers)
                ++count;
            if (data)
                ++count;
            if (totals)
                ++count;

            bool multiple = count == 2;
            if (multiple)
                os << '[';

            append_table_areas(os, table);

            if (multiple)
                os << ']';
        }
        else if (table.column_last == empty_string_id)
        {
            // single column.
            os << '[';

            bool multiple = false;
            if (table.areas && table.areas != table_area_data)
            {
                if (append_table_areas(os, table))
                {
                    os << ',';
                    multiple = true;
                }
            }

            if (multiple)
                os << '[';

            append_name_string(os, mp_cxt, table.column_first);

            if (multiple)
                os << ']';

            os << ']';
        }
        else
        {
            // column range.
            os << '[';

            if (table.areas && table.areas != table_area_data)
            {
                if (append_table_areas(os, table))
                    os << ',';
            }

            os << '[';
            append_name_string(os, mp_cxt, table.column_first);
            os << "]:[";
            append_name_string(os, mp_cxt, table.column_last);
            os << "]]";
        }

        return os.str();
    }

    virtual string get_column_name(col_t col) const
    {
        return get_column_name_a1(col);
    }

private:
    const iface::formula_model_access* mp_cxt;
};

class excel_r1c1 : public formula_name_resolver
{
    const iface::formula_model_access* mp_cxt;
public:
    excel_r1c1(const iface::formula_model_access* cxt) : mp_cxt(cxt) {}

    virtual formula_name_type resolve(const char* p, size_t n, const abs_address_t& pos) const
    {
        formula_name_type ret;
        if (!n)
            return ret;

        if (resolve_function(p, n, ret))
            return ret;

        const char* p_last = p;
        std::advance(p_last, n-1);

        // Use the sheet where the cell is unless sheet name is explicitly given.
        address_t parsed_addr(pos.sheet, 0, 0, false, false, false);

        parse_address_result parse_res = parse_address_excel_r1c1(mp_cxt, p, p_last, parsed_addr);
        switch (parse_res)
        {
            case parse_address_result::valid_address:
            {
                set_cell_reference(ret, parsed_addr);
                return ret;
            }
            break;
        }

        return ret;
    }

    virtual std::string get_name(const address_t& addr, const abs_address_t& pos, bool sheet_name) const
    {
        ostringstream os;

        if (sheet_name && mp_cxt)
        {
            sheet_t sheet = addr.sheet;
            if (!addr.abs_sheet)
                sheet += pos.sheet;

            write_sheet_name(os, *mp_cxt, sheet);
            os << '!';
        }

        if (addr.row != row_unset)
        {
            os << 'R';
            if (addr.abs_row)
                // absolute row address.
                os << (addr.row+1);
            else if (addr.row)
            {
                // relative row address different from origin.
                os << '[';
                os << addr.row;
                os << ']';
            }

        }
        if (addr.column != column_unset)
        {
            os << 'C';
            if (addr.abs_column)
                os << (addr.column+1);
            else if (addr.column)
            {
                os << '[';
                os << addr.column;
                os << ']';
            }
        }

        return os.str();
    }

    virtual std::string get_name(const range_t& range, const abs_address_t& pos, bool sheet_name) const
    {
        return std::string();
    }

    virtual std::string get_name(const table_t& table) const
    {
        return std::string();
    }

    virtual std::string get_column_name(col_t col) const
    {
        return std::string();
    }
};

/**
 * Name resolver for ODFF formula expressions.
 */
class odff_resolver : public formula_name_resolver
{
public:
    odff_resolver(const iface::formula_model_access* cxt) : formula_name_resolver(), mp_cxt(cxt) {}
    virtual ~odff_resolver() {}

    virtual formula_name_type resolve(const char* p, size_t n, const abs_address_t& pos) const
    {
        formula_name_type ret;

        if (resolve_function(p, n, ret))
            return ret;

        if (!n)
            // Empty string.
            return ret;

        // First character must be '['.
        if (*p != '[')
            return ret;

        ++p;
        const char* p_last = p;
        std::advance(p_last, n-2);

        // Last character must be ']'.
        if (*p_last != ']')
            return ret;

        --p_last;

        // Use the sheet where the cell is unless sheet name is explicitly given.
        address_t parsed_addr(pos.sheet, 0, 0, false, false, false);

        parse_address_result parse_res = parse_address_odff(mp_cxt, p, p_last, parsed_addr);

        // prevent for example H to be recognized as column address
        if (parse_res == valid_address && parsed_addr.row != row_unset)
        {
            // This is a single cell address.
            to_relative_address(parsed_addr, pos);
            set_cell_reference(ret, parsed_addr);
            return ret;
        }

        if (parse_res == range_expected)
        {
            if (p == p_last)
                // ':' occurs as the last character.  This is not allowed.
                return ret;

            ++p; // skip ':'

            to_relative_address(parsed_addr, pos);
            set_address(ret.range.first, parsed_addr);

            // For now, we assume the sheet index of the end address is identical
            // to that of the begin address.
            parse_res = parse_address_odff(NULL, p, p_last, parsed_addr);
            if (parse_res != valid_address)
                // The 2nd part after the ':' is not valid.
                return ret;

            to_relative_address(parsed_addr, pos);
            set_address(ret.range.last, parsed_addr);
            ret.range.last.sheet = ret.range.first.sheet; // re-use the sheet index of the begin address.
            ret.type = formula_name_type::range_reference;
            return ret;
        }

        resolve_function_or_name(p, n, ret);

        return ret;
    }

    virtual string get_name(const address_t& addr, const abs_address_t& pos, bool sheet_name) const
    {
        ostringstream os;
        os << '[';
        if (sheet_name)
            append_address_a1(os, mp_cxt, addr, pos, '.');
        else
        {
            os << '.';
            append_address_a1(os, NULL, addr, pos, 0);
        }
        os << ']';
        return os.str();
    }

    virtual string get_name(const range_t& range, const abs_address_t& pos, bool sheet_name) const
    {
        ostringstream os;
        os << '[';

        if (sheet_name)
        {
            append_address_a1(os, mp_cxt, range.first, pos, '.');
            os << ':';
            append_address_a1(os, mp_cxt, range.last, pos, '.');
        }
        else
        {
            os << '.';
            append_address_a1(os, NULL, range.first, pos, 0);
            os << ":.";
            append_address_a1(os, NULL, range.last, pos, 0);
        }

        os << ']';
        return os.str();
    }

    virtual string get_name(const table_t& table) const
    {
        // TODO : ODF doesn't support table reference yet.
        return string();
    }

    virtual string get_column_name(col_t col) const
    {
        return get_column_name_a1(col);
    }

private:
    const iface::formula_model_access* mp_cxt;
};

}

std::unique_ptr<formula_name_resolver> formula_name_resolver::get(
    formula_name_resolver_t type, const iface::formula_model_access* cxt)
{

    switch (type)
    {
        case formula_name_resolver_excel_a1:
            return std::unique_ptr<formula_name_resolver>(new excel_a1(cxt));
        case formula_name_resolver_excel_r1c1:
            return std::unique_ptr<formula_name_resolver>(new excel_r1c1(cxt));
        case formula_name_resolver_odff:
            return std::unique_ptr<formula_name_resolver>(new odff_resolver(cxt));
        case formula_name_resolver_calc_a1:
        case formula_name_resolver_unknown:
        default:
            ;
    }

    return std::unique_ptr<formula_name_resolver>();
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
