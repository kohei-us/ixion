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

#include "inputparser.hpp"
#include "cell.hpp"
#include "formula_lexer.hpp"
#include "formula_parser.hpp"
#include "depends_tracker.hpp"

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
    formula_cell_inserter(ptr_map<string, base_cell>& cell_map) :
        m_cell_map(cell_map) {}

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
    const vector<string>& cell_names, ptr_map<string, base_cell>& cell_map, map<const base_cell*, string>& ptr_name_map)
{
    ensure_unique_names(cell_names);

    typedef ptr_map<string, base_cell> _cellmap_type;
    typedef map<const base_cell*, string> _ptrname_type;
    for_each(cell_names.begin(), cell_names.end(), formula_cell_inserter(cell_map));
    _cellmap_type::const_iterator itr = cell_map.begin(), itr_end = cell_map.end();

    // debug output.
    for (; itr != itr_end; ++itr)
    {    
        cout << itr->first << " := " << itr->second << endl;
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
bool parse_model_input(const string& fpath, const string& dotpath)
{
    try
    {
        // Read the model definition file, and parse the model cells.
        model_parser parser(fpath);
        parser.parse();

        // First, create empty formula cell instances so that we can have 
        // name-to-pointer associations.
        const vector<string>& cell_names = parser.get_cell_names();

        ptr_map<string, base_cell> cell_map;
        depends_tracker::ptr_name_map_type ptr_name_map;
        create_empty_formula_cells(cell_names, cell_map, ptr_name_map);

        depends_tracker deptracker(&ptr_name_map);
        const vector<model_parser::cell>& cells = parser.get_cells();
        size_t cell_count = cells.size();
        for (size_t i = 0; i < cell_count; ++i)
        {   
            const model_parser::cell& cell = cells[i]; 
            cout << "parsing cell " << cell.get_name() << " (initial content:" << cell.print() << ")" << endl;
            formula_parser fparser(cell.get_tokens(), &cell_map);
            fparser.parse();

            // Put the formula tokens into formula cell instance.
            ptr_map<string, base_cell>::iterator itr = cell_map.find(cell.get_name());
            if (itr == cell_map.end())
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
        deptracker.topo_sort_cells();
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

const char* model_parser::cell::print() const
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

                    // test-print tokens.
                    cout << "tokens from lexer: " << print_tokens(tokens, true) << endl;

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

