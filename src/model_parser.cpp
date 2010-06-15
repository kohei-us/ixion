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

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <functional>

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

void flush_buffer(vector<char>& buf, string& str)
{
    buf.push_back(0); // null-terminate the buffer.
    str = &buf[0];
    buf.clear();
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
    const vector<string>& cell_names, cell_name_ptr_map_t& cell_map, cell_ptr_name_map_t& ptr_name_map)
{
    ensure_unique_names(cell_names);

    typedef ptr_map<string, base_cell> _cellmap_type;
    typedef map<const base_cell*, string> _ptrname_type;
    for_each(cell_names.begin(), cell_names.end(), formula_cell_inserter(cell_map));
    _cellmap_type::const_iterator itr = cell_map.begin(), itr_end = cell_map.end();

    // debug output.
    for (; itr != itr_end; ++itr)
    {    
#if DEBUG_INPUT_PARSER
        cout << itr->first << " := " << itr->second << endl;
#endif
        ptr_name_map.insert(_ptrname_type::value_type(itr->second, itr->first));
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
        model_parser parser(fpath);
        parser.parse();

        // First, create empty formula cell instances so that we can have 
        // name-to-pointer associations.
        const vector<string>& cell_names = parser.get_cell_names();

        cell_name_ptr_map_t  cell_name_ptr_map;
        cell_ptr_name_map_t  cell_ptr_name_map;
        create_empty_formula_cells(cell_names, cell_name_ptr_map, cell_ptr_name_map);

        depends_tracker deptracker(&cell_ptr_name_map);
        const vector<model_parser::cell>& cells = parser.get_cells();
        vector<model_parser::cell>::const_iterator itr_cell = cells.begin(), itr_cell_end = cells.end();
        for (; itr_cell != itr_cell_end; ++itr_cell)
        {   
            const model_parser::cell& cell = *itr_cell;
#if DEBUG_INPUT_PARSER
            cout << "parsing cell " << cell.get_name() << " (initial content:" << cell.print() << ")" << endl;
#endif
            // Parse the lexer tokens and turn them into formula tokens.
            formula_parser fparser(cell.get_tokens(), &cell_name_ptr_map);
            fparser.parse();
            fparser.print_tokens();

            // Put the formula tokens into formula cell instance.
            ptr_map<string, base_cell>::iterator itr = cell_name_ptr_map.find(cell.get_name());
            if (itr == cell_name_ptr_map.end())
                throw general_error("formula cell not found");

            base_cell* pcell = itr->second;
            if (pcell->get_celltype() != celltype_formula)
                throw general_error("formula cell is expected but not found");

            // Transfer formula tokens from the parser to the cell.
            formula_cell* fcell = static_cast<formula_cell*>(pcell);
            fcell->swap_tokens(fparser.get_tokens());
            assert(fparser.get_tokens().empty());

            // Register cell dependencies.
            const vector<base_cell*>& deps = fparser.get_depend_cells();
            for_each(deps.begin(), deps.end(), depcell_inserter(deptracker, fcell));
        }

        deptracker.print_dot_graph(dotpath);
        deptracker.interpret_all_cells(thread_count);
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
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

model_parser::model_parser(const string& filepath) :
    m_filepath(filepath)
{
}

model_parser::~model_parser()
{
}

void model_parser::parse()
{
    ifstream file(m_filepath.c_str());
    if (!file)
        // failed to open the specified file.
        throw file_not_found(m_filepath);

    char c;
    string name, formula;
    vector<char> buf;
    buf.reserve(255);
    vector<cell> fcells;
    vector<string> cell_names;
    while (file.get(c))
    {
        switch (c)
        {
            case '=':
                if (buf.empty())
                    throw parse_error("left hand side is empty");
                flush_buffer(buf, name);
                cell_names.push_back(name);
                break;
            case '\n':
                if (!buf.empty())
                {
                    if (name.empty())
                        throw parse_error("'=' is missing");

                    flush_buffer(buf, formula);

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

                    cell fcell(name, tokens);
                    fcells.push_back(fcell);
                }
                name.clear();
                formula.clear();
                break;
            default:
                buf.push_back(c);
        }
    }

    m_fcells.swap(fcells);
    m_cell_names.swap(cell_names);
}

const vector<model_parser::cell>& model_parser::get_cells() const
{
    return m_fcells;
}

const vector<string>& model_parser::get_cell_names() const
{
    return m_cell_names;
}

}

