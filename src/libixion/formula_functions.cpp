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
#include <ixion/macros.hpp>
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

typedef mdds::sorted_string_map<formula_function_t> map_type;

// Keys must be sorted.
constexpr map_type::entry entries[] =
{
    { IXION_ASCII("ABS"), formula_function_t::func_abs },
    { IXION_ASCII("ACOS"), formula_function_t::func_acos },
    { IXION_ASCII("ACOSH"), formula_function_t::func_acosh },
    { IXION_ASCII("ACOT"), formula_function_t::func_acot },
    { IXION_ASCII("ACOTH"), formula_function_t::func_acoth },
    { IXION_ASCII("ADDRESS"), formula_function_t::func_address },
    { IXION_ASCII("AGGREGATE"), formula_function_t::func_aggregate },
    { IXION_ASCII("AND"), formula_function_t::func_and },
    { IXION_ASCII("ARABIC"), formula_function_t::func_arabic },
    { IXION_ASCII("AREAS"), formula_function_t::func_areas },
    { IXION_ASCII("ASC"), formula_function_t::func_asc },
    { IXION_ASCII("ASIN"), formula_function_t::func_asin },
    { IXION_ASCII("ASINH"), formula_function_t::func_asinh },
    { IXION_ASCII("ATAN"), formula_function_t::func_atan },
    { IXION_ASCII("ATAN2"), formula_function_t::func_atan2 },
    { IXION_ASCII("ATANH"), formula_function_t::func_atanh },
    { IXION_ASCII("AVEDEV"), formula_function_t::func_avedev },
    { IXION_ASCII("AVERAGE"), formula_function_t::func_average },
    { IXION_ASCII("AVERAGEA"), formula_function_t::func_averagea },
    { IXION_ASCII("AVERAGEIF"), formula_function_t::func_averageif },
    { IXION_ASCII("AVERAGEIFS"), formula_function_t::func_averageifs },
    { IXION_ASCII("B"), formula_function_t::func_b },
    { IXION_ASCII("BAHTTEXT"), formula_function_t::func_bahttext },
    { IXION_ASCII("BASE"), formula_function_t::func_base },
    { IXION_ASCII("BETADIST"), formula_function_t::func_betadist },
    { IXION_ASCII("BETAINV"), formula_function_t::func_betainv },
    { IXION_ASCII("BINOMDIST"), formula_function_t::func_binomdist },
    { IXION_ASCII("BITAND"), formula_function_t::func_bitand },
    { IXION_ASCII("BITLSHIFT"), formula_function_t::func_bitlshift },
    { IXION_ASCII("BITOR"), formula_function_t::func_bitor },
    { IXION_ASCII("BITRSHIFT"), formula_function_t::func_bitrshift },
    { IXION_ASCII("BITXOR"), formula_function_t::func_bitxor },
    { IXION_ASCII("CEILING"), formula_function_t::func_ceiling },
    { IXION_ASCII("CELL"), formula_function_t::func_cell },
    { IXION_ASCII("CHAR"), formula_function_t::func_char },
    { IXION_ASCII("CHIDIST"), formula_function_t::func_chidist },
    { IXION_ASCII("CHIINV"), formula_function_t::func_chiinv },
    { IXION_ASCII("CHISQDIST"), formula_function_t::func_chisqdist },
    { IXION_ASCII("CHISQINV"), formula_function_t::func_chisqinv },
    { IXION_ASCII("CHITEST"), formula_function_t::func_chitest },
    { IXION_ASCII("CHOOSE"), formula_function_t::func_choose },
    { IXION_ASCII("CLEAN"), formula_function_t::func_clean },
    { IXION_ASCII("CODE"), formula_function_t::func_code },
    { IXION_ASCII("COLOR"), formula_function_t::func_color },
    { IXION_ASCII("COLUMN"), formula_function_t::func_column },
    { IXION_ASCII("COLUMNS"), formula_function_t::func_columns },
    { IXION_ASCII("COMBIN"), formula_function_t::func_combin },
    { IXION_ASCII("COMBINA"), formula_function_t::func_combina },
    { IXION_ASCII("CONCAT"), formula_function_t::func_concat },
    { IXION_ASCII("CONCATENATE"), formula_function_t::func_concatenate },
    { IXION_ASCII("CONFIDENCE"), formula_function_t::func_confidence },
    { IXION_ASCII("CORREL"), formula_function_t::func_correl },
    { IXION_ASCII("COS"), formula_function_t::func_cos },
    { IXION_ASCII("COSH"), formula_function_t::func_cosh },
    { IXION_ASCII("COT"), formula_function_t::func_cot },
    { IXION_ASCII("COTH"), formula_function_t::func_coth },
    { IXION_ASCII("COUNT"), formula_function_t::func_count },
    { IXION_ASCII("COUNTA"), formula_function_t::func_counta },
    { IXION_ASCII("COUNTBLANK"), formula_function_t::func_countblank },
    { IXION_ASCII("COUNTIF"), formula_function_t::func_countif },
    { IXION_ASCII("COUNTIFS"), formula_function_t::func_countifs },
    { IXION_ASCII("COVAR"), formula_function_t::func_covar },
    { IXION_ASCII("CRITBINOM"), formula_function_t::func_critbinom },
    { IXION_ASCII("CSC"), formula_function_t::func_csc },
    { IXION_ASCII("CSCH"), formula_function_t::func_csch },
    { IXION_ASCII("CUMIPMT"), formula_function_t::func_cumipmt },
    { IXION_ASCII("CUMPRINC"), formula_function_t::func_cumprinc },
    { IXION_ASCII("CURRENT"), formula_function_t::func_current },
    { IXION_ASCII("DATE"), formula_function_t::func_date },
    { IXION_ASCII("DATEDIF"), formula_function_t::func_datedif },
    { IXION_ASCII("DATEVALUE"), formula_function_t::func_datevalue },
    { IXION_ASCII("DAVERAGE"), formula_function_t::func_daverage },
    { IXION_ASCII("DAY"), formula_function_t::func_day },
    { IXION_ASCII("DAYS"), formula_function_t::func_days },
    { IXION_ASCII("DAYS360"), formula_function_t::func_days360 },
    { IXION_ASCII("DB"), formula_function_t::func_db },
    { IXION_ASCII("DCOUNT"), formula_function_t::func_dcount },
    { IXION_ASCII("DCOUNTA"), formula_function_t::func_dcounta },
    { IXION_ASCII("DDB"), formula_function_t::func_ddb },
    { IXION_ASCII("DDE"), formula_function_t::func_dde },
    { IXION_ASCII("DECIMAL"), formula_function_t::func_decimal },
    { IXION_ASCII("DEGREES"), formula_function_t::func_degrees },
    { IXION_ASCII("DEVSQ"), formula_function_t::func_devsq },
    { IXION_ASCII("DGET"), formula_function_t::func_dget },
    { IXION_ASCII("DMAX"), formula_function_t::func_dmax },
    { IXION_ASCII("DMIN"), formula_function_t::func_dmin },
    { IXION_ASCII("DOLLAR"), formula_function_t::func_dollar },
    { IXION_ASCII("DPRODUCT"), formula_function_t::func_dproduct },
    { IXION_ASCII("DSTDEV"), formula_function_t::func_dstdev },
    { IXION_ASCII("DSTDEVP"), formula_function_t::func_dstdevp },
    { IXION_ASCII("DSUM"), formula_function_t::func_dsum },
    { IXION_ASCII("DVAR"), formula_function_t::func_dvar },
    { IXION_ASCII("DVARP"), formula_function_t::func_dvarp },
    { IXION_ASCII("EASTERSUNDAY"), formula_function_t::func_eastersunday },
    { IXION_ASCII("EFFECT"), formula_function_t::func_effect },
    { IXION_ASCII("ENCODEURL"), formula_function_t::func_encodeurl },
    { IXION_ASCII("ERRORTYPE"), formula_function_t::func_errortype },
    { IXION_ASCII("EUROCONVERT"), formula_function_t::func_euroconvert },
    { IXION_ASCII("EVEN"), formula_function_t::func_even },
    { IXION_ASCII("EXACT"), formula_function_t::func_exact },
    { IXION_ASCII("EXP"), formula_function_t::func_exp },
    { IXION_ASCII("EXPONDIST"), formula_function_t::func_expondist },
    { IXION_ASCII("FACT"), formula_function_t::func_fact },
    { IXION_ASCII("FALSE"), formula_function_t::func_false },
    { IXION_ASCII("FDIST"), formula_function_t::func_fdist },
    { IXION_ASCII("FILTERXML"), formula_function_t::func_filterxml },
    { IXION_ASCII("FIND"), formula_function_t::func_find },
    { IXION_ASCII("FINDB"), formula_function_t::func_findb },
    { IXION_ASCII("FINV"), formula_function_t::func_finv },
    { IXION_ASCII("FISHER"), formula_function_t::func_fisher },
    { IXION_ASCII("FISHERINV"), formula_function_t::func_fisherinv },
    { IXION_ASCII("FIXED"), formula_function_t::func_fixed },
    { IXION_ASCII("FLOOR"), formula_function_t::func_floor },
    { IXION_ASCII("FORECAST"), formula_function_t::func_forecast },
    { IXION_ASCII("FORMULA"), formula_function_t::func_formula },
    { IXION_ASCII("FOURIER"), formula_function_t::func_fourier },
    { IXION_ASCII("FREQUENCY"), formula_function_t::func_frequency },
    { IXION_ASCII("FTEST"), formula_function_t::func_ftest },
    { IXION_ASCII("FV"), formula_function_t::func_fv },
    { IXION_ASCII("GAMMA"), formula_function_t::func_gamma },
    { IXION_ASCII("GAMMADIST"), formula_function_t::func_gammadist },
    { IXION_ASCII("GAMMAINV"), formula_function_t::func_gammainv },
    { IXION_ASCII("GAMMALN"), formula_function_t::func_gammaln },
    { IXION_ASCII("GAUSS"), formula_function_t::func_gauss },
    { IXION_ASCII("GCD"), formula_function_t::func_gcd },
    { IXION_ASCII("GEOMEAN"), formula_function_t::func_geomean },
    { IXION_ASCII("GETPIVOTDATA"), formula_function_t::func_getpivotdata },
    { IXION_ASCII("GOALSEEK"), formula_function_t::func_goalseek },
    { IXION_ASCII("GROWTH"), formula_function_t::func_growth },
    { IXION_ASCII("HARMEAN"), formula_function_t::func_harmean },
    { IXION_ASCII("HLOOKUP"), formula_function_t::func_hlookup },
    { IXION_ASCII("HOUR"), formula_function_t::func_hour },
    { IXION_ASCII("HYPERLINK"), formula_function_t::func_hyperlink },
    { IXION_ASCII("HYPGEOMDIST"), formula_function_t::func_hypgeomdist },
    { IXION_ASCII("IF"), formula_function_t::func_if },
    { IXION_ASCII("IFERROR"), formula_function_t::func_iferror },
    { IXION_ASCII("IFNA"), formula_function_t::func_ifna },
    { IXION_ASCII("IFS"), formula_function_t::func_ifs },
    { IXION_ASCII("INDEX"), formula_function_t::func_index },
    { IXION_ASCII("INDIRECT"), formula_function_t::func_indirect },
    { IXION_ASCII("INFO"), formula_function_t::func_info },
    { IXION_ASCII("INT"), formula_function_t::func_int },
    { IXION_ASCII("INTERCEPT"), formula_function_t::func_intercept },
    { IXION_ASCII("IPMT"), formula_function_t::func_ipmt },
    { IXION_ASCII("IRR"), formula_function_t::func_irr },
    { IXION_ASCII("ISBLANK"), formula_function_t::func_isblank },
    { IXION_ASCII("ISERR"), formula_function_t::func_iserr },
    { IXION_ASCII("ISERROR"), formula_function_t::func_iserror },
    { IXION_ASCII("ISEVEN"), formula_function_t::func_iseven },
    { IXION_ASCII("ISFORMULA"), formula_function_t::func_isformula },
    { IXION_ASCII("ISLOGICAL"), formula_function_t::func_islogical },
    { IXION_ASCII("ISNA"), formula_function_t::func_isna },
    { IXION_ASCII("ISNONTEXT"), formula_function_t::func_isnontext },
    { IXION_ASCII("ISNUMBER"), formula_function_t::func_isnumber },
    { IXION_ASCII("ISODD"), formula_function_t::func_isodd },
    { IXION_ASCII("ISOWEEKNUM"), formula_function_t::func_isoweeknum },
    { IXION_ASCII("ISPMT"), formula_function_t::func_ispmt },
    { IXION_ASCII("ISREF"), formula_function_t::func_isref },
    { IXION_ASCII("ISTEXT"), formula_function_t::func_istext },
    { IXION_ASCII("JIS"), formula_function_t::func_jis },
    { IXION_ASCII("KURT"), formula_function_t::func_kurt },
    { IXION_ASCII("LARGE"), formula_function_t::func_large },
    { IXION_ASCII("LCM"), formula_function_t::func_lcm },
    { IXION_ASCII("LEFT"), formula_function_t::func_left },
    { IXION_ASCII("LEFTB"), formula_function_t::func_leftb },
    { IXION_ASCII("LEN"), formula_function_t::func_len },
    { IXION_ASCII("LENB"), formula_function_t::func_lenb },
    { IXION_ASCII("LINEST"), formula_function_t::func_linest },
    { IXION_ASCII("LN"), formula_function_t::func_ln },
    { IXION_ASCII("LOG"), formula_function_t::func_log },
    { IXION_ASCII("LOG10"), formula_function_t::func_log10 },
    { IXION_ASCII("LOGEST"), formula_function_t::func_logest },
    { IXION_ASCII("LOGINV"), formula_function_t::func_loginv },
    { IXION_ASCII("LOGNORMDIST"), formula_function_t::func_lognormdist },
    { IXION_ASCII("LOOKUP"), formula_function_t::func_lookup },
    { IXION_ASCII("LOWER"), formula_function_t::func_lower },
    { IXION_ASCII("MATCH"), formula_function_t::func_match },
    { IXION_ASCII("MAX"), formula_function_t::func_max },
    { IXION_ASCII("MAXA"), formula_function_t::func_maxa },
    { IXION_ASCII("MAXIFS"), formula_function_t::func_maxifs },
    { IXION_ASCII("MDETERM"), formula_function_t::func_mdeterm },
    { IXION_ASCII("MEDIAN"), formula_function_t::func_median },
    { IXION_ASCII("MID"), formula_function_t::func_mid },
    { IXION_ASCII("MIDB"), formula_function_t::func_midb },
    { IXION_ASCII("MIN"), formula_function_t::func_min },
    { IXION_ASCII("MINA"), formula_function_t::func_mina },
    { IXION_ASCII("MINIFS"), formula_function_t::func_minifs },
    { IXION_ASCII("MINUTE"), formula_function_t::func_minute },
    { IXION_ASCII("MINVERSE"), formula_function_t::func_minverse },
    { IXION_ASCII("MIRR"), formula_function_t::func_mirr },
    { IXION_ASCII("MMULT"), formula_function_t::func_mmult },
    { IXION_ASCII("MOD"), formula_function_t::func_mod },
    { IXION_ASCII("MODE"), formula_function_t::func_mode },
    { IXION_ASCII("MONTH"), formula_function_t::func_month },
    { IXION_ASCII("MULTIRANGE"), formula_function_t::func_multirange },
    { IXION_ASCII("MUNIT"), formula_function_t::func_munit },
    { IXION_ASCII("MVALUE"), formula_function_t::func_mvalue },
    { IXION_ASCII("N"), formula_function_t::func_n },
    { IXION_ASCII("NA"), formula_function_t::func_na },
    { IXION_ASCII("NEG"), formula_function_t::func_neg },
    { IXION_ASCII("NEGBINOMDIST"), formula_function_t::func_negbinomdist },
    { IXION_ASCII("NETWORKDAYS"), formula_function_t::func_networkdays },
    { IXION_ASCII("NOMINAL"), formula_function_t::func_nominal },
    { IXION_ASCII("NORMDIST"), formula_function_t::func_normdist },
    { IXION_ASCII("NORMINV"), formula_function_t::func_norminv },
    { IXION_ASCII("NORMSDIST"), formula_function_t::func_normsdist },
    { IXION_ASCII("NORMSINV"), formula_function_t::func_normsinv },
    { IXION_ASCII("NOT"), formula_function_t::func_not },
    { IXION_ASCII("NOW"), formula_function_t::func_now },
    { IXION_ASCII("NPER"), formula_function_t::func_nper },
    { IXION_ASCII("NPV"), formula_function_t::func_npv },
    { IXION_ASCII("NUMBERVALUE"), formula_function_t::func_numbervalue },
    { IXION_ASCII("ODD"), formula_function_t::func_odd },
    { IXION_ASCII("OFFSET"), formula_function_t::func_offset },
    { IXION_ASCII("OR"), formula_function_t::func_or },
    { IXION_ASCII("PDURATION"), formula_function_t::func_pduration },
    { IXION_ASCII("PEARSON"), formula_function_t::func_pearson },
    { IXION_ASCII("PERCENTILE"), formula_function_t::func_percentile },
    { IXION_ASCII("PERCENTRANK"), formula_function_t::func_percentrank },
    { IXION_ASCII("PERMUT"), formula_function_t::func_permut },
    { IXION_ASCII("PERMUTATIONA"), formula_function_t::func_permutationa },
    { IXION_ASCII("PHI"), formula_function_t::func_phi },
    { IXION_ASCII("PI"), formula_function_t::func_pi },
    { IXION_ASCII("PMT"), formula_function_t::func_pmt },
    { IXION_ASCII("POISSON"), formula_function_t::func_poisson },
    { IXION_ASCII("POWER"), formula_function_t::func_power },
    { IXION_ASCII("PPMT"), formula_function_t::func_ppmt },
    { IXION_ASCII("PROB"), formula_function_t::func_prob },
    { IXION_ASCII("PRODUCT"), formula_function_t::func_product },
    { IXION_ASCII("PROPER"), formula_function_t::func_proper },
    { IXION_ASCII("PV"), formula_function_t::func_pv },
    { IXION_ASCII("QUARTILE"), formula_function_t::func_quartile },
    { IXION_ASCII("RADIANS"), formula_function_t::func_radians },
    { IXION_ASCII("RAND"), formula_function_t::func_rand },
    { IXION_ASCII("RANK"), formula_function_t::func_rank },
    { IXION_ASCII("RATE"), formula_function_t::func_rate },
    { IXION_ASCII("RAWSUBTRACT"), formula_function_t::func_rawsubtract },
    { IXION_ASCII("REGEX"), formula_function_t::func_regex },
    { IXION_ASCII("REPLACE"), formula_function_t::func_replace },
    { IXION_ASCII("REPLACEB"), formula_function_t::func_replaceb },
    { IXION_ASCII("REPT"), formula_function_t::func_rept },
    { IXION_ASCII("RIGHT"), formula_function_t::func_right },
    { IXION_ASCII("RIGHTB"), formula_function_t::func_rightb },
    { IXION_ASCII("ROMAN"), formula_function_t::func_roman },
    { IXION_ASCII("ROUND"), formula_function_t::func_round },
    { IXION_ASCII("ROUNDDOWN"), formula_function_t::func_rounddown },
    { IXION_ASCII("ROUNDSIG"), formula_function_t::func_roundsig },
    { IXION_ASCII("ROUNDUP"), formula_function_t::func_roundup },
    { IXION_ASCII("ROW"), formula_function_t::func_row },
    { IXION_ASCII("ROWS"), formula_function_t::func_rows },
    { IXION_ASCII("RRI"), formula_function_t::func_rri },
    { IXION_ASCII("RSQ"), formula_function_t::func_rsq },
    { IXION_ASCII("SEARCH"), formula_function_t::func_search },
    { IXION_ASCII("SEARCHB"), formula_function_t::func_searchb },
    { IXION_ASCII("SEC"), formula_function_t::func_sec },
    { IXION_ASCII("SECH"), formula_function_t::func_sech },
    { IXION_ASCII("SECOND"), formula_function_t::func_second },
    { IXION_ASCII("SHEET"), formula_function_t::func_sheet },
    { IXION_ASCII("SHEETS"), formula_function_t::func_sheets },
    { IXION_ASCII("SIGN"), formula_function_t::func_sign },
    { IXION_ASCII("SIN"), formula_function_t::func_sin },
    { IXION_ASCII("SINH"), formula_function_t::func_sinh },
    { IXION_ASCII("SKEW"), formula_function_t::func_skew },
    { IXION_ASCII("SKEWP"), formula_function_t::func_skewp },
    { IXION_ASCII("SLN"), formula_function_t::func_sln },
    { IXION_ASCII("SLOPE"), formula_function_t::func_slope },
    { IXION_ASCII("SMALL"), formula_function_t::func_small },
    { IXION_ASCII("SQRT"), formula_function_t::func_sqrt },
    { IXION_ASCII("STANDARDIZE"), formula_function_t::func_standardize },
    { IXION_ASCII("STDEV"), formula_function_t::func_stdev },
    { IXION_ASCII("STDEVA"), formula_function_t::func_stdeva },
    { IXION_ASCII("STDEVP"), formula_function_t::func_stdevp },
    { IXION_ASCII("STDEVPA"), formula_function_t::func_stdevpa },
    { IXION_ASCII("STEYX"), formula_function_t::func_steyx },
    { IXION_ASCII("STYLE"), formula_function_t::func_style },
    { IXION_ASCII("SUBSTITUTE"), formula_function_t::func_substitute },
    { IXION_ASCII("SUBTOTAL"), formula_function_t::func_subtotal },
    { IXION_ASCII("SUM"), formula_function_t::func_sum },
    { IXION_ASCII("SUMIF"), formula_function_t::func_sumif },
    { IXION_ASCII("SUMIFS"), formula_function_t::func_sumifs },
    { IXION_ASCII("SUMPRODUCT"), formula_function_t::func_sumproduct },
    { IXION_ASCII("SUMSQ"), formula_function_t::func_sumsq },
    { IXION_ASCII("SUMX2MY2"), formula_function_t::func_sumx2my2 },
    { IXION_ASCII("SUMX2PY2"), formula_function_t::func_sumx2py2 },
    { IXION_ASCII("SUMXMY2"), formula_function_t::func_sumxmy2 },
    { IXION_ASCII("SWITCH"), formula_function_t::func_switch },
    { IXION_ASCII("SYD"), formula_function_t::func_syd },
    { IXION_ASCII("T"), formula_function_t::func_t },
    { IXION_ASCII("TAN"), formula_function_t::func_tan },
    { IXION_ASCII("TANH"), formula_function_t::func_tanh },
    { IXION_ASCII("TDIST"), formula_function_t::func_tdist },
    { IXION_ASCII("TEXT"), formula_function_t::func_text },
    { IXION_ASCII("TEXTJOIN"), formula_function_t::func_textjoin },
    { IXION_ASCII("TIME"), formula_function_t::func_time },
    { IXION_ASCII("TIMEVALUE"), formula_function_t::func_timevalue },
    { IXION_ASCII("TINV"), formula_function_t::func_tinv },
    { IXION_ASCII("TODAY"), formula_function_t::func_today },
    { IXION_ASCII("TRANSPOSE"), formula_function_t::func_transpose },
    { IXION_ASCII("TREND"), formula_function_t::func_trend },
    { IXION_ASCII("TRIM"), formula_function_t::func_trim },
    { IXION_ASCII("TRIMMEAN"), formula_function_t::func_trimmean },
    { IXION_ASCII("TRUE"), formula_function_t::func_true },
    { IXION_ASCII("TRUNC"), formula_function_t::func_trunc },
    { IXION_ASCII("TTEST"), formula_function_t::func_ttest },
    { IXION_ASCII("TYPE"), formula_function_t::func_type },
    { IXION_ASCII("UNICHAR"), formula_function_t::func_unichar },
    { IXION_ASCII("UNICODE"), formula_function_t::func_unicode },
    { IXION_ASCII("UPPER"), formula_function_t::func_upper },
    { IXION_ASCII("VALUE"), formula_function_t::func_value },
    { IXION_ASCII("VAR"), formula_function_t::func_var },
    { IXION_ASCII("VARA"), formula_function_t::func_vara },
    { IXION_ASCII("VARP"), formula_function_t::func_varp },
    { IXION_ASCII("VARPA"), formula_function_t::func_varpa },
    { IXION_ASCII("VDB"), formula_function_t::func_vdb },
    { IXION_ASCII("VLOOKUP"), formula_function_t::func_vlookup },
    { IXION_ASCII("WAIT"), formula_function_t::func_wait },
    { IXION_ASCII("WEBSERVICE"), formula_function_t::func_webservice },
    { IXION_ASCII("WEEKDAY"), formula_function_t::func_weekday },
    { IXION_ASCII("WEEKNUM"), formula_function_t::func_weeknum },
    { IXION_ASCII("WEIBULL"), formula_function_t::func_weibull },
    { IXION_ASCII("XOR"), formula_function_t::func_xor },
    { IXION_ASCII("YEAR"), formula_function_t::func_year },
    { IXION_ASCII("ZTEST"), formula_function_t::func_ztest },
};

const map_type& get()
{
    static map_type mt(entries, std::size(entries), formula_function_t::func_unknown);
    return mt;
}

} // builtin_funcs namespace

constexpr std::string_view unknown_func_name = "unknown";

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
    for (const builtin_funcs::map_type::entry& e : builtin_funcs::entries)
    {
        if (e.value == oc)
            return e.key;
    }
    return unknown_func_name;
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
