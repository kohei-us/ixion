/*************************************************************************
 *
 * Copyright (c) 2010, 2011 Kohei Yoshida
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

#include "ixion/model_parser.hpp"
#include "ixion/cell.hpp"
#include "ixion/formula_lexer.hpp"
#include "ixion/formula_parser.hpp"
#include "ixion/depends_tracker.hpp"
#include "ixion/formula_interpreter.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/cell_listener_tracker.hpp"

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

#define DEBUG_MODEL_PARSER 0

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
    ref_cell_picker(model_context& cxt, const abs_address_t& origin, vector<base_cell*>& deps) :
        m_context(cxt), m_origin(origin), m_deps(deps) {}

    void operator() (formula_token_base* p)
    {
        switch (p->get_opcode())
        {
            case fop_single_ref:
            {
                abs_address_t addr = p->get_single_ref().to_abs(m_origin);
                const base_cell* refcell = m_context.get_cell(addr);
                if (refcell)
                    m_deps.push_back(const_cast<base_cell*>(refcell));
            }
            break;
            case fop_range_ref:
            {
                abs_range_t range = p->get_range_ref().to_abs(m_origin);
                vector<base_cell*> cells;
                m_context.get_cells(range, cells);
                vector<base_cell*>::const_iterator itr = cells.begin(), itr_end = cells.end();
                for (; itr != itr_end; ++itr)
                    m_deps.push_back(*itr);
            }
            break;
            default:
                ; // ignore the rest.
        }
    }

private:
    model_context& m_context;
    const abs_address_t& m_origin;
    vector<base_cell*>&  m_deps;
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

    explicit formula_cell_listener_handler(model_context& cxt, const abs_address_t& addr, mode_t mode) :
        m_context(cxt), m_listener_tracker(cell_listener_tracker::get(cxt)), m_addr(addr), m_mode(mode)
    {
#if DEBUG_MODEL_PARSER
        __IXION_DEBUG_OUT__ << "formula_cell_listener_handler: cell position=" << m_addr.get_name() << endl;
#endif
    }

    void operator() (formula_token_base* p) const
    {
        switch (p->get_opcode())
        {
            case fop_single_ref:
            {
                abs_address_t addr = p->get_single_ref().to_abs(m_addr);
#if DEBUG_MODEL_PARSER
                __IXION_DEBUG_OUT__ << "formula_cell_listener_handler: ref address=" << addr.get_name() << endl;
#endif
                if (m_mode == mode_add)
                {
                    m_listener_tracker.add(m_addr, addr);
                }
                else
                {
                    assert(m_mode == mode_remove);
                    m_listener_tracker.remove(m_addr, addr);
                }
            }
            break;
            case fop_range_ref:
            {
                abs_range_t range = p->get_range_ref().to_abs(m_addr);
                if (m_mode == mode_add)
                    cell_listener_tracker::get(m_context).add(m_addr, range);
                else
                {
                    assert(m_mode == mode_remove);
                    cell_listener_tracker::get(m_context).remove(m_addr, range);
                }
            }
            break;
            default:
                ; // ignore the rest.
        }
    }

private:
    model_context& m_context;
    cell_listener_tracker& m_listener_tracker;
    const abs_address_t& m_addr;
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
        __IXION_DEBUG_OUT__ << "  cell " << name << endl;
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
    explicit cell_dependency_handler(model_context& cxt, dependency_tracker& dep_tracker, dirty_cells_t& dirty_cells) :
        m_context(cxt), m_dep_tracker(dep_tracker), m_dirty_cells(dirty_cells) {}

    void operator() (base_cell* pcell)
    {
        if (pcell->get_celltype() != celltype_formula)
            return;

#if DEBUG_MODEL_PARSER
        __IXION_DEBUG_OUT__ << get_formula_result_output_separator() << endl;
        __IXION_DEBUG_OUT__ << "processing dependency of " << m_context.get_cell_name(pcell) << endl;
#endif
        // Register cell dependencies.
        formula_cell* fcell = static_cast<formula_cell*>(pcell);
        vector<formula_token_base*> ref_tokens;
        fcell->get_ref_tokens(m_context, ref_tokens);

#if DEBUG_MODEL_PARSER
        __IXION_DEBUG_OUT__ << "this cell contains " << ref_tokens.size() << " reference tokens." << endl;
#endif
        // Pick up the referenced cells from the ref tokens.  I should
        // probably combine this with the above get_ref_tokens() call above
        // for efficiency.
        vector<base_cell*> deps;
        abs_address_t cell_pos = m_context.get_cell_position(pcell);
        for_each(ref_tokens.begin(), ref_tokens.end(), ref_cell_picker(m_context, cell_pos, deps));

#if DEBUG_MODEL_PARSER
        __IXION_DEBUG_OUT__ << "number of precedent cells picked up: " << deps.size() << endl;
#endif
        // Register dependency information.  Only dirty cells should be
        // registered as precedent cells since non-dirty cells are equivalent
        // to constants.
        for_each(deps.begin(), deps.end(), depcell_inserter(m_dep_tracker, m_dirty_cells, fcell));
    }

private:
    model_context& m_context;
    dependency_tracker& m_dep_tracker;
    dirty_cells_t& m_dirty_cells;
};

typedef ::std::pair<abs_address_t, formula_cell*> address_cell_pair_type;

/**
 * Function object to convert each lexer token cell into formula token cell.
 * Converted cells are put into the context object during this process.
 * Note that each cell passed on to this function represents either a
 * brand-new cell, or an edited cell.
 */
class lexer_formula_cell_converter : public unary_function<model_parser::cell, void>
{
    model_context& m_context;
    vector<address_cell_pair_type>& m_fcells;
    bool m_first_static_content;
public:
    lexer_formula_cell_converter(model_context& cxt, vector<address_cell_pair_type>& fcells) :
        m_context(cxt), m_fcells(fcells), m_first_static_content(true) {}

    void operator() (const model_parser::cell& model_cell)
    {
        switch (model_cell.get_type())
        {
            case model_parser::ct_formula:
                convert_formula_cell(model_cell);
            break;
            case model_parser::ct_value:
                convert_numeric_cell(model_cell);
            break;
            case model_parser::ct_string:
                convert_string_cell(model_cell);
            break;
            default:
                throw general_error("unhandled lexer cell type.");
        }
    }

private:
    void remove_self_as_listener(formula_cell* fcell, const abs_address_t& addr)
    {
        // Go through all its existing references, and remove
        // itself as their listener.  This step is important
        // especially during partial re-calculation.
        vector<formula_token_base*> ref_tokens;
        fcell->get_ref_tokens(m_context, ref_tokens);
        for_each(ref_tokens.begin(), ref_tokens.end(),
                 formula_cell_listener_handler(m_context,
                     addr, formula_cell_listener_handler::mode_remove));
    }

    void convert_string_cell(const model_parser::cell& model_cell)
    {
        const mem_str_buf& name = model_cell.get_name();
        formula_name_type name_type = m_context.get_name_resolver().resolve(name.str(), abs_address_t());
        if (name_type.type != formula_name_type::cell_reference)
        {
            ostringstream os;
            os << "failed to convert " << name.str() << " to a string cell.  ";
            os << "Only a normal cell instance can be a string cell.";
            throw general_error(os.str());
        }

        abs_address_t addr;
        addr.sheet = name_type.address.sheet;
        addr.row = name_type.address.row;
        addr.column = name_type.address.col;

        // For a string cell, there should be only one lexer token which must
        // be a string token.
        const lexer_tokens_t& lexer_tokens = model_cell.get_tokens();
        if (lexer_tokens.size() != 1)
            throw general_error("there should only be one lexer token for a string cell.");

        const lexer_token_base& token = lexer_tokens.back();
        if (token.get_opcode() != op_string)
            throw general_error("string lexer token expected, but not found.");

        base_cell* p = m_context.get_cell(addr);
        if (p && p->get_celltype() == celltype_formula)
        {
            // Pre-existing formula cell.
            formula_cell* fcell = static_cast<formula_cell*>(p);
            remove_self_as_listener(fcell, addr);
        }

        mem_str_buf str = token.get_string();
        m_context.set_cell(addr, new string_cell(0));

        if (m_first_static_content)
        {
            cout << get_formula_result_output_separator() << endl;
            m_first_static_content = false;
        }
        const formula_name_resolver& resolver = m_context.get_name_resolver();
        cout << resolver.get_name(addr) << ": (s) " << str.str() << endl;
    }

    void convert_numeric_cell(const model_parser::cell& model_cell)
    {
        const mem_str_buf& name = model_cell.get_name();
        formula_name_type name_type = m_context.get_name_resolver().resolve(name.str(), abs_address_t());
        if (name_type.type != formula_name_type::cell_reference)
        {
            ostringstream os;
            os << "failed to convert " << name.str() << " to a numeric cell.  ";
            os << "Only a normal cell instance can be a numeric cell.";
            throw general_error(os.str());
        }

        // For a value cell, there should be no more than 2 lexer tokens: one
        // for the sign and one for the number.
        const lexer_tokens_t& lexer_tokens = model_cell.get_tokens();
        if (lexer_tokens.empty())
            throw general_error("no lexer tokens for a value cell.");
        size_t token_count = lexer_tokens.size();
        if (token_count > 2)
            throw general_error("there should be no more than 2 lexer tokens for a value cell.");

        const lexer_token_base& value_token = lexer_tokens.back();
        if (value_token.get_opcode() != op_value)
            throw general_error("value token expected, but not found.");

        double value = value_token.get_value();
        if (token_count == 2)
        {
            const lexer_token_base& sign_token = lexer_tokens.front();
            switch (sign_token.get_opcode())
            {
                case op_plus:
                    // do nothing.
                break;
                case op_minus:
                    value *= -1.0;
                break;
                default:
                    throw general_error("unexpected first token type.");
            }
        }

        abs_address_t addr;
        addr.sheet = name_type.address.sheet;
        addr.row = name_type.address.row;
        addr.column = name_type.address.col;
#if DEBUG_MODEL_PARSER
        __IXION_DEBUG_OUT__ << addr.get_name() << " : value = " << value << endl;
#endif
        base_cell* p = m_context.get_cell(addr);
        if (p && p->get_celltype() == celltype_formula)
        {
            // Pre-existing formula cell.
            formula_cell* fcell = static_cast<formula_cell*>(p);
            remove_self_as_listener(fcell, addr);
        }

        m_context.set_cell(addr, new numeric_cell(value));

        if (m_first_static_content)
        {
            cout << get_formula_result_output_separator() << endl;
            m_first_static_content = false;
        }
        const formula_name_resolver& resolver = m_context.get_name_resolver();
        cout << resolver.get_name(addr) << ": (n) " << value << endl;
    }

    void convert_formula_cell(const model_parser::cell& model_cell)
    {
        const string& name = model_cell.get_name().str();

#if DEBUG_MODEL_PARSER
        __IXION_DEBUG_OUT__ << "parsing cell " << name << " (initial content:" << model_cell.print() << ")" << endl;
#endif
        formula_parser fparser(model_cell.get_tokens(), m_context);

        formula_cell* fcell = NULL;
        formula_name_type name_type = m_context.get_name_resolver().resolve(name, abs_address_t());
        switch (name_type.type)
        {
            case formula_name_type::named_expression:
                fcell = m_context.get_named_expression(name);
                if (!fcell)
                {
                    // No prior named expression with the same name.
                    auto_ptr<formula_cell> pcell(new formula_cell);
                    m_context.set_named_expression(name, pcell);
                    fcell = m_context.get_named_expression(name);
                }
                if (!fcell)
                    throw general_error("failed to insert a named expression");
            break;
            case formula_name_type::cell_reference:
            {
                abs_address_t addr;
                addr.sheet = name_type.address.sheet;
                addr.row = name_type.address.row;
                addr.column = name_type.address.col;
#if DEBUG_MODEL_PARSER
                __IXION_DEBUG_OUT__ << addr.get_name() << endl;
#endif
                base_cell* p = m_context.get_cell(addr);
                if (!p)
                {
                    // No prior cell at this address. Create a new one.
                    m_context.set_cell(addr, new formula_cell);
                    fcell = static_cast<formula_cell*>(m_context.get_cell(addr));
                }
                else if (p->get_celltype() != celltype_formula)
                {
                    // Prior cell exists, but it's not a formula cell.
                    m_context.set_cell(addr, new formula_cell);
                    fcell = static_cast<formula_cell*>(m_context.get_cell(addr));
                }
                else
                {
                    // Pre-existing formula cell.
                    fcell = static_cast<formula_cell*>(p);
                    remove_self_as_listener(fcell, addr);
                }

                if (!fcell)
                    throw general_error("failed to insert a new formula cell");

                m_fcells.push_back(
                    address_cell_pair_type(addr, fcell));

                fparser.set_origin(addr);
            }
            break;
            default:
                throw general_error("unknown name type.");
        }

        assert(fcell);

        // Parse the lexer tokens and turn them into formula tokens.
        fparser.parse();
        fparser.print_tokens();

        // Associate the formula tokens with the formula cell instance.
        formula_tokens_t* p = new formula_tokens_t;
        p->swap(fparser.get_tokens());
        fcell->set_identifier(m_context.add_formula_tokens(p));
    }
};

/**
 * Function object that goes through the references of each formula cell and
 * registers the formula cell as a listener to all its referenced cells.
 */
class formula_cell_ref_to_listener_handler : public unary_function<address_cell_pair_type, void>
{
    model_context& m_context;
public:
    formula_cell_ref_to_listener_handler(model_context& cxt) :
        m_context(cxt) {}

    void operator() (const address_cell_pair_type& v)
    {
        formula_cell* fcell = v.second;
#if DEBUG_MODEL_PARSER
        __IXION_DEBUG_OUT__ << "processing formula cell at " << m_context.get_cell_name(fcell) << endl;
#endif
        // Now, register the formula cell as a listener to all its references.
        vector<formula_token_base*> ref_tokens;
        fcell->get_ref_tokens(m_context, ref_tokens);
#if DEBUG_MODEL_PARSER
        __IXION_DEBUG_OUT__ << "  number of reference tokens: " << ref_tokens.size() << endl;
#endif
        for_each(ref_tokens.begin(), ref_tokens.end(),
                 formula_cell_listener_handler(m_context,
                     v.first, formula_cell_listener_handler::mode_add));
    }
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

bool is_separator(char c)
{
    return c == '=' || c == ':' || c == '@';
}

void parse_init(const char*& p, parse_data& data)
{
    model_parser::cell_type content_type = model_parser::ct_unknown;
    mem_str_buf buf, name, formula;
    for (; *p != '\n'; ++p)
    {
        if (name.empty() && is_separator(*p))
        {
            // Separator encountered.  Set the name and clear the buffer.
            if (buf.empty())
                throw model_parser::parse_error("left hand side is empty");

            name = buf;
            buf.clear();
            data.cell_names.push_back(name.str());
            switch (*p)
            {
                case '=':
                    content_type = model_parser::ct_formula;
                break;
                case ':':
                    content_type = model_parser::ct_value;
                break;
                case '@':
                    content_type = model_parser::ct_string;
                break;
                default:
                    ;
            }
        }
        else
        {
            if (buf.empty())
                buf.set_start(p);
            else
                buf.inc();
        }
    }

#if DEBUG_MODEL_PARSER
    __IXION_DEBUG_OUT__ << "name: " << name.str() << "  buf: " << buf.str() << endl;
#endif

    if (!buf.empty())
    {
        if (name.empty())
            throw model_parser::parse_error("separator is missing");

        formula = buf;

        // tokenize the formula string, and create a formula cell
        // with the tokens.
        formula_lexer lexer(formula.get(), formula.size());
        lexer.tokenize();
        lexer_tokens_t tokens;
        lexer.swap_tokens(tokens);

#if DEBUG_MODEL_PARSER
        // test-print tokens.
        __IXION_DEBUG_OUT__ << "tokens from lexer: " << print_tokens(tokens, true) << endl;
#endif

        model_parser::cell fcell(name, content_type, tokens);
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
    if (*p == '%')
    {
        // This line is a comment.  Skip the rest of the line.
        _com.swap(com);
        while (*p != '\n') ++p;
        return;
    }
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

/**
 * This is where primitive lexer tokens get converted to formula tokens, and
 * any dependency tracking information gets built.
 *
 * @param cells array of cells containing lexer tokens.  <i>This array only
 *              contains either new cells or edited cells.</i>
 * @param context context representing current session.
 * @param dirty_cells cells that need to be re-calculated.
 */
void convert_lexer_tokens(const vector<model_parser::cell>& cells, model_context& context, dirty_cells_t& dirty_cells)
{
#if DEBUG_MODEL_PARSER
    __IXION_DEBUG_OUT__ << "number of lexer token cells: " << cells.size() << endl;
#endif
    vector<address_cell_pair_type> fcells;

    // First, convert each lexer token cell into formula token cell, and put
    // the formula cells into the context object.
    for_each(cells.begin(), cells.end(),
             lexer_formula_cell_converter(context, fcells));

#if DEBUG_MODEL_PARSER
    __IXION_DEBUG_OUT__ << "number of converted formula cells: " << fcells.size() << endl;
#endif

    // Next, go through the formula cells and their references, and register
    // the formula cells as listeners to their respective references.
    for_each(fcells.begin(), fcells.end(),
             formula_cell_ref_to_listener_handler(context));

    // Go through all the listeners and determine dirty cells - cells to be
    // (re-)calculated.
    dirty_cells_t _dirty_cells;
    {
        // single, cell-to-cell listeners.
        vector<address_cell_pair_type>::iterator itr = fcells.begin(), itr_end = fcells.end();
        for (; itr != itr_end; ++itr)
        {
#if DEBUG_MODEL_PARSER
            __IXION_DEBUG_OUT__ << "processing " << context.get_name_resolver().get_name(itr->first, abs_address_t()) << endl;
#endif
            formula_cell* fcell = itr->second;
            _dirty_cells.insert(fcell);
            cell_listener_tracker& tracker = cell_listener_tracker::get(context);
            tracker.get_all_range_listeners(itr->first, _dirty_cells);
            tracker.get_all_cell_listeners(itr->first, _dirty_cells);
        }
    }

#if DEBUG_MODEL_PARSER
    __IXION_DEBUG_OUT__ << get_formula_result_output_separator() << endl;
    vector<address_cell_pair_type>::const_iterator itr = fcells.begin(), itr_end = fcells.end();
    for (; itr != itr_end; ++itr)
    {
        const abs_address_t& target = itr->first;
        cell_listener_tracker::get(context).print_cell_listeners(target);
    }

    __IXION_DEBUG_OUT__ << get_formula_result_output_separator() << endl;
    __IXION_DEBUG_OUT__ << "All dirty cells: " << endl;
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

model_parser::cell::cell(const mem_str_buf& name, cell_type type, lexer_tokens_t& tokens) :
    m_name(name), m_type(type)
{
    // Note that this will empty the passed token container !
    m_tokens.swap(tokens);
}

model_parser::cell::cell(const model_parser::cell& r) :
    m_name(r.m_name),
    m_type(r.m_type),
    m_tokens(r.m_tokens) {}

model_parser::cell::~cell() {}

string model_parser::cell::print() const
{
    return print_tokens(m_tokens, false);
}

const mem_str_buf& model_parser::cell::get_name() const
{
    return m_name;
}

model_parser::cell_type model_parser::cell::get_type() const
{
    return m_type;
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
        // In each iteration, the p always points to the 1st character of a
        // line.
        if (*p == '%')
        {
            // This line contains a command.
            mem_str_buf buf_com;
            parse_command(p, buf_com);
            if (buf_com.equals("%"))
            {
                // This is a comment line.  Just ignore it.
            }
            else if (buf_com.equals("calc"))
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
            else if (buf_com.equals("exit"))
            {
                // Exit the loop.
                return;
            }
            else if (buf_com.equals("mode init"))
            {
                parse_mode = parse_mode_init;
            }
            else if (buf_com.equals("mode result"))
            {
                // Clear any previous result values.
                data.formula_results.clear();
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

        switch (pcell->get_celltype())
        {
            case celltype_formula:
            {
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
            break;
            case celltype_numeric:
            {
                if (pcell->get_value() != res.get_value())
                {
                    ostringstream os;
                    os << "unexpected numeric result: (expected: " << res.get_value() << "; actual: " << pcell->get_value() << ")";
                    throw check_error(os.str());
                }
            }
            break;
            default:
                throw check_error("unhandled cell type.");
        }
    }
}

const base_cell* model_parser::get_cell_from_name(const string& name)
{
    const formula_name_resolver& resolver = m_context.get_name_resolver();
    formula_name_type name_type = resolver.resolve(name, abs_address_t());
    switch (name_type.type)
    {
        case formula_name_type::cell_reference:
        {
            const formula_name_type::address_type& addr = name_type.address;
            return m_context.get_cell(abs_address_t(addr.sheet, addr.row, addr.col));
        }
        case formula_name_type::named_expression:
            return m_context.get_named_expression(name);
        default:
            ;
    }
    return NULL;
}

}

