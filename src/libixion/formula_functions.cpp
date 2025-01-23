/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_functions.hpp"
#include "debug.hpp"
#include "column_store_type.hpp" // internal mdds::multi_type_vector
#include "utils.hpp"
#include "utf8.hpp"

#include <ixion/formula_tokens.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/matrix.hpp>
#include <ixion/model_iterator.hpp>
#include <ixion/cell_access.hpp>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include <cassert>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <cmath>
#include <optional>
#include <iterator>

#include <mdds/sorted_string_map.hpp>

namespace ixion {

namespace {

namespace builtin_funcs {

using map_type = mdds::sorted_string_map<formula_function_t, mdds::ssmap::hash_key_finder>;

// Keys must be sorted.
constexpr map_type::entry_type entries[] =
{
    { "ABS", formula_function_t::func_abs },
    { "ACOS", formula_function_t::func_acos },
    { "ACOSH", formula_function_t::func_acosh },
    { "ACOT", formula_function_t::func_acot },
    { "ACOTH", formula_function_t::func_acoth },
    { "ADDRESS", formula_function_t::func_address },
    { "AGGREGATE", formula_function_t::func_aggregate },
    { "AND", formula_function_t::func_and },
    { "ARABIC", formula_function_t::func_arabic },
    { "AREAS", formula_function_t::func_areas },
    { "ASC", formula_function_t::func_asc },
    { "ASIN", formula_function_t::func_asin },
    { "ASINH", formula_function_t::func_asinh },
    { "ATAN", formula_function_t::func_atan },
    { "ATAN2", formula_function_t::func_atan2 },
    { "ATANH", formula_function_t::func_atanh },
    { "AVEDEV", formula_function_t::func_avedev },
    { "AVERAGE", formula_function_t::func_average },
    { "AVERAGEA", formula_function_t::func_averagea },
    { "AVERAGEIF", formula_function_t::func_averageif },
    { "AVERAGEIFS", formula_function_t::func_averageifs },
    { "B", formula_function_t::func_b },
    { "BAHTTEXT", formula_function_t::func_bahttext },
    { "BASE", formula_function_t::func_base },
    { "BETADIST", formula_function_t::func_betadist },
    { "BETAINV", formula_function_t::func_betainv },
    { "BINOMDIST", formula_function_t::func_binomdist },
    { "BITAND", formula_function_t::func_bitand },
    { "BITLSHIFT", formula_function_t::func_bitlshift },
    { "BITOR", formula_function_t::func_bitor },
    { "BITRSHIFT", formula_function_t::func_bitrshift },
    { "BITXOR", formula_function_t::func_bitxor },
    { "CEILING", formula_function_t::func_ceiling },
    { "CELL", formula_function_t::func_cell },
    { "CHAR", formula_function_t::func_char },
    { "CHIDIST", formula_function_t::func_chidist },
    { "CHIINV", formula_function_t::func_chiinv },
    { "CHISQDIST", formula_function_t::func_chisqdist },
    { "CHISQINV", formula_function_t::func_chisqinv },
    { "CHITEST", formula_function_t::func_chitest },
    { "CHOOSE", formula_function_t::func_choose },
    { "CLEAN", formula_function_t::func_clean },
    { "CODE", formula_function_t::func_code },
    { "COLOR", formula_function_t::func_color },
    { "COLUMN", formula_function_t::func_column },
    { "COLUMNS", formula_function_t::func_columns },
    { "COMBIN", formula_function_t::func_combin },
    { "COMBINA", formula_function_t::func_combina },
    { "CONCAT", formula_function_t::func_concat },
    { "CONCATENATE", formula_function_t::func_concatenate },
    { "CONFIDENCE", formula_function_t::func_confidence },
    { "CORREL", formula_function_t::func_correl },
    { "COS", formula_function_t::func_cos },
    { "COSH", formula_function_t::func_cosh },
    { "COT", formula_function_t::func_cot },
    { "COTH", formula_function_t::func_coth },
    { "COUNT", formula_function_t::func_count },
    { "COUNTA", formula_function_t::func_counta },
    { "COUNTBLANK", formula_function_t::func_countblank },
    { "COUNTIF", formula_function_t::func_countif },
    { "COUNTIFS", formula_function_t::func_countifs },
    { "COVAR", formula_function_t::func_covar },
    { "CRITBINOM", formula_function_t::func_critbinom },
    { "CSC", formula_function_t::func_csc },
    { "CSCH", formula_function_t::func_csch },
    { "CUMIPMT", formula_function_t::func_cumipmt },
    { "CUMPRINC", formula_function_t::func_cumprinc },
    { "CURRENT", formula_function_t::func_current },
    { "DATE", formula_function_t::func_date },
    { "DATEDIF", formula_function_t::func_datedif },
    { "DATEVALUE", formula_function_t::func_datevalue },
    { "DAVERAGE", formula_function_t::func_daverage },
    { "DAY", formula_function_t::func_day },
    { "DAYS", formula_function_t::func_days },
    { "DAYS360", formula_function_t::func_days360 },
    { "DB", formula_function_t::func_db },
    { "DCOUNT", formula_function_t::func_dcount },
    { "DCOUNTA", formula_function_t::func_dcounta },
    { "DDB", formula_function_t::func_ddb },
    { "DDE", formula_function_t::func_dde },
    { "DECIMAL", formula_function_t::func_decimal },
    { "DEGREES", formula_function_t::func_degrees },
    { "DEVSQ", formula_function_t::func_devsq },
    { "DGET", formula_function_t::func_dget },
    { "DMAX", formula_function_t::func_dmax },
    { "DMIN", formula_function_t::func_dmin },
    { "DOLLAR", formula_function_t::func_dollar },
    { "DPRODUCT", formula_function_t::func_dproduct },
    { "DSTDEV", formula_function_t::func_dstdev },
    { "DSTDEVP", formula_function_t::func_dstdevp },
    { "DSUM", formula_function_t::func_dsum },
    { "DVAR", formula_function_t::func_dvar },
    { "DVARP", formula_function_t::func_dvarp },
    { "EASTERSUNDAY", formula_function_t::func_eastersunday },
    { "EFFECT", formula_function_t::func_effect },
    { "ENCODEURL", formula_function_t::func_encodeurl },
    { "ERRORTYPE", formula_function_t::func_errortype },
    { "EUROCONVERT", formula_function_t::func_euroconvert },
    { "EVEN", formula_function_t::func_even },
    { "EXACT", formula_function_t::func_exact },
    { "EXP", formula_function_t::func_exp },
    { "EXPONDIST", formula_function_t::func_expondist },
    { "FACT", formula_function_t::func_fact },
    { "FALSE", formula_function_t::func_false },
    { "FDIST", formula_function_t::func_fdist },
    { "FILTERXML", formula_function_t::func_filterxml },
    { "FIND", formula_function_t::func_find },
    { "FINDB", formula_function_t::func_findb },
    { "FINV", formula_function_t::func_finv },
    { "FISHER", formula_function_t::func_fisher },
    { "FISHERINV", formula_function_t::func_fisherinv },
    { "FIXED", formula_function_t::func_fixed },
    { "FLOOR", formula_function_t::func_floor },
    { "FORECAST", formula_function_t::func_forecast },
    { "FORMULA", formula_function_t::func_formula },
    { "FOURIER", formula_function_t::func_fourier },
    { "FREQUENCY", formula_function_t::func_frequency },
    { "FTEST", formula_function_t::func_ftest },
    { "FV", formula_function_t::func_fv },
    { "GAMMA", formula_function_t::func_gamma },
    { "GAMMADIST", formula_function_t::func_gammadist },
    { "GAMMAINV", formula_function_t::func_gammainv },
    { "GAMMALN", formula_function_t::func_gammaln },
    { "GAUSS", formula_function_t::func_gauss },
    { "GCD", formula_function_t::func_gcd },
    { "GEOMEAN", formula_function_t::func_geomean },
    { "GETPIVOTDATA", formula_function_t::func_getpivotdata },
    { "GOALSEEK", formula_function_t::func_goalseek },
    { "GROWTH", formula_function_t::func_growth },
    { "HARMEAN", formula_function_t::func_harmean },
    { "HLOOKUP", formula_function_t::func_hlookup },
    { "HOUR", formula_function_t::func_hour },
    { "HYPERLINK", formula_function_t::func_hyperlink },
    { "HYPGEOMDIST", formula_function_t::func_hypgeomdist },
    { "IF", formula_function_t::func_if },
    { "IFERROR", formula_function_t::func_iferror },
    { "IFNA", formula_function_t::func_ifna },
    { "IFS", formula_function_t::func_ifs },
    { "INDEX", formula_function_t::func_index },
    { "INDIRECT", formula_function_t::func_indirect },
    { "INFO", formula_function_t::func_info },
    { "INT", formula_function_t::func_int },
    { "INTERCEPT", formula_function_t::func_intercept },
    { "IPMT", formula_function_t::func_ipmt },
    { "IRR", formula_function_t::func_irr },
    { "ISBLANK", formula_function_t::func_isblank },
    { "ISERR", formula_function_t::func_iserr },
    { "ISERROR", formula_function_t::func_iserror },
    { "ISEVEN", formula_function_t::func_iseven },
    { "ISFORMULA", formula_function_t::func_isformula },
    { "ISLOGICAL", formula_function_t::func_islogical },
    { "ISNA", formula_function_t::func_isna },
    { "ISNONTEXT", formula_function_t::func_isnontext },
    { "ISNUMBER", formula_function_t::func_isnumber },
    { "ISODD", formula_function_t::func_isodd },
    { "ISOWEEKNUM", formula_function_t::func_isoweeknum },
    { "ISPMT", formula_function_t::func_ispmt },
    { "ISREF", formula_function_t::func_isref },
    { "ISTEXT", formula_function_t::func_istext },
    { "JIS", formula_function_t::func_jis },
    { "KURT", formula_function_t::func_kurt },
    { "LARGE", formula_function_t::func_large },
    { "LCM", formula_function_t::func_lcm },
    { "LEFT", formula_function_t::func_left },
    { "LEFTB", formula_function_t::func_leftb },
    { "LEN", formula_function_t::func_len },
    { "LENB", formula_function_t::func_lenb },
    { "LINEST", formula_function_t::func_linest },
    { "LN", formula_function_t::func_ln },
    { "LOG", formula_function_t::func_log },
    { "LOG10", formula_function_t::func_log10 },
    { "LOGEST", formula_function_t::func_logest },
    { "LOGINV", formula_function_t::func_loginv },
    { "LOGNORMDIST", formula_function_t::func_lognormdist },
    { "LOOKUP", formula_function_t::func_lookup },
    { "LOWER", formula_function_t::func_lower },
    { "MATCH", formula_function_t::func_match },
    { "MAX", formula_function_t::func_max },
    { "MAXA", formula_function_t::func_maxa },
    { "MAXIFS", formula_function_t::func_maxifs },
    { "MDETERM", formula_function_t::func_mdeterm },
    { "MEDIAN", formula_function_t::func_median },
    { "MID", formula_function_t::func_mid },
    { "MIDB", formula_function_t::func_midb },
    { "MIN", formula_function_t::func_min },
    { "MINA", formula_function_t::func_mina },
    { "MINIFS", formula_function_t::func_minifs },
    { "MINUTE", formula_function_t::func_minute },
    { "MINVERSE", formula_function_t::func_minverse },
    { "MIRR", formula_function_t::func_mirr },
    { "MMULT", formula_function_t::func_mmult },
    { "MOD", formula_function_t::func_mod },
    { "MODE", formula_function_t::func_mode },
    { "MONTH", formula_function_t::func_month },
    { "MULTIRANGE", formula_function_t::func_multirange },
    { "MUNIT", formula_function_t::func_munit },
    { "MVALUE", formula_function_t::func_mvalue },
    { "N", formula_function_t::func_n },
    { "NA", formula_function_t::func_na },
    { "NEG", formula_function_t::func_neg },
    { "NEGBINOMDIST", formula_function_t::func_negbinomdist },
    { "NETWORKDAYS", formula_function_t::func_networkdays },
    { "NOMINAL", formula_function_t::func_nominal },
    { "NORMDIST", formula_function_t::func_normdist },
    { "NORMINV", formula_function_t::func_norminv },
    { "NORMSDIST", formula_function_t::func_normsdist },
    { "NORMSINV", formula_function_t::func_normsinv },
    { "NOT", formula_function_t::func_not },
    { "NOW", formula_function_t::func_now },
    { "NPER", formula_function_t::func_nper },
    { "NPV", formula_function_t::func_npv },
    { "NUMBERVALUE", formula_function_t::func_numbervalue },
    { "ODD", formula_function_t::func_odd },
    { "OFFSET", formula_function_t::func_offset },
    { "OR", formula_function_t::func_or },
    { "PDURATION", formula_function_t::func_pduration },
    { "PEARSON", formula_function_t::func_pearson },
    { "PERCENTILE", formula_function_t::func_percentile },
    { "PERCENTRANK", formula_function_t::func_percentrank },
    { "PERMUT", formula_function_t::func_permut },
    { "PERMUTATIONA", formula_function_t::func_permutationa },
    { "PHI", formula_function_t::func_phi },
    { "PI", formula_function_t::func_pi },
    { "PMT", formula_function_t::func_pmt },
    { "POISSON", formula_function_t::func_poisson },
    { "POWER", formula_function_t::func_power },
    { "PPMT", formula_function_t::func_ppmt },
    { "PROB", formula_function_t::func_prob },
    { "PRODUCT", formula_function_t::func_product },
    { "PROPER", formula_function_t::func_proper },
    { "PV", formula_function_t::func_pv },
    { "QUARTILE", formula_function_t::func_quartile },
    { "RADIANS", formula_function_t::func_radians },
    { "RAND", formula_function_t::func_rand },
    { "RANK", formula_function_t::func_rank },
    { "RATE", formula_function_t::func_rate },
    { "RAWSUBTRACT", formula_function_t::func_rawsubtract },
    { "REGEX", formula_function_t::func_regex },
    { "REPLACE", formula_function_t::func_replace },
    { "REPLACEB", formula_function_t::func_replaceb },
    { "REPT", formula_function_t::func_rept },
    { "RIGHT", formula_function_t::func_right },
    { "RIGHTB", formula_function_t::func_rightb },
    { "ROMAN", formula_function_t::func_roman },
    { "ROUND", formula_function_t::func_round },
    { "ROUNDDOWN", formula_function_t::func_rounddown },
    { "ROUNDSIG", formula_function_t::func_roundsig },
    { "ROUNDUP", formula_function_t::func_roundup },
    { "ROW", formula_function_t::func_row },
    { "ROWS", formula_function_t::func_rows },
    { "RRI", formula_function_t::func_rri },
    { "RSQ", formula_function_t::func_rsq },
    { "SEARCH", formula_function_t::func_search },
    { "SEARCHB", formula_function_t::func_searchb },
    { "SEC", formula_function_t::func_sec },
    { "SECH", formula_function_t::func_sech },
    { "SECOND", formula_function_t::func_second },
    { "SHEET", formula_function_t::func_sheet },
    { "SHEETS", formula_function_t::func_sheets },
    { "SIGN", formula_function_t::func_sign },
    { "SIN", formula_function_t::func_sin },
    { "SINH", formula_function_t::func_sinh },
    { "SKEW", formula_function_t::func_skew },
    { "SKEWP", formula_function_t::func_skewp },
    { "SLN", formula_function_t::func_sln },
    { "SLOPE", formula_function_t::func_slope },
    { "SMALL", formula_function_t::func_small },
    { "SQRT", formula_function_t::func_sqrt },
    { "STANDARDIZE", formula_function_t::func_standardize },
    { "STDEV", formula_function_t::func_stdev },
    { "STDEVA", formula_function_t::func_stdeva },
    { "STDEVP", formula_function_t::func_stdevp },
    { "STDEVPA", formula_function_t::func_stdevpa },
    { "STEYX", formula_function_t::func_steyx },
    { "STYLE", formula_function_t::func_style },
    { "SUBSTITUTE", formula_function_t::func_substitute },
    { "SUBTOTAL", formula_function_t::func_subtotal },
    { "SUM", formula_function_t::func_sum },
    { "SUMIF", formula_function_t::func_sumif },
    { "SUMIFS", formula_function_t::func_sumifs },
    { "SUMPRODUCT", formula_function_t::func_sumproduct },
    { "SUMSQ", formula_function_t::func_sumsq },
    { "SUMX2MY2", formula_function_t::func_sumx2my2 },
    { "SUMX2PY2", formula_function_t::func_sumx2py2 },
    { "SUMXMY2", formula_function_t::func_sumxmy2 },
    { "SWITCH", formula_function_t::func_switch },
    { "SYD", formula_function_t::func_syd },
    { "T", formula_function_t::func_t },
    { "TAN", formula_function_t::func_tan },
    { "TANH", formula_function_t::func_tanh },
    { "TDIST", formula_function_t::func_tdist },
    { "TEXT", formula_function_t::func_text },
    { "TEXTJOIN", formula_function_t::func_textjoin },
    { "TIME", formula_function_t::func_time },
    { "TIMEVALUE", formula_function_t::func_timevalue },
    { "TINV", formula_function_t::func_tinv },
    { "TODAY", formula_function_t::func_today },
    { "TRANSPOSE", formula_function_t::func_transpose },
    { "TREND", formula_function_t::func_trend },
    { "TRIM", formula_function_t::func_trim },
    { "TRIMMEAN", formula_function_t::func_trimmean },
    { "TRUE", formula_function_t::func_true },
    { "TRUNC", formula_function_t::func_trunc },
    { "TTEST", formula_function_t::func_ttest },
    { "TYPE", formula_function_t::func_type },
    { "UNICHAR", formula_function_t::func_unichar },
    { "UNICODE", formula_function_t::func_unicode },
    { "UPPER", formula_function_t::func_upper },
    { "VALUE", formula_function_t::func_value },
    { "VAR", formula_function_t::func_var },
    { "VARA", formula_function_t::func_vara },
    { "VARP", formula_function_t::func_varp },
    { "VARPA", formula_function_t::func_varpa },
    { "VDB", formula_function_t::func_vdb },
    { "VLOOKUP", formula_function_t::func_vlookup },
    { "WAIT", formula_function_t::func_wait },
    { "WEBSERVICE", formula_function_t::func_webservice },
    { "WEEKDAY", formula_function_t::func_weekday },
    { "WEEKNUM", formula_function_t::func_weeknum },
    { "WEIBULL", formula_function_t::func_weibull },
    { "XOR", formula_function_t::func_xor },
    { "YEAR", formula_function_t::func_year },
    { "ZTEST", formula_function_t::func_ztest },
};

const map_type& get()
{
    static map_type mt(entries, std::size(entries), formula_function_t::func_unknown);
    return mt;
}

} // builtin_funcs namespace

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

bool pop_and_check_for_odd_value(formula_value_stack& args)
{
    double v = args.pop_value();
    intmax_t iv = std::trunc(v);
    iv = std::abs(iv);
    bool odd = iv & 0x01;
    return odd;
}

/**
 * Pop a single-value argument from the stack and interpret it as a boolean
 * value if it's either boolean or numeric type, or ignore if it's a string or
 * error type.  The behavior is undefined if called for non-single-value
 * argument type.
 */
std::optional<bool> pop_one_value_as_boolean(const model_context& cxt, formula_value_stack& args)
{
    std::optional<bool> ret;

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        {
            auto addr = args.pop_single_ref();
            cell_access ca = cxt.get_cell_access(addr);

            switch (ca.get_value_type())
            {
                case cell_value_t::boolean:
                case cell_value_t::numeric:
                    ret = ca.get_boolean_value();
                    break;
                default:;
            }

            break;
        }
        case stack_value_t::value:
        case stack_value_t::boolean:
            ret = args.pop_boolean();
            break;
        case stack_value_t::string:
        case stack_value_t::error:
            // ignore string type
            args.pop_back();
            break;
        case stack_value_t::range_ref:
        case stack_value_t::matrix:
            // should not be called for non-single value.
            throw formula_error(formula_error_t::general_error);
    }

    return ret;
}

/**
 * Pop a value from the stack, and insert one or more numeric values to the
 * specified sequence container.
 */
template<typename ContT>
void append_values_from_stack(
    const model_context& cxt, formula_value_stack& args, std::back_insert_iterator<ContT> insert_it)
{
    static_assert(
        std::is_floating_point_v<typename ContT::value_type>,
        "this function only supports a container of floating point values.");

    switch (args.get_type())
    {
        case stack_value_t::boolean:
        case stack_value_t::value:
            insert_it = args.pop_value();
            break;
        case stack_value_t::single_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            auto ca = cxt.get_cell_access(addr);
            switch (ca.get_value_type())
            {
                case cell_value_t::boolean:
                    insert_it = ca.get_boolean_value() ? 1.0 : 0.0;
                    break;
                case cell_value_t::numeric:
                    insert_it = ca.get_numeric_value();
                    break;
                default:;
            }
            break;
        }
        case stack_value_t::range_ref:
        {
            const formula_result_wait_policy_t wait_policy = cxt.get_formula_result_wait_policy();
            abs_range_t range = args.pop_range_ref();

            column_block_callback_t cb = [&insert_it, wait_policy](
                col_t col, row_t row1, row_t row2, const column_block_shape_t& node)
            {
                assert(row1 <= row2);
                row_t length = row2 - row1 + 1;

                switch (node.type)
                {
                    case column_block_t::boolean:
                    {
                        auto blk_range = detail::make_element_range<column_block_t::boolean>{}(node, length);
                        auto func = [](bool b) { return b ? 1.0 : 0.0; };
                        std::transform(blk_range.begin(), blk_range.end(), insert_it, func);
                        break;
                    }
                    case column_block_t::numeric:
                    {
                        auto blk_range = detail::make_element_range<column_block_t::numeric>{}(node, length);
                        std::copy(blk_range.begin(), blk_range.end(), insert_it);
                        break;
                    }
                    case column_block_t::formula:
                    {
                        auto blk_range = detail::make_element_range<column_block_t::formula>{}(node, length);

                        for (const formula_cell* fc : blk_range)
                        {
                            formula_result res = fc->get_result_cache(wait_policy);
                            switch (res.get_type())
                            {
                                case formula_result::result_type::boolean:
                                    insert_it = res.get_boolean() ? 1.0 : 0.0;
                                    break;
                                case formula_result::result_type::value:
                                    insert_it = res.get_value();
                                    break;
                                default:;
                            }
                        }

                        break;
                    }
                    default:;
                }
                return true;
            };

            for (sheet_t sheet = range.first.sheet; sheet <= range.last.sheet; ++sheet)
                cxt.walk(sheet, range, cb);

            break;
        }
        default:
            args.pop_back();
    }
}

} // anonymous namespace

// ============================================================================

formula_functions::invalid_arg::invalid_arg(const std::string& msg) :
    general_error(msg) {}

formula_function_t formula_functions::get_function_opcode(const formula_token& token)
{
    assert(token.opcode == fop_function);
    return std::get<formula_function_t>(token.value);
}

formula_function_t formula_functions::get_function_opcode(std::string_view s)
{
    std::string upper;

    for (char c : s)
    {
        if (c > 'Z')
        {
            // Convert to upper case.
            c -= 'a' - 'A';
        }

        upper.push_back(c);
    }

    return builtin_funcs::get().find(upper.data(), upper.size());
}

std::string_view formula_functions::get_function_name(formula_function_t oc)
{
    return builtin_funcs::get().find_key(oc);
}

formula_functions::formula_functions(model_context& cxt, const abs_address_t& pos) :
    m_context(cxt), m_pos(pos)
{
}

formula_functions::~formula_functions()
{
}

void formula_functions::interpret(formula_function_t oc, formula_value_stack& args)
{
    try
    {
        switch (oc)
        {
            case formula_function_t::func_abs:
                fnc_abs(args);
                break;
            case formula_function_t::func_and:
                fnc_and(args);
                break;
            case formula_function_t::func_average:
                fnc_average(args);
                break;
            case formula_function_t::func_column:
                fnc_column(args);
                break;
            case formula_function_t::func_columns:
                fnc_columns(args);
                break;
            case formula_function_t::func_concatenate:
                fnc_concatenate(args);
                break;
            case formula_function_t::func_count:
                fnc_count(args);
                break;
            case formula_function_t::func_counta:
                fnc_counta(args);
                break;
            case formula_function_t::func_countblank:
                fnc_countblank(args);
                break;
            case formula_function_t::func_exact:
                fnc_exact(args);
                break;
            case formula_function_t::func_false:
                fnc_false(args);
                break;
            case formula_function_t::func_find:
                fnc_find(args);
                break;
            case formula_function_t::func_if:
                fnc_if(args);
                break;
            case formula_function_t::func_isblank:
                fnc_isblank(args);
                break;
            case formula_function_t::func_iserr:
                fnc_iserr(args);
                break;
            case formula_function_t::func_iserror:
                fnc_iserror(args);
                break;
            case formula_function_t::func_iseven:
                fnc_iseven(args);
                break;
            case formula_function_t::func_isformula:
                fnc_isformula(args);
                break;
            case formula_function_t::func_islogical:
                fnc_islogical(args);
                break;
            case formula_function_t::func_isna:
                fnc_isna(args);
                break;
            case formula_function_t::func_isnontext:
                fnc_isnontext(args);
                break;
            case formula_function_t::func_isnumber:
                fnc_isnumber(args);
                break;
            case formula_function_t::func_isodd:
                fnc_isodd(args);
                break;
            case formula_function_t::func_isref:
                fnc_isref(args);
                break;
            case formula_function_t::func_istext:
                fnc_istext(args);
                break;
            case formula_function_t::func_int:
                fnc_int(args);
                break;
            case formula_function_t::func_left:
                fnc_left(args);
                break;
            case formula_function_t::func_len:
                fnc_len(args);
                break;
            case formula_function_t::func_max:
                fnc_max(args);
                break;
            case formula_function_t::func_median:
                fnc_median(args);
                break;
            case formula_function_t::func_mid:
                fnc_mid(args);
                break;
            case formula_function_t::func_min:
                fnc_min(args);
                break;
            case formula_function_t::func_mmult:
                fnc_mmult(args);
                break;
            case formula_function_t::func_mode:
                fnc_mode(args);
                break;
            case formula_function_t::func_n:
                fnc_n(args);
                break;
            case formula_function_t::func_na:
                fnc_na(args);
                break;
            case formula_function_t::func_not:
                fnc_not(args);
                break;
            case formula_function_t::func_now:
                fnc_now(args);
                break;
            case formula_function_t::func_or:
                fnc_or(args);
                break;
            case formula_function_t::func_pi:
                fnc_pi(args);
                break;
            case formula_function_t::func_replace:
                fnc_replace(args);
                break;
            case formula_function_t::func_rept:
                fnc_rept(args);
                break;
            case formula_function_t::func_right:
                fnc_right(args);
                break;
            case formula_function_t::func_row:
                fnc_row(args);
                break;
            case formula_function_t::func_rows:
                fnc_rows(args);
                break;
            case formula_function_t::func_sheet:
                fnc_sheet(args);
                break;
            case formula_function_t::func_sheets:
                fnc_sheets(args);
                break;
            case formula_function_t::func_substitute:
                fnc_substitute(args);
                break;
            case formula_function_t::func_subtotal:
                fnc_subtotal(args);
                break;
            case formula_function_t::func_sum:
                fnc_sum(args);
                break;
            case formula_function_t::func_t:
                fnc_t(args);
                break;
            case formula_function_t::func_textjoin:
                fnc_textjoin(args);
                break;
            case formula_function_t::func_trim:
                fnc_trim(args);
                break;
            case formula_function_t::func_true:
                fnc_true(args);
                break;
            case formula_function_t::func_type:
                fnc_type(args);
                break;
            case formula_function_t::func_wait:
                fnc_wait(args);
                break;
            case formula_function_t::func_unknown:
            default:
            {
                std::ostringstream os;
                os << "formula function not implemented yet (name="
                    << get_formula_function_name(oc)
                    << ")";
                throw not_implemented_error(os.str());
            }
        }
    }
    catch (const formula_error& e)
    {
        using t = std::underlying_type<formula_error_t>::type;
        formula_error_t err = e.get_error();
        if (static_cast<t>(err) >= 200u)
            // re-throw if it's an internal error.
            throw;

        args.clear();
        args.push_error(err);
    }
}

void formula_functions::fnc_max(formula_value_stack& args) const
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

void formula_functions::fnc_median(formula_value_stack& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("MEDIAN requires one or more arguments.");

    std::vector<double> seq;

    while (!args.empty())
        append_values_from_stack(m_context, args, std::back_inserter(seq));

    std::size_t mid_pos = seq.size() / 2;

    if (seq.size() & 0x01)
    {
        // odd number of values
        auto it_mid = seq.begin() + mid_pos;
        std::nth_element(seq.begin(), it_mid, seq.end());
        args.push_value(seq[mid_pos]);
    }
    else
    {
        // even number of values.  Take the average of the two mid values.
        std::sort(seq.begin(), seq.end());
        double v = seq[mid_pos - 1] + seq[mid_pos];
        args.push_value(v / 2.0);
    }
}

void formula_functions::fnc_min(formula_value_stack& args) const
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

void formula_functions::fnc_mode(formula_value_stack& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("MODE requires one or more arguments.");

    std::vector<double> seq;

    while (!args.empty())
        append_values_from_stack(m_context, args, std::back_inserter(seq));

    if (seq.empty())
    {
        args.push_error(formula_error_t::no_value_available);
        return;
    }

    std::sort(seq.begin(), seq.end());

    // keep counting the number of adjacent equal values in the sorted sequence.

    using value_count_type = std::tuple<double, std::size_t>;
    std::vector<value_count_type> value_counts;

    for (auto it = seq.begin(); it != seq.end(); )
    {
        double cur_v = *it;
        auto it_tail = std::find_if(it, seq.end(), [cur_v](double v) { return cur_v < v; });
        std::size_t len = std::distance(it, it_tail);
        value_counts.emplace_back(cur_v, len);
        it = it_tail;
    }

    assert(!value_counts.empty());

    // Sort the results by the frequency in descending order first, then the
    // value in ascending order.
    auto func_comp = [](value_count_type lhs, value_count_type rhs)
    {
        if (std::get<1>(lhs) > std::get<1>(rhs))
            return true;

        return std::get<0>(lhs) < std::get<0>(rhs);
    };

    std::sort(value_counts.begin(), value_counts.end(), func_comp);
    auto [top_value, top_count] = value_counts[0];

    if (top_count == 1)
    {
        args.push_error(formula_error_t::no_value_available);
        return;
    }

    args.push_value(top_value);
}

void formula_functions::fnc_sum(formula_value_stack& args) const
{
    IXION_TRACE("function: sum");

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

    IXION_TRACE("function: sum end (result=" << ret << ")");
}

void formula_functions::fnc_count(formula_value_stack& args) const
{
    double ret = 0;

    while (!args.empty())
    {
        switch (args.get_type())
        {
            case stack_value_t::value:
                args.pop_back();
                ++ret;
                break;
            case stack_value_t::range_ref:
            {
                abs_range_t range = args.pop_range_ref();
                ret += m_context.count_range(range, value_numeric | value_boolean);
                break;
            }
            case stack_value_t::single_ref:
            {
                abs_address_t pos = args.pop_single_ref();
                abs_range_t range;
                range.first = range.last = pos;
                ret += m_context.count_range(range, value_numeric | value_boolean);
                break;
            }
            default:
                args.pop_back();
        }
    }

    args.push_value(ret);
}

void formula_functions::fnc_counta(formula_value_stack& args) const
{
    double ret = 0;

    while (!args.empty())
    {
        switch (args.get_type())
        {
            case stack_value_t::string:
            case stack_value_t::value:
                args.pop_back();
                ++ret;
                break;
            case stack_value_t::range_ref:
            {
                abs_range_t range = args.pop_range_ref();
                ret += m_context.count_range(range, value_numeric | value_boolean | value_string);
                break;
            }
            case stack_value_t::single_ref:
            {
                abs_address_t pos = args.pop_single_ref();
                abs_range_t range;
                range.first = range.last = pos;
                ret += m_context.count_range(range, value_numeric | value_boolean | value_string);
                break;
            }
            default:
                args.pop_back();
        }

    }

    args.push_value(ret);
}

void formula_functions::fnc_countblank(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("COUNTBLANK requires exactly 1 argument.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        case stack_value_t::range_ref:
        {
            abs_range_t range = args.pop_range_ref();
            double ret = m_context.count_range(range, value_empty);
            args.push_value(ret);
            break;
        }
        default:
            throw formula_functions::invalid_arg("COUNTBLANK only takes a reference argument.");
    }
}

void formula_functions::fnc_abs(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ABS requires exactly 1 argument.");

    double v = args.pop_value();
    args.push_value(std::abs(v));
}

void formula_functions::fnc_average(formula_value_stack& args) const
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
                break;
            }
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

void formula_functions::fnc_mmult(formula_value_stack& args) const
{
    matrix mx[2];
    matrix* mxp = mx;
    const matrix* mxp_end = mxp + 2;

    bool is_arg_invalid = false;

    // NB : the stack is LIFO i.e. the first matrix is the right matrix and
    // the second one is the left one.

    while (!args.empty())
    {
        if (mxp == mxp_end)
        {
            is_arg_invalid = true;
            break;
        }

        auto m = args.maybe_pop_matrix();
        if (!m)
        {
            is_arg_invalid = true;
            break;
        }

        mxp->swap(*m);
        ++mxp;
    }

    if (mxp != mxp_end)
        is_arg_invalid = true;

    if (is_arg_invalid)
        throw formula_functions::invalid_arg("MMULT requires exactly two ranges.");

    mx[0].swap(mx[1]); // Make it so that 0 -> left and 1 -> right.

    if (!mx[0].is_numeric() || !mx[1].is_numeric())
        throw formula_functions::invalid_arg(
            "MMULT requires two numeric ranges. At least one range is not numeric.");

    numeric_matrix ans = multiply_matrices(mx[0], mx[1]);

    args.push_matrix(ans);
}

void formula_functions::fnc_pi(formula_value_stack& args) const
{
    if (!args.empty())
        throw formula_functions::invalid_arg("PI takes no arguments.");

    args.push_value(M_PI);
}

void formula_functions::fnc_int(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("INT requires exactly 1 argument.");

    double v = args.pop_value();
    args.push_value(std::floor(v));
}

void formula_functions::fnc_and(formula_value_stack& args) const
{
    const formula_result_wait_policy_t wait_policy = m_context.get_formula_result_wait_policy();
    bool final_result = true;

    while (!args.empty() && final_result)
    {
        switch (args.get_type())
        {
            case stack_value_t::single_ref:
            case stack_value_t::value:
            case stack_value_t::string:
            {
                std::optional<bool> v = pop_one_value_as_boolean(m_context, args);
                if (v)
                    final_result = *v;
                break;
            }
            case stack_value_t::range_ref:
            {
                auto range = args.pop_range_ref();
                sheet_t sheet = range.first.sheet;
                abs_rc_range_t rc_range = range;

                column_block_callback_t cb = [&final_result, wait_policy](
                    col_t col, row_t row1, row_t row2, const column_block_shape_t& node)
                {
                    assert(row1 <= row2);
                    row_t length = row2 - row1 + 1;

                    switch (node.type)
                    {
                        case column_block_t::empty:
                        case column_block_t::string:
                        case column_block_t::unknown:
                            // non-numeric blocks get skipped.
                            break;
                        case column_block_t::boolean:
                        {
                            auto blk_range = detail::make_element_range<column_block_t::boolean>{}(node, length);
                            bool res = std::all_of(blk_range.begin(), blk_range.end(), [](bool v) { return v; });
                            final_result = res;
                            break;
                        }
                        case column_block_t::numeric:
                        {
                            auto blk_range = detail::make_element_range<column_block_t::numeric>{}(node, length);
                            bool res = std::all_of(blk_range.begin(), blk_range.end(), [](double v) { return v != 0.0; });
                            final_result = res;
                            break;
                        }
                        case column_block_t::formula:
                        {
                            auto blk_range = detail::make_element_range<column_block_t::formula>{}(node, length);

                            for (const formula_cell* fc : blk_range)
                            {
                                formula_result res = fc->get_result_cache(wait_policy);
                                switch (res.get_type())
                                {
                                    case formula_result::result_type::boolean:
                                        final_result = res.get_boolean();
                                        break;
                                    case formula_result::result_type::value:
                                        final_result = res.get_value() != 0.0;
                                        break;
                                    default:;
                                }
                            }

                            break;
                        }
                    }

                    return final_result; // returning false will end the walk.
                };

                m_context.walk(sheet, rc_range, cb);
                break;
            }
            default:
                throw formula_error(formula_error_t::general_error);
        }
    }

    args.clear();
    args.push_boolean(final_result);
}

void formula_functions::fnc_or(formula_value_stack& args) const
{
    const formula_result_wait_policy_t wait_policy = m_context.get_formula_result_wait_policy();
    bool final_result = false;

    while (!args.empty())
    {
        bool this_result = false;

        switch (args.get_type())
        {
            case stack_value_t::single_ref:
            case stack_value_t::value:
            case stack_value_t::string:
            {
                std::optional<bool> v = pop_one_value_as_boolean(m_context, args);
                if (v)
                    this_result = *v;
                break;
            }
            case stack_value_t::range_ref:
            {
                auto range = args.pop_range_ref();
                sheet_t sheet = range.first.sheet;
                abs_rc_range_t rc_range = range;

                // We will bail out of the walk on the first positive result.

                column_block_callback_t cb = [&this_result, wait_policy](
                    col_t col, row_t row1, row_t row2, const column_block_shape_t& node)
                {
                    assert(row1 <= row2);
                    row_t length = row2 - row1 + 1;

                    switch (node.type)
                    {
                        case column_block_t::empty:
                        case column_block_t::string:
                        case column_block_t::unknown:
                            // non-numeric blocks get skipped.
                            break;
                        case column_block_t::boolean:
                        {
                            auto blk_range = detail::make_element_range<column_block_t::boolean>{}(node, length);
                            this_result = std::any_of(blk_range.begin(), blk_range.end(), [](bool v) { return v; });
                            break;
                        }
                        case column_block_t::numeric:
                        {
                            auto blk_range = detail::make_element_range<column_block_t::numeric>{}(node, length);
                            this_result = std::any_of(blk_range.begin(), blk_range.end(), [](double v) { return v != 0.0; });
                            break;
                        }
                        case column_block_t::formula:
                        {
                            auto blk_range = detail::make_element_range<column_block_t::formula>{}(node, length);

                            for (const formula_cell* fc : blk_range)
                            {
                                formula_result res = fc->get_result_cache(wait_policy);
                                switch (res.get_type())
                                {
                                    case formula_result::result_type::boolean:
                                        this_result = res.get_boolean();
                                        break;
                                    case formula_result::result_type::value:
                                        this_result = res.get_value() != 0.0;
                                        break;
                                    default:;
                                }

                                if (this_result)
                                    break;
                            }

                            break;
                        }
                    }

                    return !this_result; // returning false will end the walk.
                };

                m_context.walk(sheet, rc_range, cb);
                break;
            }
            default:
                throw formula_error(formula_error_t::general_error);
        }

        if (this_result)
        {
            final_result = true;
            break;
        }
    }

    args.clear();
    args.push_boolean(final_result);
}

void formula_functions::fnc_if(formula_value_stack& args) const
{
    if (args.size() != 3)
        throw formula_functions::invalid_arg("IF requires exactly 3 arguments.");

    formula_value_stack::iterator pos = args.begin();
    bool eval = args.get_value(0) != 0.0;
    if (eval)
        std::advance(pos, 1);
    else
        std::advance(pos, 2);

    formula_value_stack ret(m_context);
    ret.push_back(args.release(pos));
    args.swap(ret);
}

void formula_functions::fnc_true(formula_value_stack& args) const
{
    if (!args.empty())
        throw formula_functions::invalid_arg("TRUE takes no arguments.");

    args.push_boolean(true);
}

void formula_functions::fnc_false(formula_value_stack& args) const
{
    if (!args.empty())
        throw formula_functions::invalid_arg("FALSE takes no arguments.");

    args.push_boolean(false);
}

void formula_functions::fnc_not(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("NOT requires exactly one argument.");

    args.push_boolean(!args.pop_boolean());
}

void formula_functions::fnc_isblank(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ISBLANK requires exactly one argument.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            bool res = m_context.get_celltype(addr) == celltype_t::empty;
            args.push_boolean(res);
            break;
        }
        case stack_value_t::range_ref:
        {
            abs_range_t range = args.pop_range_ref();
            bool res = m_context.is_empty(range);
            args.push_boolean(res);
            break;
        }
        default:
        {
            args.clear();
            args.push_boolean(false);
        }
    }
}

void formula_functions::fnc_iserr(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ISERR requires exactly one argument.");

    using _int_type = std::underlying_type_t<formula_error_t>;

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            auto ca = m_context.get_cell_access(addr);
            if (ca.get_value_type() != cell_value_t::error)
            {
                args.push_boolean(false);
                break;
            }

            auto v = static_cast<_int_type>(ca.get_error_value());
            args.push_boolean(1u <= v && v <= 6u);
            break;
        }
        case stack_value_t::error:
        {
            auto v = static_cast<_int_type>(args.pop_error());
            args.push_boolean(1u <= v && v <= 6u);
            break;
        }
        default:
        {
            args.clear();
            args.push_boolean(false);
        }
    }
}

void formula_functions::fnc_iserror(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ISERROR requires exactly one argument.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            bool res = m_context.get_cell_value_type(addr) == cell_value_t::error;
            args.push_boolean(res);
            break;
        }
        case stack_value_t::error:
        {
            args.clear();
            args.push_boolean(true);
            break;
        }
        default:
        {
            args.clear();
            args.push_boolean(false);
        }
    }
}

void formula_functions::fnc_iseven(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ISEVEN requires exactly one argument.");

    bool odd = pop_and_check_for_odd_value(args);
    args.push_boolean(!odd);
}

void formula_functions::fnc_isformula(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ISFORMULA requires exactly one argument.");

    if (args.get_type() != stack_value_t::single_ref)
    {
        args.clear();
        args.push_boolean(false);
        return;
    }

    abs_address_t addr = args.pop_single_ref();
    bool res = m_context.get_celltype(addr) == celltype_t::formula;
    args.push_boolean(res);
}

void formula_functions::fnc_islogical(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ISLOGICAL requires exactly one argument.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            bool res = m_context.get_cell_value_type(addr) == cell_value_t::boolean;
            args.push_boolean(res);
            break;
        }
        case stack_value_t::boolean:
            args.clear();
            args.push_boolean(true);
            break;
        default:
            args.clear();
            args.push_boolean(false);
    }
}

void formula_functions::fnc_isna(formula_value_stack& args) const
{
    if (args.size() != 1u)
        throw formula_functions::invalid_arg("ISNA requires exactly one argument.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            auto ca = m_context.get_cell_access(addr);
            formula_error_t err = ca.get_error_value();
            args.push_boolean(err == formula_error_t::no_value_available);
            break;
        }
        case stack_value_t::error:
        {
            bool res = args.pop_error() == formula_error_t::no_value_available;
            args.push_boolean(res);
            break;
        }
        default:
        {
            args.clear();
            args.push_boolean(false);
        }
    }
}

void formula_functions::fnc_isnontext(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ISNONTEXT requires exactly one argument.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            bool res = m_context.get_cell_value_type(addr) != cell_value_t::string;
            args.push_boolean(res);
            break;
        }
        case stack_value_t::string:
            args.clear();
            args.push_boolean(false);
            break;
        default:
            args.clear();
            args.push_boolean(true);
    }
}

void formula_functions::fnc_isnumber(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ISNUMBER requires exactly one argument.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            bool res = m_context.get_cell_value_type(addr) == cell_value_t::numeric;
            args.push_boolean(res);
            break;
        }
        case stack_value_t::value:
            args.clear();
            args.push_boolean(true);
            break;
        default:
            args.clear();
            args.push_boolean(false);
    }
}

void formula_functions::fnc_isodd(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ISODD requires exactly one argument.");

    bool odd = pop_and_check_for_odd_value(args);
    args.push_boolean(odd);
}

void formula_functions::fnc_isref(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ISREF requires exactly one argument.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        case stack_value_t::range_ref:
            args.clear();
            args.push_boolean(true);
            break;
        default:
            args.clear();
            args.push_boolean(false);
    }
}

void formula_functions::fnc_istext(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("ISTEXT requires exactly one argument.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            bool res = m_context.get_cell_value_type(addr) == cell_value_t::string;
            args.push_boolean(res);
            break;
        }
        case stack_value_t::string:
            args.clear();
            args.push_boolean(true);
            break;
        default:
            args.clear();
            args.push_boolean(false);
    }
}

void formula_functions::fnc_n(formula_value_stack& args) const
{
    if (args.size() != 1u)
        throw formula_functions::invalid_arg("N takes exactly one argument.");

    double v = args.pop_value();
    args.push_value(v);
}

void formula_functions::fnc_na(formula_value_stack& args) const
{
    if (!args.empty())
        throw formula_functions::invalid_arg("NA takes no arguments.");

    args.push_error(formula_error_t::no_value_available);
}

void formula_functions::fnc_type(formula_value_stack& args) const
{
    if (args.size() != 1u)
        throw formula_functions::invalid_arg("TYPE requires exactly one argument.");

    switch (args.get_type())
    {
        case stack_value_t::boolean:
            args.pop_back();
            args.push_value(4);
            break;
        case stack_value_t::error:
            args.pop_back();
            args.push_value(16);
            break;
        case stack_value_t::matrix:
        case stack_value_t::range_ref:
            args.pop_back();
            args.push_value(64);
            break;
        case stack_value_t::string:
            args.pop_back();
            args.push_value(2);
            break;
        case stack_value_t::value:
            args.pop_back();
            args.push_value(1);
            break;
        case stack_value_t::single_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            cell_access ca = m_context.get_cell_access(addr);

            switch (ca.get_value_type())
            {
                case cell_value_t::boolean:
                    args.push_value(4);
                    break;
                case cell_value_t::empty:
                case cell_value_t::numeric:
                    args.push_value(1);
                    break;
                case cell_value_t::error:
                    args.push_value(16);
                    break;
                case cell_value_t::string:
                    args.push_value(2);
                    break;
                case cell_value_t::unknown:
                    throw formula_error(formula_error_t::no_result_error);
            }

            break;
        }
    }
}

void formula_functions::fnc_len(formula_value_stack& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("LEN requires exactly one argument.");

    std::string s = args.pop_string();

    auto positions = detail::calc_utf8_byte_positions(s);
    args.push_value(positions.size());
}

void formula_functions::fnc_mid(formula_value_stack& args) const
{
    if (args.size() != 3)
        throw formula_functions::invalid_arg("MID requires exactly 3 arguments.");

    int len = std::floor(args.pop_value());
    int start = std::floor(args.pop_value()); // 1-based

    if (len < 0 || start < 1)
    {
        args.clear();
        args.push_error(formula_error_t::invalid_value_type);
        return;
    }

    std::string s = args.pop_string();

    auto positions = detail::calc_utf8_byte_positions(s);

    start -= 1; // convert to 0-based start position

    if (std::size_t(start) >= positions.size())
    {
        args.push_string(std::string{});
        return;
    }

    std::size_t skip_front = positions[start];
    std::size_t skip_back = 0;

    int max_length = positions.size() - start;
    if (len < max_length)
        skip_back = s.size() - positions[start + len];

    auto it_head = s.cbegin() + skip_front;
    auto it_end = s.cend() - skip_back;

    std::string truncated;
    std::copy(it_head, it_end, std::back_inserter(truncated));
    args.push_string(truncated);
}

void formula_functions::fnc_concatenate(formula_value_stack& args) const
{
    std::string s;
    while (!args.empty())
        s = args.pop_string() + s;
    args.push_string(std::move(s));
}

void formula_functions::fnc_exact(formula_value_stack& args) const
{
    if (args.size() != 2u)
        throw formula_functions::invalid_arg("EXACT requires exactly 2 arguments.");

    std::string right = args.pop_string();
    std::string left = args.pop_string();

    return args.push_boolean(right == left);
}

void formula_functions::fnc_find(formula_value_stack& args) const
{
    if (args.size() < 2u || args.size() > 3u)
        throw formula_functions::invalid_arg("FIND requires at least 2 and no more than 3 arguments.");

    int start_pos = 0;
    if (args.size() == 3u)
        start_pos = std::floor(args.pop_value()) - 1; // to 0-based

    if (start_pos < 0)
    {
        args.clear();
        args.push_error(formula_error_t::invalid_value_type);
        return;
    }

    std::string content = args.pop_string();
    std::string part = args.pop_string();

    auto positions = detail::calc_utf8_byte_positions(content);

    // convert the logical utf-8 start position to a corresponding byte start position

    if (std::size_t(start_pos) >= positions.size())
    {
        args.push_error(formula_error_t::invalid_value_type);
        return;
    }

    start_pos = positions[start_pos];
    std::size_t pos = content.find(part, start_pos);

    if (pos == std::string::npos)
    {
        args.push_error(formula_error_t::invalid_value_type);
        return;
    }

    // convert the byte position to a logical utf-8 character position.
    auto it = std::lower_bound(positions.begin(), positions.end(), pos);

    if (it == positions.end() || *it != pos)
    {
        // perhaps invalid utf-8 string...
        args.push_error(formula_error_t::invalid_value_type);
        return;
    }

    pos = std::distance(positions.begin(), it);
    args.push_value(pos + 1); // back to 1-based
}

void formula_functions::fnc_left(formula_value_stack& args) const
{
    if (args.empty() || args.size() > 2)
        throw formula_functions::invalid_arg(
            "LEFT requires at least one argument but no more than 2.");

    int n = 1; // when the 2nd arg is skipped, it defaults to 1.
    if (args.size() == 2)
        n = std::floor(args.pop_value());

    if (n < 0)
    {
        args.clear();
        args.push_error(formula_error_t::invalid_value_type);
        return;
    }

    std::string s = args.pop_string();

    auto positions = detail::calc_utf8_byte_positions(s);

    // Resize ONLY when the desired length is lower than the original string length.
    if (std::size_t(n) < positions.size())
        s.resize(positions[n]);

    args.push_string(std::move(s));
}

void formula_functions::fnc_replace(formula_value_stack& args) const
{
    if (args.size() != 4u)
        throw formula_functions::invalid_arg("REPLACE requires exactly 4 arguments.");

    std::string new_text = args.pop_string();
    int n_segment = std::floor(args.pop_value());
    int pos_segment = std::floor(args.pop_value()) - 1; // to 0-based

    if (n_segment < 0 || pos_segment < 0)
    {
        args.clear();
        args.push_error(formula_error_t::invalid_value_type);
        return;
    }

    std::string content = args.pop_string();

    auto positions = detail::calc_utf8_byte_positions(content);

    pos_segment = std::min<int>(pos_segment, positions.size());
    n_segment = std::min<int>(n_segment, positions.size() - pos_segment);

    // convert to its byte position.
    std::size_t pos_bytes = std::size_t(pos_segment) < positions.size() ? positions[pos_segment] : content.size();

    // copy the leading segment.
    auto it = std::next(content.begin(), pos_bytes);
    std::string content_new{content.begin(), it};

    content_new += new_text;

    // copy the trailing segment.
    std::size_t pos_logical = pos_segment + n_segment;
    pos_bytes = pos_logical < positions.size() ? positions[pos_logical] : content.size();
    it = std::next(content.begin(), pos_bytes);
    std::copy(it, content.end(), std::back_inserter(content_new));

    args.push_string(content_new);
}

void formula_functions::fnc_rept(formula_value_stack& args) const
{
    if (args.size() != 2u)
        throw formula_functions::invalid_arg("REPT requires 2 arguments.");

    int count = args.pop_value();
    if (count < 0)
    {
        args.clear();
        args.push_error(formula_error_t::invalid_value_type);
        return;
    }

    std::string s = args.pop_string();
    std::ostringstream os;
    for (int i = 0; i < count; ++i)
        os << s;

    args.push_string(os.str());
}

void formula_functions::fnc_right(formula_value_stack& args) const
{
    if (args.empty() || args.size() > 2)
        throw formula_functions::invalid_arg(
            "RIGHT requires at least one argument but no more than 2.");

    int n = 1; // when the 2nd arg is skipped, it defaults to 1.
    if (args.size() == 2)
        n = std::floor(args.pop_value());

    if (n < 0)
    {
        args.clear();
        args.push_error(formula_error_t::invalid_value_type);
        return;
    }

    if (n == 0)
    {
        args.clear();
        args.push_string(std::string{});
        return;
    }

    std::string s = args.pop_string();

    auto positions = detail::calc_utf8_byte_positions(s);

    // determine how many logical characters to skip.
    n = int(positions.size()) - n;

    if (n > 0)
    {
        assert(std::size_t(n) < positions.size());
        auto it = std::next(s.begin(), positions[n]);
        std::string s_skipped;
        std::copy(it, s.end(), std::back_inserter(s_skipped));
        s.swap(s_skipped);
    }

    args.push_string(std::move(s));
}

void formula_functions::fnc_substitute(formula_value_stack& args) const
{
    if (args.size() < 3 || args.size() > 4)
        throw formula_functions::invalid_arg(
            "SUBSTITUTE requires at least 3 arguments but no more than 4.");

    constexpr int replace_all = -1;
    int which = replace_all;

    if (args.size() == 4)
    {
        // explicit which value provided.
        which = std::floor(args.pop_value());

        if (which < 1)
        {
            args.clear();
            args.push_error(formula_error_t::invalid_value_type);
            return;
        }
    }

    const std::string text_new = args.pop_string();
    const std::string text_old = args.pop_string();
    const std::string content = args.pop_string();
    std::string content_new;

    std::size_t pos = 0;
    int which_found = 0;

    while (true)
    {
        std::size_t found_pos = content.find(text_old, pos);
        if (found_pos == std::string::npos)
        {
            // Copy the rest of the string to the new buffer and exit.
            content_new.append(content, pos, std::string::npos);
            break;
        }

        ++which_found;
        bool replace_this = which_found == which || which == replace_all;
        content_new.append(content, pos, found_pos - pos);
        content_new.append(replace_this ? text_new : text_old);
        pos = found_pos + text_old.size();
    }

    args.clear();
    args.push_string(std::move(content_new));
}

void formula_functions::fnc_t(formula_value_stack& args) const
{
    if (args.size() != 1u)
        throw formula_functions::invalid_arg("T takes exactly one argument.");

    switch (args.get_type())
    {
        case stack_value_t::string:
            // Do nothing and reuse the string value as the return value.
            break;
        case stack_value_t::single_ref:
        case stack_value_t::range_ref:
        {
            auto addr = args.pop_single_ref();
            auto ca = m_context.get_cell_access(addr);

            std::string s;

            if (ca.get_value_type() == cell_value_t::string)
                s = ca.get_string_value();

            args.push_string(std::move(s));
            break;
        }
        default:
        {
            args.pop_back();
            args.push_string(std::string{});
        }
    }
}

void formula_functions::fnc_textjoin(formula_value_stack& args) const
{
    if (args.size() < 3u)
        throw formula_functions::invalid_arg("TEXTJOIN requires at least 3 arguments.");

    std::deque<abs_range_t> ranges;

    while (args.size() > 2u)
        ranges.push_front(args.pop_range_ref());

    bool skip_empty = args.pop_boolean();
    std::string delim = args.pop_string();
    std::vector<std::string> tokens;

    for (const abs_range_t& range : ranges)
    {
        for (sheet_t sheet = range.first.sheet; sheet <= range.last.sheet; ++sheet)
        {
            model_iterator miter = m_context.get_model_iterator(sheet, rc_direction_t::horizontal, range);

            for (; miter.has(); miter.next())
            {
                auto& cell = miter.get();

                switch (cell.type)
                {
                    case celltype_t::string:
                    {
                        auto sid = std::get<string_id_t>(cell.value);
                        const std::string* s = m_context.get_string(sid);
                        assert(s);
                        tokens.emplace_back(*s);
                        break;
                    }
                    case celltype_t::numeric:
                    {
                        std::ostringstream os;
                        os << std::get<double>(cell.value);
                        tokens.emplace_back(os.str());
                        break;
                    }
                    case celltype_t::boolean:
                    {
                        std::ostringstream os;
                        os << std::boolalpha << std::get<bool>(cell.value);
                        tokens.emplace_back(os.str());
                        break;
                    }
                    case celltype_t::formula:
                    {
                        const auto* fc = std::get<const formula_cell*>(cell.value);
                        formula_result res = fc->get_result_cache(m_context.get_formula_result_wait_policy());
                        tokens.emplace_back(res.str(m_context));
                        break;
                    }
                    case celltype_t::empty:
                    {
                        if (!skip_empty)
                            tokens.emplace_back();
                        break;
                    }
                    case celltype_t::unknown:
                        // logic error - this should never happen!
                        throw formula_error(formula_error_t::no_result_error);
                }
            }
        }
    }

    if (tokens.empty())
    {
        args.push_string(std::string{});
        return;
    }

    std::string result = std::move(tokens.front());

    for (auto it = std::next(tokens.begin()); it != tokens.end(); ++it)
    {
        result.append(delim);
        result.append(*it);
    }

    args.push_string(std::move(result));
}

void formula_functions::fnc_trim(formula_value_stack& args) const
{
    if (args.size() != 1u)
        throw formula_functions::invalid_arg("TRIM takes exactly one argument.");

    std::string s = args.pop_string();
    const char* p = s.data();
    const char* p_end = p + s.size();
    const char* p_head = nullptr;

    std::vector<std::string> tokens;

    for (; p != p_end; ++p)
    {
        if (*p == ' ')
        {
            if (p_head)
            {
                tokens.emplace_back(p_head, std::distance(p_head, p));
                p_head = nullptr;
            }

            continue;
        }

        if (!p_head)
            // keep track of the head of each token.
            p_head = p;
    }

    if (p_head)
        tokens.emplace_back(p_head, std::distance(p_head, p));

    if (tokens.empty())
    {
        args.push_string(std::string{});
        return;
    }

    std::ostringstream os;
    std::copy(tokens.cbegin(), std::prev(tokens.cend()), std::ostream_iterator<std::string>(os, " "));
    os << tokens.back();

    args.push_string(os.str());
}

void formula_functions::fnc_now(formula_value_stack& args) const
{
    if (!args.empty())
        throw formula_functions::invalid_arg("NOW takes no arguments.");

    // TODO: this value is currently not accurate since we don't take into
    // account the zero date yet.
    double cur_time = get_current_time();
    cur_time /= 86400.0; // convert seconds to days.
    args.push_value(cur_time);
}

void formula_functions::fnc_wait(formula_value_stack& args) const
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    args.clear();
    args.push_value(1);
}

void formula_functions::fnc_subtotal(formula_value_stack& args) const
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
            break;
        }
        default:
        {
            std::ostringstream os;
            os << "SUBTOTAL: function type " << subtype << " not implemented yet";
            throw formula_functions::invalid_arg(os.str());
        }
    }
}

void formula_functions::fnc_column(formula_value_stack& args) const
{
    if (args.empty())
    {
        args.push_value(m_pos.column + 1);
        return;
    }

    if (args.size() > 1)
        throw formula_functions::invalid_arg("COLUMN requires 1 argument or less.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        case stack_value_t::range_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            args.push_value(addr.column + 1);
            break;
        }
        default:
            throw formula_error(formula_error_t::invalid_value_type);
    }
}

void formula_functions::fnc_columns(formula_value_stack& args) const
{
    double res = 0.0;

    while (!args.empty())
    {
        switch (args.get_type())
        {
            case stack_value_t::single_ref:
            case stack_value_t::range_ref:
            {
                abs_range_t range = args.pop_range_ref();
                res += range.last.column - range.first.column + 1;
                break;
            }
            default:
                throw formula_error(formula_error_t::invalid_value_type);
        }
    }

    args.push_value(res);
}

void formula_functions::fnc_row(formula_value_stack& args) const
{
    if (args.empty())
    {
        args.push_value(m_pos.row + 1);
        return;
    }

    if (args.size() > 1)
        throw formula_functions::invalid_arg("ROW requires 1 argument or less.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        case stack_value_t::range_ref:
        {
            abs_address_t addr = args.pop_single_ref();
            args.push_value(addr.row + 1);
            break;
        }
        default:
            throw formula_error(formula_error_t::invalid_value_type);
    }
}

void formula_functions::fnc_rows(formula_value_stack& args) const
{
    double res = 0.0;

    while (!args.empty())
    {
        switch (args.get_type())
        {
            case stack_value_t::single_ref:
            case stack_value_t::range_ref:
            {
                abs_range_t range = args.pop_range_ref();
                res += range.last.row - range.first.row + 1;
                break;
            }
            default:
                throw formula_error(formula_error_t::invalid_value_type);
        }
    }

    args.push_value(res);
}

void formula_functions::fnc_sheet(formula_value_stack& args) const
{
    if (args.empty())
    {
        // Take the current sheet index.
        args.push_value(m_pos.sheet + 1);
        return;
    }

    if (args.size() > 1u)
        throw formula_functions::invalid_arg("SHEET only takes one argument or less.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        case stack_value_t::range_ref:
        {
            abs_range_t range = args.pop_range_ref();
            args.push_value(range.first.sheet + 1);
            break;
        }
        case stack_value_t::string:
        {
            // TODO: we need to make this case insensitive.
            std::string sheet_name = args.pop_string();
            sheet_t sheet_id = m_context.get_sheet_index(sheet_name);
            if (sheet_id == invalid_sheet)
                throw formula_error(formula_error_t::no_value_available);

            args.push_value(sheet_id + 1);
            break;
        }
        default:
            throw formula_error(formula_error_t::invalid_value_type);
    }
}

void formula_functions::fnc_sheets(formula_value_stack& args) const
{
    if (args.empty())
    {
        args.push_value(m_context.get_sheet_count());
        return;
    }

    if (args.size() != 1u)
        throw formula_functions::invalid_arg("SHEETS only takes one argument or less.");

    switch (args.get_type())
    {
        case stack_value_t::single_ref:
        case stack_value_t::range_ref:
        {
            abs_range_t range = args.pop_range_ref();
            sheet_t n = range.last.sheet - range.first.sheet + 1;
            args.push_value(n);
            break;
        }
        default:
            throw formula_error(formula_error_t::no_value_available);
    }
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
