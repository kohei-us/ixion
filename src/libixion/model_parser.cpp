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
#include "formula_name_resolver.hpp"
#include "formula_result.hpp"
#include "mem_str_buf.hpp"

#include <sstream>
#include <iostream>
#include <vector>
#include <functional>
#include <cstring>
#include <cassert>
#include <memory>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/assign/ptr_map_inserter.hpp>

using namespace std;
using ::boost::ptr_map;
using ::boost::assign::ptr_map_insert;

#define DEBUG_INPUT_PARSER 0

namespace ixion {

namespace {

// ============================================================================
// Function objects

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
    ref_cell_picker(const model_context& cxt, vector<base_cell*>& deps) :
        m_context(cxt), m_deps(deps) {}

    void operator() (formula_token_base* p)
    {
        if (p->get_opcode() != fop_single_ref)
            return;

        address_t addr = static_cast<single_ref_token*>(p)->get_single_ref();
        const base_cell* refcell = m_context.get_cell(addr);
        if (refcell)
            m_deps.push_back(const_cast<base_cell*>(refcell));
    }

private:
    const model_context& m_context;
    vector<base_cell*>& m_deps;
};

class depcell_inserter : public unary_function<base_cell*, void>
{
public:
    depcell_inserter(dependency_tracker& tracker, const dirty_cells_t& dirty_cells, formula_cell* fcell) : 
        m_tracker(tracker), 
        m_dirty_cells(dirty_cells),
        mp_fcell(fcell) {}

    void operator() (base_cell* p)
    {
        if (m_dirty_cells.count(p) > 0)
            m_tracker.insert_depend(mp_fcell, p);
    }
private:
    dependency_tracker& m_tracker;
    const dirty_cells_t& m_dirty_cells;
    formula_cell* mp_fcell;
};

class formula_cell_listener_handler : public unary_function<formula_token_base*, void>
{
public:
    enum mode_t { mode_add, mode_remove };

    explicit formula_cell_listener_handler(model_context& cxt, const address_t& addr, mode_t mode) : 
        m_context(cxt), m_addr(addr), m_mode(mode) {}

    void operator() (formula_token_base* p) const
    {
        if (p->get_opcode() == fop_single_ref)
        {
            address_t addr = p->get_single_ref();
            base_cell* cell = m_context.get_cell(addr);
            if (!cell)
                return;

            if (m_mode == mode_add)
            {
                cell->add_listener(m_addr);
            }
            else
            {
                assert(m_mode == mode_remove);
                cell->remove_listener(m_addr);
            }
        }
    }

private:
    model_context& m_context;
    const address_t& m_addr;
    formula_cell* mp_cell;
    mode_t m_mode;
};

class formula_cell_printer : public unary_function<const base_cell*, void>
{
    const model_context& m_context;
public:
    formula_cell_printer(const model_context& cxt) : m_context(cxt) {}

    void operator() (const base_cell* p) const
    {
        string name = "<unknown>";
        if (p->get_celltype() == celltype_formula)
        {
            string expr_name = m_context.get_cell_name(p);
            if (!expr_name.empty())
                name = expr_name;
        }
        cout << "  cell " << name << endl;
    }
};

class ptr_name_map_builder : public unary_function<const base_cell*, void>
{
public:
    ptr_name_map_builder(cell_ptr_name_map_t& map) : 
        m_map(map) {}

    void operator() (const base_cell* p) const
    {
        m_map.insert(cell_ptr_name_map_t::value_type(p, global::get_cell_name(p)));
    }
private:
    cell_ptr_name_map_t& m_map;
};

class cell_dependency_handler : public unary_function<base_cell*, void>
{
public:
    explicit cell_dependency_handler(const model_context& cxt, dependency_tracker& dep_tracker, dirty_cells_t& dirty_cells) :
        m_context(cxt), m_dep_tracker(dep_tracker), m_dirty_cells(dirty_cells) {}

    void operator() (base_cell* pcell)
    {
        if (pcell->get_celltype() != celltype_formula)
            return;

#if DEBUG_INPUT_PARSER
        cout << get_formula_result_output_separator() << endl;
        cout << "processing dependency of " << m_context.get_cell_name(pcell) << endl;
#endif
        // Register cell dependencies.
        formula_cell* fcell = static_cast<formula_cell*>(pcell);
        vector<formula_token_base*> ref_tokens;
        fcell->get_ref_tokens(ref_tokens);

#if DEBUG_INPUT_PARSER
        cout << "this cell contains " << ref_tokens.size() << " reference tokens." << endl;
#endif
        // Pick up the referenced cells from the ref tokens.  I should
        // probably combine this with the above get_ref_tokens() call above
        // for efficiency.
        vector<base_cell*> deps;
        for_each(ref_tokens.begin(), ref_tokens.end(), ref_cell_picker(m_context, deps));

#if DEBUG_INPUT_PARSER
        cout << "number of precedent cells picked up: " << deps.size() << endl;
#endif
        // Register dependency information.  Only dirty cells should be
        // registered as precedent cells since non-dirty cells are equivalent
        // to constants.
        for_each(deps.begin(), deps.end(), depcell_inserter(m_dep_tracker, m_dirty_cells, fcell));
    }

private:
    const model_context& m_context;
    dependency_tracker& m_dep_tracker;
    dirty_cells_t& m_dirty_cells;
};

// ============================================================================

enum parse_mode_t
{
    parse_mode_unknown = 0,
    parse_mode_init,
    parse_mode_result,
    parse_mode_edit
};

/**
 * A bucket of data used during model definition parsing.
 */
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

void ensure_unique_names(const vector<string>& cell_names)
{
    vector<string> names = cell_names;
    sort(names.begin(), names.end());
    if (unique(names.begin(), names.end()) != names.end())
        throw general_error("Duplicate names exist in the list of cell names.");
}

void convert_lexer_tokens(const vector<model_parser::cell>& cells, model_context& context, dirty_cells_t& dirty_cells)
{
    typedef ::std::pair<address_t, formula_cell*> address_cell_pair_type;

    dirty_cells_t _dirty_cells;
    vector<address_cell_pair_type> fcells;
    vector<model_parser::cell>::const_iterator itr_cell = cells.begin(), itr_cell_end = cells.end();
    for (; itr_cell != itr_cell_end; ++itr_cell)
    {   
        const model_parser::cell& model_cell = *itr_cell;
        const string& name = model_cell.get_name();

#if DEBUG_INPUT_PARSER
        cout << "parsing cell " << name << " (initial content:" << model_cell.print() << ")" << endl;
#endif
        // Parse the lexer tokens and turn them into formula tokens.
        formula_parser fparser(model_cell.get_tokens(), context);
        fparser.parse();
        fparser.print_tokens();

        formula_cell* fcell = NULL;
        formula_name_type name_type = context.get_name_resolver().resolve(name);
        switch (name_type.type)
        {
            case formula_name_type::named_expression:
                fcell = context.get_named_expression(name);
                if (!fcell)
                {
                    // No prior named expression with the same name.
                    auto_ptr<formula_cell> pcell(new formula_cell);
                    context.set_named_expression(name, pcell);
                    fcell = context.get_named_expression(name);
                }
                if (!fcell)
                    throw general_error("failed to insert a named expression");
            break;
            case formula_name_type::cell_reference:
            {
                address_t addr;
                addr.sheet = name_type.address.sheet;
                addr.row = name_type.address.row;
                addr.column = name_type.address.col;
                base_cell* p = context.get_cell(addr);
                if (!p)
                {
                    // No prior cell at this address. Create a new one.
                    auto_ptr<base_cell> pcell(new formula_cell);
                    context.set_cell(addr, pcell);
                    fcell = static_cast<formula_cell*>(context.get_cell(addr));
                }
                else if (p->get_celltype() != celltype_formula)
                {
                    // Prior cell exists, but it's not a formula cell.
                    // Transfer the listener cells over to the new cell
                    // instance.
                    auto_ptr<base_cell> pcell(new formula_cell);
                    pcell->swap_listeners(*p);
                    context.set_cell(addr, pcell);
                    fcell = static_cast<formula_cell*>(context.get_cell(addr));
                }
                else
                {
                    // Pre-existing formula cell.
                    fcell = static_cast<formula_cell*>(p);
                }

                if (!fcell)
                    throw general_error("failed to insert a new formula cell");

                fcells.push_back(
                    address_cell_pair_type(addr, fcell));
            }
            break;
            default:
                throw general_error("unknown name type.");
        }

        assert(fcell);

        // Put the formula tokens into formula cell instance, and put the 
        // formula cell into context.
        fcell->swap_tokens(fparser.get_tokens());
    }

    vector<address_cell_pair_type>::iterator itr_fcell = fcells.begin(), itr_fcell_end = fcells.end();
    for (; itr_fcell != itr_fcell_end; ++itr_fcell)
    {
        formula_cell* fcell = itr_fcell->second;
#if DEBUG_INPUT_PARSER
        cout << "processing formula cell at " << context.get_cell_name(fcell) << endl;
#endif
        // Now, register the formula cell as a listener to all its references.
        vector<formula_token_base*> ref_tokens;
        fcell->get_ref_tokens(ref_tokens);
#if DEBUG_INPUT_PARSER
        cout << "  number of reference tokens: " << ref_tokens.size() << endl;
#endif
        for_each(ref_tokens.begin(), ref_tokens.end(), 
                 formula_cell_listener_handler(context,
                     itr_fcell->first, formula_cell_listener_handler::mode_add));
        
        // Add this cell and all its listeners to the dirty cell list.
        _dirty_cells.insert(fcell);

        fcell->get_all_listeners(context, _dirty_cells);
    }

#if DEBUG_INPUT_PARSER
    cout << get_formula_result_output_separator() << endl;
    vector<address_cell_pair_type>::const_iterator itr = fcells.begin(), itr_end = fcells.end();
    for (; itr != itr_end; ++itr)
    {
        const formula_cell* p = itr->second;
        p->print_listeners(context);
    }

    cout << get_formula_result_output_separator() << endl;
    cout << "All dirty cells: " << endl;
    for_each(_dirty_cells.begin(), _dirty_cells.end(), formula_cell_printer(context));
#endif
    dirty_cells.swap(_dirty_cells);
}

}

// ============================================================================

model_parser::parse_error::parse_error(const string& msg)
{
    ostringstream os;
    os << "parse error: " << msg;
    m_msg = os.str();
}

model_parser::parse_error::~parse_error() throw()
{
}

const char* model_parser::parse_error::what() const throw()
{
    return m_msg.c_str();
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
    global::load_file_content(m_filepath, strm);

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

                ensure_unique_names(data.cell_names);

                // Perform full calculation on currently stored cells.

                // Convert lexer tokens into formula tokens and put them into
                // formula cells.
                dirty_cells_t dirty_cells;
                convert_lexer_tokens(data.cells, m_context, dirty_cells);
                calc(dirty_cells);
            }
            else if (buf_com.equals("recalc"))
            {
                if (parse_mode != parse_mode_edit)
                    throw parse_error("'recalc' command must be used in the edit mode.");

                ostringstream os;
                os << get_formula_result_output_separator() << endl
                    << "recalculating" << endl;
                cout << os.str();
                dirty_cells_t dirty_cells;
                convert_lexer_tokens(data.cells, m_context, dirty_cells);
                calc(dirty_cells);
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

void model_parser::calc(dirty_cells_t& cells)
{
    // Now, register the dependency info on each dirty cell, and interpret all
    // dirty cells.
    dependency_tracker deptracker(cells, m_context);
    for_each(cells.begin(), cells.end(), cell_dependency_handler(m_context, deptracker, cells));
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
        // resolve name and get cell instance from the context.  The cell may
        // be either a real cell, or a named expression.
        const base_cell* pcell = get_cell_from_name(name);
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

const base_cell* model_parser::get_cell_from_name(const string& name)
{
    const formula_name_resolver_base& resolver = m_context.get_name_resolver();
    formula_name_type name_type = resolver.resolve(name);
    switch (name_type.type)
    {
        case formula_name_type::cell_reference:
        {
            const formula_name_type::address_type& addr = name_type.address;
            return m_context.get_cell(address_t(addr.sheet, addr.row, addr.col));
        }
        case formula_name_type::named_expression:
            return m_context.get_named_expression(name);
        default:
            ;
    }
    return NULL;
}

}

