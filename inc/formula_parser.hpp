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

#include <boost/noncopyable.hpp>

#include "global.hpp"
#include "tokens.hpp"
#include "formula_tokens.hpp"

namespace ixion {

/** 
 * Class formula_parser parses a series of primitive (or lexer) tokens 
 * passed on from the lexer, and turn them into a series of formula tokens. 
 * It also picks up a list of names that the cell depends on. 
 */
class formula_parser : public ::boost::noncopyable
{
public:
    formula_parser(const lexer_tokens_t& tokens);
    ~formula_parser();

    void parse();

    const formula_tokens_t& get_tokens() const;
    

private:
    formula_parser(); // disabled

    lexer_tokens_t   m_tokens;
    formula_tokens_t m_formula_tokens;
};

}

#endif
