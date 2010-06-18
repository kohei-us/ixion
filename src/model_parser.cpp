/*************************************************************************
 *
 * Copyright (c) 2010 Kohei Yoshida
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

#include "model_parser.hpp"
#include "cell.hpp"
#include "formula_lexer.hpp"
#include "formula_parser.hpp"
#include "depends_tracker.hpp"
#include "formula_interpreter.hpp"
#include "formula_result.hpp"
#include "mem_str_buf.hpp"

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <functional>
#include <cstring>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/assign/ptr_map_inserter.hpp>

using namespace std;
using ::boost::ptr_map;
using ::boost::assign::ptr_map_insert;

#include <stdio.h>
#include <string>
#include <sys/time.h>

#define DEBUG_INPUT_PARSER 0

namespace {

class StackPrinter
{
public:
    explicit StackPrinter(const char* msg) :
        msMsg(msg)
    {
        fprintf(stdout, "%s: --begin\n", msMsg.c_str());
        mfStartTime = getTime();
    }

    ~StackPrinter()
    {
        double fEndTime = getTime();
        fprintf(stdout, "%s: --end (duration: %g sec)\n", msMsg.c_str(), (fEndTime-mfStartTime));
    }

    void printTime(int line) const
    {
        double fEndTime = getTime();
        fprintf(stdout, "%s: --(%d) (duration: %g sec)\n", msMsg.c_str(), line, (fEndTime-mfStartTime));
    }

private:
    double getTime() const
    {
        timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec + tv.tv_usec / 1000000.0;
    }

    ::std::string msMsg;
    double mfStartTime;
};

}

namespace ixion {

namespace {

void load_file_content(const string& filepath, string& content)
{
    ifstream file(filepath.c_str());
    if (!file)
        // failed to open the specified file.
        throw model_parser::file_not_found(filepath);

    ostringstream os;
    os << file.rdbuf() << ' '; // extra char as the end position.
    file.close();

    os.str().swap(content);
}

enum parse_mode_t
{
    parse_mode_unknown = 0,
    parse_mode_init,
    parse_mode_result,
    parse_mode_edit
};

struct parse_data
{
    vector<model_parser::cell>  cells;
    vector<string>              cell_names;
    model_parser::results_type  formula_results;
};

void parse_init(const char*& p, parse_data& data)
{
    mem_str_buf buf, name, formula;
    for (; *p != '\n'; ++p)
    {
        if (*p == '=')
        {
            if (buf.empty())
                throw model_parser::parse_error("left hand side is empty");

            name = buf;
            buf.clear();
            data.cell_names.push_back(name.str());
        }
        else
        {
            if (buf.empty())
                buf.set_start(p);
            else
                buf.inc();
        }
    }

    if (!buf.empty())
    {
        if (name.empty())
            throw model_parser::parse_error("'=' is missing");

        formula = buf;

        // tokenize the formula string, and create a formula cell 
        // with the tokens.
        formula_lexer lexer(formula);
        lexer.tokenize();
        lexer_tokens_t tokens;
        lexer.swap_tokens(tokens);

#if DEBUG_INPUT_PARSER
        // test-print tokens.
        cout << "tokens from lexer: " << print_tokens(tokens, true) << endl;
#endif

        model_parser::cell fcell(name.str(), tokens);
        data.cells.push_back(fcell);
    }
}

void parse_result(const char*& p, model_parser::results_type& results)
{
    mem_str_buf buf, name, result;
    for (; *p != '\n'; ++p)
    {
        if (*p == '=')
        {
            if (buf.empty())
                throw model_parser::parse_error("left hand side is empty");

            name = buf;
            buf.clear();
        }
        else
        {
            if (buf.empty())
                buf.set_start(p);
            else
                buf.inc();
        }
    }

    if (!buf.empty())
    {
        if (name.empty())
            throw model_parser::parse_error("'=' is missing");

        result = buf;
    }

    string name_s = name.str();
    formula_result res;
    res.parse(result.str());
    model_parser::results_type::iterator itr = results.find(name_s);
    if (itr == results.end())
    {
        // This cell doesn't exist yet.
        pair<model_parser::results_type::iterator, bool> r = 
            results.insert(model_parser::results_type::value_type(name_s, res));
        if (!r.second)
            throw model_parser::parse_error("failed to insert a new result.");
    }
    else
        itr->second = res;
}

void parse_command(const char*& p, mem_str_buf& com)
{
    mem_str_buf _com;
    ++p; // skip '%'.
    _com.set_start(p);
    for (++p; *p != '\n'; ++p)
        _com.inc();
    _com.swap(com);
}

class formula_cell_inserter : public unary_function<string, void>
{
public:
    formula_cell_inserter(ptr_map<string, base_cell>& cell_name_ptr_map) :
        m_cell_map(cell_name_ptr_map) {}

    void operator() (const string& name)
    {
        // This inserts a new'ed formula_cell instance associated with the name.
        ptr_map_insert<formula_cell>(m_cell_map)(name);
    }

private:
    ptr_map<string, base_cell>& m_cell_map;
};

class ref_cell_picker : public unary_function<formula_token_base*, void>
{
public:
    ref_cell_picker(vector<base_cell*>& deps) :
        m_deps(deps) {}

    void operator() (formula_token_base* p)
    {
        if (p->get_opcode() != fop_single_ref)
            return;

        const base_cell* refcell = static_cast<single_ref_token*>(p)->get_single_ref();
        m_deps.push_back(const_cast<base_cell*>(refcell));
    }

private:
    vector<base_cell*>& m_deps;
};

class depcell_inserter : public unary_function<base_cell*, void>
{
public:
    depcell_inserter(depends_tracker& tracker, formula_cell* fcell) : m_tracker(tracker), mp_fcell(fcell) {}

    void operator() (base_cell* p)
    {
        m_tracker.insert_depend(mp_fcell, p);
    }
private:
    depends_tracker& m_tracker;
    formula_cell* mp_fcell;
};

void ensure_unique_names(const vector<string>& cell_names)
{
    vector<string> names = cell_names;
    sort(names.begin(), names.end());
    if (unique(names.begin(), names.end()) != names.end())
        throw general_error("Duplicate names exist in the list of cell names.");
}

void create_empty_formula_cells(
    const vector<string>& cell_names, cell_name_ptr_map_t& cell_map)
{
    ensure_unique_names(cell_names);
    cell_map.clear();

    typedef ptr_map<string, base_cell> _cellmap_type;
    typedef map<const base_cell*, string> _ptrname_type;
    for_each(cell_names.begin(), cell_names.end(), formula_cell_inserter(cell_map));
    _cellmap_type::const_iterator itr = cell_map.begin(), itr_end = cell_map.end();

#if DEBUG_INPUT_PARSER
    for (; itr != itr_end; ++itr)
        cout << itr->first << " := " << itr->second << endl;
#endif
}

void convert_lexer_tokens(const vector<model_parser::cell>& cells, cell_name_ptr_map_t& formula_cells)
{
    vector<model_parser::cell>::const_iterator itr_cell = cells.begin(), itr_cell_end = cells.end();
    for (; itr_cell != itr_cell_end; ++itr_cell)
    {   
        const model_parser::cell& cell = *itr_cell;
#if DEBUG_INPUT_PARSER
        cout << "parsing cell " << cell.get_name() << " (initial content:" << cell.print() << ")" << endl;
#endif
        // Parse the lexer tokens and turn them into formula tokens.
        formula_parser fparser(cell.get_tokens(), &formula_cells);
        fparser.parse();
        fparser.print_tokens();

        // Put the formula tokens into formula cell instance.
        const string & name = cell.get_name();
        cell_name_ptr_map_t::iterator itr = formula_cells.find(name);
        if (itr == formula_cells.end())
        {
            // Insert a new formula cell instance.
            ptr_map_insert<formula_cell>(formula_cells)(name);
            itr = formula_cells.find(name);
            if (itr == formula_cells.end())
                throw general_error("inserted formula cell instance not found");
        }

        // Transfer formula tokens from the parser to the cell.
        formula_cell* fcell = static_cast<formula_cell*>(itr->second);
        fcell->swap_tokens(fparser.get_tokens());
    }
}

}

/** 
 * This method does the following: 
 *  
 * <ol> 
 * <li>Read the input file, and parse the definition of each model 
 * "cell" and tokenize it into a series of lexer tokens.</li> 
 * <li>Create instances of formula cells, store them and map them 
 * with names.  The mapping will be used when resolving model cell's 
 * names to pointers of their corresponding formula cell instances.</li> 
 * <li>Parse the lexer tokens for each model cell, and convert them 
 * into formula tokens.  At this point, referenced cell names stored in 
 * the lexer tokens get converted into formula cell pointers.</li> 
 * <li>Pass the formula tokens into corresponding formula cell instances, 
 * and pass the cell dependency data to dependency tracker class instance.
 * </li> 
 * </ol>
 *
 * @param fpath path to the input model file. 
 * 
 * @return true if the conversion is successful, false otherwise.
 */
bool parse_model_input(const string& fpath, const string& dotpath, size_t thread_count)
{
#if DEBUG_INPUT_PARSER
    StackPrinter __stack_printer__("ixion::parse_model_input");
#endif
    try
    {
        // Read the model definition file, and parse the model cells. The
        // model parser parses each line and break it into lexer tokens.
        model_parser parser(fpath, thread_count);
        parser.parse();
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
        return false;
    }
    return true;
}

// ============================================================================

model_parser::file_not_found::file_not_found(const string& fpath) : 
    m_fpath(fpath)
{
}

model_parser::file_not_found::~file_not_found() throw()
{
}

const char* model_parser::file_not_found::what() const throw()
{
    ostringstream oss;
    oss << "specified file not found: " << m_fpath;
    return oss.str().c_str();
}

// ============================================================================

model_parser::parse_error::parse_error(const string& msg) :
    m_msg(msg)
{
}

model_parser::parse_error::~parse_error() throw()
{
}

const char* model_parser::parse_error::what() const throw()
{
    ostringstream oss;
    oss << "parse error: " << m_msg;
    return oss.str().c_str();
}

// ============================================================================

model_parser::check_error::check_error(const string& msg) :
    general_error(msg) {}

// ============================================================================

model_parser::cell::cell(const string& name, lexer_tokens_t& tokens) :
    m_name(name)
{
    // Note that this will empty the passed token container !
    m_tokens.swap(tokens);
}

model_parser::cell::cell(const model_parser::cell& r) :
    m_name(r.m_name),
    m_tokens(r.m_tokens)
{
}

model_parser::cell::~cell()
{
}

string model_parser::cell::print() const
{
    return print_tokens(m_tokens, false);
}

const string& model_parser::cell::get_name() const
{
    return m_name;
}

const lexer_tokens_t& model_parser::cell::get_tokens() const
{
    return m_tokens;
}

// ============================================================================

model_parser::model_parser(const string& filepath, size_t thread_count) :
    m_filepath(filepath), m_thread_count(thread_count)
{
}

model_parser::~model_parser()
{
}

void model_parser::parse()
{
    string strm;
    load_file_content(m_filepath, strm);

    parse_mode_t parse_mode = parse_mode_unknown;
    const char *p = &strm[0], *p_last = &strm[strm.size()-1];
    parse_data data;

    for (; p != p_last; ++p)
    {
        if (*p == '%')
        {
            // This line contains a command.
            mem_str_buf buf_com;
            parse_command(p, buf_com);
            if (buf_com.equals("calc"))
            {
                if (parse_mode != parse_mode_init)
                    throw parse_error("'calc' command must be used in the init mode.");

                // Perform full calculation on currently stored cells.

                // First, create empty formula cell instances so that we can have 
                // name-to-pointer associations.
                create_empty_formula_cells(data.cell_names, m_cells);
                convert_lexer_tokens(data.cells, m_cells);
                calc();
            }
            else if (buf_com.equals("recalc"))
            {
                if (parse_mode != parse_mode_edit)
                    throw parse_error("'recalc' command must be used in the edit mode.");

                ostringstream os;
                os << get_formula_result_output_separator() << endl
                    << "recalculating" << endl;
                cout << os.str();
                convert_lexer_tokens(data.cells, m_cells);
                calc();
            }
            else if (buf_com.equals("check"))
            {
                // Check cell results.
                check(data.formula_results);
            }
            else if (buf_com.equals("mode init"))
            {
                parse_mode = parse_mode_init;
            }
            else if (buf_com.equals("mode result"))
            {
                parse_mode = parse_mode_result;
            }
            else if (buf_com.equals("mode edit"))
            {
                parse_mode = parse_mode_edit;
                data.cells.clear();
                data.cell_names.clear();
            }
            else
            {
                ostringstream os;
                os << "unknown command: " << buf_com.str() << endl;
                throw parse_error(os.str());
            }
            continue;
        }

        switch (parse_mode)
        {
            case parse_mode_init:
                parse_init(p, data);
            break;
            case parse_mode_edit:
                parse_init(p, data);
            break;
            case parse_mode_result:
                parse_result(p, data.formula_results);
            break;
            default:
                throw parse_error("unknown parse mode");
        }
    }
}

void model_parser::calc()
{
    cell_ptr_name_map_t cell_names;
    build_ptr_name_map(m_cells, cell_names);
    global::set_cell_name_map(&cell_names);

    depends_tracker deptracker(&cell_names);
    cell_name_ptr_map_t::iterator itr = m_cells.begin(), itr_end = m_cells.end();
    for (; itr != itr_end; ++itr)
    {
        base_cell* pcell = itr->second;
        if (pcell->get_celltype() != celltype_formula)
            throw general_error("formula cell is expected but not found");

        // Register cell dependencies.
        formula_cell* fcell = static_cast<formula_cell*>(pcell);
        vector<formula_token_base*> ref_tokens;
        fcell->get_ref_tokens(ref_tokens);

        // Pick up referenced cells from ref tokens.  Combine this with the 
        // above get_ref_tokens() call above for efficiency.
        vector<base_cell*> deps;
        for_each(ref_tokens.begin(), ref_tokens.end(), ref_cell_picker(deps));

        for_each(deps.begin(), deps.end(), depcell_inserter(deptracker, fcell));
    }
    deptracker.interpret_all_cells(m_thread_count);
}

void model_parser::check(const results_type& formula_results)
{
    cout << get_formula_result_output_separator() << endl
         << "checking results" << endl
         << get_formula_result_output_separator() << endl;

    results_type::const_iterator itr = formula_results.begin(), itr_end = formula_results.end();
    for (; itr != itr_end; ++itr)
    {
        const string& name = itr->first;
        const formula_result& res = itr->second;
        cout << name << " : " << res.str() << endl;
        const base_cell* pcell = get_cell(name);
        if (!pcell)
        {
            ostringstream os;
            os << "specified cell instance not found: " << name;
            throw check_error(os.str());
        }

        if (pcell->get_celltype() != celltype_formula)
            throw check_error("expected formula cell");

        const formula_result* res_cell = static_cast<const formula_cell*>(pcell)->get_result_cache();
        if (!res_cell)
            throw check_error("result is not cached");

        if (*res_cell != res)
        {
            ostringstream os;
            os << "unexpected result: (expected: " << res.str() << "; actual: " << res_cell->str() << ")";
            throw check_error(os.str());
        }
    }
}

const base_cell* model_parser::get_cell(const string& name) const
{
    cell_name_ptr_map_t::const_iterator itr = m_cells.find(name);
    return itr == m_cells.end() ? NULL : itr->second;
}

}

