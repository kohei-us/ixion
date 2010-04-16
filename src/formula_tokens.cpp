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

#include "formula_tokens.hpp"

namespace ixion {

const char* print_tokens(const formula_tokens_t& tokens, bool verbose)
{
    return "";
}

// ============================================================================

formula_token_base::formula_token_base(fopcode_t op) :
    m_opcode(op)
{
}

formula_token_base::formula_token_base(const formula_token_base& r) :
    m_opcode(r.m_opcode)
{
}

formula_token_base::~formula_token_base()
{
}

fopcode_t formula_token_base::get_opcode() const
{
    return m_opcode;
}

const base_cell* formula_token_base::get_single_ref() const
{
    return NULL;
}

// ============================================================================

opcode_token::opcode_token(fopcode_t oc) :
    formula_token_base(oc)
{
}

opcode_token::~opcode_token()
{
}

// ============================================================================

single_ref_token::single_ref_token(const base_cell* pcell) :
    formula_token_base(fop_single_ref),
    mp_cell(pcell)
{
}

single_ref_token::single_ref_token(const single_ref_token& r) :
    formula_token_base(r),
    mp_cell(r.mp_cell)
{
}

single_ref_token::~single_ref_token()
{
}

const base_cell* single_ref_token::get_single_ref() const
{
    return mp_cell;
}

}
