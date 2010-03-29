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

#ifndef __FORMULA_PARSER_HPP__
#define __FORMULA_PARSER_HPP__

#include "global.hpp"
#include "tokens.hpp"
#include "formula_tokens.hpp"

#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include <string>

namespace ixion {

class base_cell;

typedef ::boost::ptr_map< ::std::string, base_cell> cell_name_map_t;

/** 
 * Class formula_parser parses a series of primitive (or lexer) tokens 
 * passed on from the lexer, and turn them into a series of formula tokens. 
 * It also picks up a list of names that the cell depends on. 
 */
class formula_parser : public ::boost::noncopyable
{
    typedef ::std::vector<base_cell*> depends_cell_array_type;

public:
    formula_parser(const lexer_tokens_t& tokens, const cell_name_map_t* p_cell_names);
    ~formula_parser();

    void parse();

    const formula_tokens_t& get_tokens() const;
    formula_tokens_t& get_tokens();
    
private:
    formula_parser(); // disabled

    void name(const token_base& t);

    const lexer_tokens_t    m_tokens;
    formula_tokens_t        m_formula_tokens;
    depends_cell_array_type m_depend_cells;

    const cell_name_map_t* mp_cell_names;
};

}

#endif
