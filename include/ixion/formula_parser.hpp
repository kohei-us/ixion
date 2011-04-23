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

#ifndef __IXION_FORMULA_PARSER_HPP__
#define __IXION_FORMULA_PARSER_HPP__

#include "ixion/global.hpp"
#include "ixion/lexer_tokens.hpp"
#include "ixion/formula_tokens.hpp"
#include "ixion/hash_container/map.hpp"

#include <string>
#include <boost/noncopyable.hpp>

namespace ixion {

class model_context;

/** 
 * Class formula_parser parses a series of primitive (or lexer) tokens 
 * passed on from the lexer, and turn them into a series of formula tokens. 
 * It also picks up a list of cells that it depends on.
 */
class formula_parser : public ::boost::noncopyable
{
public:
    class parse_error : public general_error
    {
    public:
        parse_error(const ::std::string& msg);
    };

    formula_parser(const lexer_tokens_t& tokens, const model_context& cxt);
    ~formula_parser();

    void set_origin(const address_t& pos);
    void parse();
    void print_tokens() const;

    formula_tokens_t& get_tokens();
    
private:
    formula_parser(); // disabled

    void primitive(lexer_opcode_t oc);
    void name(const lexer_token_base& t);
    void value(const lexer_token_base& t);

    const lexer_tokens_t&   m_tokens; // lexer tokens of this expression
    const model_context&    m_context;
    formula_tokens_t        m_formula_tokens;
    address_t               m_pos;    // reference position (usually current cell). always absolute.
};

}

#endif
