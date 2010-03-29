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

#include "formula_parser.hpp"

#include <iostream>

using namespace std;

namespace ixion {

namespace {

class ref_error : public general_error
{
public:
    ref_error(const string& msg) :
        general_error(msg) {}
};

}

formula_parser::formula_parser(const lexer_tokens_t& tokens, const cell_name_map_t* p_cell_names) :
    m_tokens(tokens),
    mp_cell_names(p_cell_names)
{
}

formula_parser::~formula_parser()
{
}

void formula_parser::parse()
{
    try
    {
        size_t n = m_tokens.size();
        for (size_t i = 0; i < n; ++i)
        {
            const token_base& t = m_tokens[i];
            switch (t.get_opcode())
            {
                case op_close:
                    break;
                case op_divide:
                    break;
                case op_minus:
                    break;
                case op_multiply:
                    break;
                case op_name:
                    name(t);
                    break;
                case op_open:
                    break;
                case op_plus:
                    break;
                case op_sep:
                    break;
                case op_string:
                    break;
                case op_value:
                    break;
                default:
                    ;
            }
        }
    }
    catch (const ref_error& e)
    {

    }
}

formula_tokens_t& formula_parser::get_tokens()
{
    return m_formula_tokens;
}

const vector<const base_cell*>& formula_parser::get_depend_cells() const
{
    return m_depend_cells;
}

void formula_parser::name(const token_base& t)
{
    const string name = t.get_string();
    cell_name_map_t::const_iterator itr = mp_cell_names->find(name);
    if (itr == mp_cell_names->end())
    {
        // referenced name not found.
        throw ref_error(name + " not found");
    }

    cout << "  name = " << name << "  pointer to the cell instance = " << itr->second << endl;
    m_formula_tokens.push_back(new single_ref_token(itr->second));
    m_depend_cells.push_back(itr->second);
}

}
