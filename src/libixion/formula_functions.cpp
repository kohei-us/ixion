/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_functions.hpp"

#include "ixion/formula_tokens.hpp"
#include "ixion/matrix.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/interface/formula_model_access.hpp"
#include "ixion/macros.hpp"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#define DEBUG_FORMULA_FUNCTIONS 0

#include <cassert>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#include <mdds/sorted_string_map.hpp>

using namespace std;

namespace ixion {

namespace {

namespace builtin_funcs {

typedef mdds::sorted_string_map<formula_function_t> map_type;

// Keys must be sorted.
const std::vector<map_type::entry> entries =
{
    { IXION_ASCII("AVERAGE"),     formula_function_t::func_average     },
    { IXION_ASCII("CONCATENATE"), formula_function_t::func_concatenate },
    { IXION_ASCII("COUNTA"),      formula_function_t::func_counta      },
    { IXION_ASCII("IF"),          formula_function_t::func_if          },
    { IXION_ASCII("LEN"),         formula_function_t::func_len         },
    { IXION_ASCII("MAX"),         formula_function_t::func_max         },
    { IXION_ASCII("MIN"),         formula_function_t::func_min         },
    { IXION_ASCII("MMULT"),       formula_function_t::func_mmult       },
    { IXION_ASCII("NOW"),         formula_function_t::func_now         },
    { IXION_ASCII("SUBTOTAL"),    formula_function_t::func_subtotal    },
    { IXION_ASCII("SUM"),         formula_function_t::func_sum         },
    { IXION_ASCII("WAIT"),        formula_function_t::func_wait        },
};

const map_type& get()
{
    static map_type mt(entries.data(), entries.size(), formula_function_t::func_unknown);
    return mt;
}


} // anonymous namespace

const char* unknown_func_name = "unknown";

/**
 * Traverse all elements of a passed matrix to sum up their values.
 */
double sum_matrix_elements(const matrix& mx)
{
    double sum = 0.0;
    size_t rows = mx.row_size();
    size_t cols = mx.col_size();
    for (size_t row = 0; row < rows; ++row)
        for (size_t col = 0; col < cols; ++col)
            sum += mx.get_numeric(row, col);

    return sum;
}

numeric_matrix multiply_matrices(const matrix& left, const matrix& right)
{
    // The column size of the left matrix must equal the row size of the right
    // matrix.

    size_t n = left.col_size();

    if (n != right.row_size())
        throw formula_error(formula_error_t::invalid_expression);

    numeric_matrix left_nm = left.as_numeric();
    numeric_matrix right_nm = right.as_numeric();

    numeric_matrix output(left_nm.row_size(), right_nm.col_size());

    for (size_t row = 0; row < output.row_size(); ++row)
    {
        for (size_t col = 0; col < output.col_size(); ++col)
        {
            double v = 0.0;
            for (size_t i = 0; i < n; ++i)
                v += left_nm(row, i) * right_nm(i, col);

            output(row, col) = v;
        }
    }

    return output;
}

}

// ============================================================================

formula_functions::invalid_arg::invalid_arg(const string& msg) :
    general_error(msg) {}

formula_function_t formula_functions::get_function_opcode(const formula_token& token)
{
    assert(token.get_opcode() == fop_function);
    return static_cast<formula_function_t>(token.get_index());
}

formula_function_t formula_functions::get_function_opcode(const char* p, size_t n)
{
    std::string upper;

    for (const char* p_end = p + n; p != p_end; ++p)
    {
        char c = *p;

        if (c > 'Z')
        {
            // Convert to upper case.
            c -= 'a' - 'A';
        }

        upper.push_back(c);
    }

    return builtin_funcs::get().find(upper.data(), upper.size());
}

const char* formula_functions::get_function_name(formula_function_t oc)
{
    for (const builtin_funcs::map_type::entry& e : builtin_funcs::entries)
    {
        if (e.value == oc)
            return e.key;
    }
    return unknown_func_name;
}

formula_functions::formula_functions(iface::formula_model_access& cxt) :
    m_context(cxt)
{
}

formula_functions::~formula_functions()
{
}

void formula_functions::interpret(formula_function_t oc, value_stack_t& args)
{
    switch (oc)
    {
        case formula_function_t::func_max:
            fnc_max(args);
            break;
        case formula_function_t::func_average:
            fnc_average(args);
            break;
        case formula_function_t::func_min:
            fnc_min(args);
            break;
        case formula_function_t::func_wait:
            fnc_wait(args);
            break;
        case formula_function_t::func_sum:
            fnc_sum(args);
            break;
        case formula_function_t::func_counta:
            fnc_counta(args);
            break;
        case formula_function_t::func_if:
            fnc_if(args);
            break;
        case formula_function_t::func_len:
            fnc_len(args);
            break;
        case formula_function_t::func_concatenate:
            fnc_concatenate(args);
            break;
        case formula_function_t::func_now:
            fnc_now(args);
            break;
        case formula_function_t::func_subtotal:
            fnc_subtotal(args);
            break;
        case formula_function_t::func_mmult:
            fnc_mmult(args);
            break;
        case formula_function_t::func_unknown:
        default:
            throw formula_functions::invalid_arg("unknown function opcode");
    }
}

void formula_functions::fnc_max(value_stack_t& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("MAX requires one or more arguments.");

    double ret = args.pop_value();
    while (!args.empty())
    {
        double v = args.pop_value();
        if (v > ret)
            ret = v;
    }
    args.push_value(ret);
}

void formula_functions::fnc_min(value_stack_t& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("MIN requires one or more arguments.");

    double ret = args.pop_value();
    while (!args.empty())
    {
        double v = args.pop_value();
        if (v < ret)
            ret = v;
    }
    args.push_value(ret);
}

void formula_functions::fnc_sum(value_stack_t& args) const
{
#if DEBUG_FORMULA_FUNCTIONS
    __IXION_DEBUG_OUT__ << "function: sum" << endl;
#endif
    if (args.empty())
        throw formula_functions::invalid_arg("SUM requires one or more arguments.");

    double ret = 0;
    while (!args.empty())
    {
        switch (args.get_type())
        {
            case stack_value_t::range_ref:
                ret += sum_matrix_elements(args.pop_range_value());
            break;
            case stack_value_t::single_ref:
            case stack_value_t::string:
            case stack_value_t::value:
            default:
                ret += args.pop_value();
        }
    }

    args.push_value(ret);

#if DEBUG_FORMULA_FUNCTIONS
    __IXION_DEBUG_OUT__ << "function: sum end (result=" << ret << ")" << endl;
#endif
}

void formula_functions::fnc_counta(value_stack_t& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("COUNTA requires one or more arguments.");

    double ret = 0;
    while (!args.empty())
    {
        switch (args.get_type())
        {
            case stack_value_t::string:
            case stack_value_t::value:
                args.pop_value();
                ++ret;
            break;
            case stack_value_t::range_ref:
            {
                abs_range_t range = args.pop_range_ref();
                ret += m_context.count_range(range, value_numeric | value_boolean | value_string);
            }
            break;
            case stack_value_t::single_ref:
            {
                abs_address_t pos = args.pop_single_ref();
                abs_range_t range;
                range.first = range.last = pos;
                ret += m_context.count_range(range, value_numeric | value_boolean | value_string);
            }
            break;
            default:
                args.pop_value();
        }

    }

    args.push_value(ret);
}

void formula_functions::fnc_average(value_stack_t& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("AVERAGE requires one or more arguments.");

    double ret = 0;
    double count = 0.0;
    while (!args.empty())
    {
        switch (args.get_type())
        {
            case stack_value_t::range_ref:
            {
                matrix mx = args.pop_range_value();
                size_t rows = mx.row_size();
                size_t cols = mx.col_size();

                for (size_t r = 0; r < rows; ++r)
                {
                    for (size_t c = 0; c < cols; ++c)
                    {
                        if (!mx.is_numeric(r, c))
                            continue;

                        ret += mx.get_numeric(r, c);
                        ++count;
                    }
                }
            }
            break;
            case stack_value_t::single_ref:
            case stack_value_t::string:
            case stack_value_t::value:
            default:
                ret += args.pop_value();
                ++count;
        }
    }

    args.push_value(ret/count);
}

void formula_functions::fnc_mmult(value_stack_t& args) const
{
    matrix mx[2];
    matrix* mxp = mx;
    const matrix* mxp_end = mxp + 2;

    bool invalid_arg = false;

    // NB : the stack is FIFO i.e. the first matrix is the right matrix and
    // the second one is the left one.

    while (!args.empty())
    {
        switch (args.get_type())
        {
            case stack_value_t::range_ref:
            {
                if (mxp == mxp_end)
                {
                    invalid_arg = true;
                    break;
                }

                matrix m = args.pop_range_value();
                mxp->swap(m);
                ++mxp;
                break;
            }
            default:
                invalid_arg = true;
        }

        if (invalid_arg)
            break;
    }

    if (mxp != mxp_end)
        invalid_arg = true;

    if (invalid_arg)
        throw formula_functions::invalid_arg("MMULT requires exactly two ranges.");

    mx[0].swap(mx[1]); // Make it so that 0 -> left and 1 -> right.

    if (!mx[0].is_numeric() || !mx[1].is_numeric())
        throw formula_functions::invalid_arg(
            "MMULT requires two numeric ranges. At least one range is not numeric.");

    numeric_matrix ans = multiply_matrices(mx[0], mx[1]);

    args.push_matrix(ans);
}

void formula_functions::fnc_if(value_stack_t& args) const
{
    if (args.size() != 3)
        throw formula_functions::invalid_arg("IF requires exactly 3 arguments.");

    value_stack_t::iterator pos = args.begin();
    bool eval = args.get_value(0) != 0.0;
    if (eval)
        std::advance(pos, 1);
    else
        std::advance(pos, 2);

    value_stack_t ret(m_context);
    ret.push_back(args.release(pos));
    args.swap(ret);
}

void formula_functions::fnc_len(value_stack_t& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("LEN requires exactly one argument.");

    string s = args.pop_string();
    args.clear();
    args.push_value(s.size());
}

void formula_functions::fnc_concatenate(value_stack_t& args)
{
    string s;
    while (!args.empty())
        s = args.pop_string() + s;
    size_t sid = m_context.add_string(&s[0], s.size());
    args.push_string(sid);
}

void formula_functions::fnc_now(value_stack_t& args) const
{
    if (!args.empty())
        throw formula_functions::invalid_arg("NOW takes no argument.");

    // TODO: this value is currently not accurate since we don't take into
    // account the zero date yet.
    double cur_time = global::get_current_time();
    cur_time /= 86400.0; // convert seconds to days.
    args.push_value(cur_time);
}

void formula_functions::fnc_wait(value_stack_t& args) const
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    args.clear();
    args.push_value(1);
}

void formula_functions::fnc_subtotal(value_stack_t& args) const
{
    if (args.size() != 2)
        throw formula_functions::invalid_arg("SUBTOTAL requires exactly 2 arguments.");

    abs_range_t range = args.pop_range_ref();
    int subtype = args.pop_value();
    switch (subtype)
    {
        case 109:
        {
            // SUM
            matrix mx = m_context.get_range_value(range);
            args.push_value(sum_matrix_elements(mx));
        }
        break;
        default:
            throw formula_functions::invalid_arg("not implemented yet");
    }
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
