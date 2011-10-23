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

#include "model_parser.hpp"

#include "ixion/cell.hpp"
#include "ixion/formula.hpp"
#include "ixion/formula_lexer.hpp"
#include "ixion/formula_parser.hpp"
#include "ixion/depends_tracker.hpp"
#include "ixion/formula_interpreter.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/cell_listener_tracker.hpp"
#include "ixion/function_objects.hpp"

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
        vector<const formula_token_base*> ref_tokens;
        fcell->get_ref_tokens(m_context, ref_tokens);
        for_each(ref_tokens.begin(), ref_tokens.end(),
                 formula_cell_listener_handler(m_context,
                     addr, formula_cell_listener_handler::mode_remove));
    }

    void convert_string_cell(const model_parser::cell& model_cell)
    {
        const mem_str_buf& name = model_cell.get_name();
        formula_name_type name_type = m_context.get_name_resolver().resolve(
            name.get(), name.size(), abs_address_t());

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
        size_t str_id = m_context.add_string(str.get(), str.size());
        m_context.set_cell(addr, new string_cell(str_id));

        if (m_first_static_content)
        {
            cout << get_formula_result_output_separator() << endl;
            m_first_static_content = false;
        }
        const formula_name_resolver& resolver = m_context.get_name_resolver();
        cout << resolver.get_name(addr, false) << ": (s) " << str.str() << endl;
    }

    void convert_numeric_cell(const model_parser::cell& model_cell)
    {
        const mem_str_buf& name = model_cell.get_name();
        formula_name_type name_type = m_context.get_name_resolver().resolve(
            name.get(), name.size(), abs_address_t());

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
        cout << resolver.get_name(addr, false) << ": (n) " << value << endl;
    }

    void convert_formula_cell(const model_parser::cell& model_cell)
    {
        const mem_str_buf& name = model_cell.get_name();

#if DEBUG_MODEL_PARSER
        __IXION_DEBUG_OUT__ << "parsing cell " << name.str() << " (initial content:" << model_cell.print() << ")" << endl;
#endif
        formula_parser fparser(model_cell.get_tokens(), m_context);

        formula_cell* fcell = NULL;
        abs_address_t addr;
        addr.sheet = global_scope; // global scope for named expressions.

        formula_name_type name_type = m_context.get_name_resolver().resolve(
            name.get(), name.size(), abs_address_t());
        switch (name_type.type)
        {
            case formula_name_type::named_expression:
                fcell = m_context.get_named_expression(name.str());
                if (!fcell)
                {
                    // No prior named expression with the same name.
                    m_context.set_named_expression(name.get(), name.size(), new formula_cell);
                    fcell = m_context.get_named_expression(name.str());
                }
                if (!fcell)
                    throw general_error("failed to insert a named expression");
            break;
            case formula_name_type::cell_reference:
            {
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
        if (!set_shared_formula_tokens_to_cell(addr, fcell, p))
            fcell->set_identifier(m_context.add_formula_tokens(addr.sheet, p));
    }

    bool set_shared_formula_tokens_to_cell(const abs_address_t& addr, formula_cell* fcell, formula_tokens_t* new_tokens)
    {
        assert(new_tokens);
        if (addr.sheet == global_scope)
            return false;

        // Check its neighbors for adjacent formula cells.
        if (addr.row == 0)
            return false;

        abs_address_t test = addr;
        test.row -= 1;
        base_cell* p = m_context.get_cell(test);
        if (!p || p->get_celltype() != celltype_formula)
            // The neighboring cell is not a formula cell.
            return false;

        formula_cell* test_cell = static_cast<formula_cell*>(p);

        if (test_cell->is_shared())
        {
            size_t token_id = test_cell->get_identifier();
            const formula_tokens_t* tokens = m_context.get_shared_formula_tokens(addr.sheet, token_id);
            assert(tokens);

            if (*new_tokens != *tokens)
                return false;

            // Make sure that we can extend the shared range properly.
            abs_range_t range = m_context.get_shared_formula_range(addr.sheet, token_id);
            if (range.first.sheet != addr.sheet)
                // Wrong sheet
                return false;

            if (range.first.column != range.last.column)
                // Must be a single column.
                return false;

            if (range.last.row != (addr.row - 1))
                // Last row is not immediately above the current cell.
                return false;

            fcell->set_identifier(token_id);
            fcell->set_shared(true);

            range.last.row += 1;
            m_context.set_shared_formula_range(addr.sheet, token_id, range);
        }
        else
        {
            size_t token_id = test_cell->get_identifier();
            const formula_tokens_t* tokens = m_context.get_formula_tokens(addr.sheet, token_id);
            assert(tokens);

            if (*new_tokens != *tokens)
                return false;

            // Move the tokens of the master cell to the shared token storage.
            size_t shared_id = m_context.set_formula_tokens_shared(addr.sheet, token_id);
            test_cell->set_shared(true);
            test_cell->set_identifier(shared_id);
            assert(test_cell->is_shared());
            fcell->set_identifier(shared_id);
            fcell->set_shared(true);
            assert(fcell->is_shared());
            abs_range_t range;
            range.first = addr;
            range.last = addr;
            range.first.row -= 1;
            m_context.set_shared_formula_range(addr.sheet, shared_id, range);
        }
        return true;
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
        vector<const formula_token_base*> ref_tokens;
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
};

bool is_separator(char c)
{
    return c == '=' || c == ':' || c == '@';
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
            __IXION_DEBUG_OUT__ << "processing " << context.get_name_resolver().get_name(itr->first, abs_address_t(), false) << endl;
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
    m_filepath(filepath), m_thread_count(thread_count), m_print_separator(true) {}

model_parser::~model_parser() {}

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

                // Perform full calculation on currently stored cells.
                calculate_cells(m_context, m_dirty_cells, m_thread_count);
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
                check();
            }
            else if (buf_com.equals("exit"))
            {
                // Exit the loop.
                return;
            }
            else if (buf_com.equals("mode init"))
            {
                parse_mode = parse_mode_init;
                m_print_separator = true;
            }
            else if (buf_com.equals("mode result"))
            {
                // Clear any previous result values.
                m_formula_results.clear();
                parse_mode = parse_mode_result;
            }
            else if (buf_com.equals("mode edit"))
            {
                parse_mode = parse_mode_edit;
                data.cells.clear();
                m_dirty_cells.clear();
                m_print_separator = true;
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
                parse_init(p);
            break;
            case parse_mode_edit:
                parse_init(p);
            break;
            case parse_mode_result:
                parse_result(p);
            break;
            default:
                throw parse_error("unknown parse mode");
        }
    }
}

namespace {

void remove_self_as_listener(
    model_context& cxt, formula_cell* fcell, const abs_address_t& addr)
{
    // Go through all its existing references, and remove
    // itself as their listener.  This step is important
    // especially during partial re-calculation.
    vector<const formula_token_base*> ref_tokens;
    fcell->get_ref_tokens(cxt, ref_tokens);
    for_each(ref_tokens.begin(), ref_tokens.end(),
             formula_cell_listener_handler(cxt,
                 addr, formula_cell_listener_handler::mode_remove));
}

}

void model_parser::parse_init(const char*& p)
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

        const formula_name_resolver& resolver = m_context.get_name_resolver();
        formula_name_type ret = resolver.resolve(name.get(), name.size(), abs_address_t());
        if (ret.type != formula_name_type::cell_reference)
        {
            ostringstream os;
            os << "invalid cell name: " << name.str();
            throw model_parser::parse_error(os.str());
        }

        abs_address_t pos(ret.address.sheet, ret.address.row, ret.address.col);

        base_cell* p = m_context.get_cell(pos);
        if (p && p->get_celltype() == celltype_formula)
        {
            // Pre-existing formula cell.
            formula_cell* fcell = static_cast<formula_cell*>(p);
            remove_self_as_listener(m_context, fcell, pos);
        }

        switch (content_type)
        {
            case ct_formula:
            {
#if DEBUG_MODEL_PARSER
                __IXION_DEBUG_OUT__ << "pos: " << resolver.get_name(pos, false) << " type: formula" << endl;
#endif
                formula_tokens_t* tokens = new formula_tokens_t;
                parse_formula_string(m_context, pos, buf.get(), buf.size(), *tokens);

                size_t tkid = m_context.add_formula_tokens(0, tokens);
                formula_cell* fcell = new formula_cell(tkid);
                m_context.set_cell(pos, fcell);
                m_dirty_cells.insert(fcell);
                register_formula_cell(m_context, pos, fcell);
#if DEBUG_MODEL_PARSER
                std::string s;
                print_formula_tokens(m_context, pos, *tokens, s);
                __IXION_DEBUG_OUT__ << "formula tokens: " << s << endl;
#endif
            }
            break;
            case ct_string:
            {
#if DEBUG_MODEL_PARSER
                __IXION_DEBUG_OUT__ << "pos: " << resolver.get_name(pos, false) << " type: string" << endl;
#endif
                // For now, we compile the raw string using lexer.  It should
                // only generate one lexer token that represents the string
                // value.  TODO: parse raw string without the lexer.
                formula_lexer lexer(buf.get(), buf.size());
                lexer.tokenize();
                lexer_tokens_t tokens;
                lexer.swap_tokens(tokens);

                if (tokens.size() != 1)
                    throw general_error("there should only be one lexer token for a string cell.");

                const lexer_token_base& token = tokens.back();
                if (token.get_opcode() != op_string)
                    throw general_error("string lexer token expected, but not found.");

                mem_str_buf str = token.get_string();
                size_t str_id = m_context.add_string(str.get(), str.size());
                m_context.set_cell(pos, new string_cell(str_id));
                if (m_print_separator)
                {
                    m_print_separator = false;
                    cout << get_formula_result_output_separator() << endl;
                }
                cout << name.str() << ": (s) " << str.str() << endl;
            }
            break;
            case ct_value:
            {
#if DEBUG_MODEL_PARSER
                __IXION_DEBUG_OUT__ << "pos: " << resolver.get_name(pos, false) << " type: numeric" << endl;
#endif
                // TODO: Parse the number without the lexer.
                formula_lexer lexer(buf.get(), buf.size());
                lexer.tokenize();
                lexer_tokens_t tokens;
                lexer.swap_tokens(tokens);

                // For a numeric cell, there should be no more than 2 lexer
                // tokens: one for the sign and one for the number.
                if (tokens.empty())
                    throw general_error("no lexer tokens for a value cell.");

                size_t token_count = tokens.size();
                if (token_count > 2)
                    throw general_error("there should be no more than 2 lexer tokens for a value cell.");

                const lexer_token_base& value_token = tokens.back();
                if (value_token.get_opcode() != op_value)
                    throw general_error("value token expected, but not found.");

                double value = value_token.get_value();
                if (token_count == 2)
                {
                    const lexer_token_base& sign_token = tokens.front();
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

                m_context.set_cell(pos, new numeric_cell(value));

                if (m_print_separator)
                {
                    m_print_separator = false;
                    cout << get_formula_result_output_separator() << endl;
                }
                cout << resolver.get_name(pos, false) << ": (n) " << value << endl;
            }
            break;
            default:
                throw model_parser::parse_error("unknown content type");
        }
    }
}

void model_parser::parse_result(const char*& p)
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
    res.parse(result.get(), result.size());
    model_parser::results_type::iterator itr = m_formula_results.find(name_s);
    if (itr == m_formula_results.end())
    {
        // This cell doesn't exist yet.
        pair<model_parser::results_type::iterator, bool> r =
            m_formula_results.insert(model_parser::results_type::value_type(name_s, res));
        if (!r.second)
            throw model_parser::parse_error("failed to insert a new result.");
    }
    else
        itr->second = res;
}

void model_parser::calc(dirty_cells_t& cells)
{
    calculate_cells(m_context, cells, m_thread_count);
}

void model_parser::check()
{
    cout << get_formula_result_output_separator() << endl
         << "checking results" << endl
         << get_formula_result_output_separator() << endl;

    results_type::const_iterator itr = m_formula_results.begin(), itr_end = m_formula_results.end();
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
            case celltype_string:
            {
                size_t str_id = pcell->get_identifier();
                const string* ps = m_context.get_string(str_id);
                if (!ps)
                    throw check_error("failed to retrieve a string value for a string cell.");

                if (*ps != res.get_string())
                {
                    ostringstream os;
                    os << "unexpected string result: (expected: " << res.get_string() << "; actual: " << *ps << ")";
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
    if (name.empty())
        return NULL;

    const formula_name_resolver& resolver = m_context.get_name_resolver();
    formula_name_type name_type = resolver.resolve(&name[0], name.size(), abs_address_t());
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

