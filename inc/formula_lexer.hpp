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

#ifndef __FORMULA_LEXER_HPP__
#define __FORMULA_LEXER_HPP__

#include <string>
#include <exception>
#include <boost/noncopyable.hpp>

#include "global.hpp"

namespace ixion {

class formula_lexer : public ::boost::noncopyable
{
public:
    class tokenize_error : public ::std::exception
    {
    public:
        tokenize_error(const ::std::string& msg);
        virtual ~tokenize_error() throw();
        virtual const char* what() const throw();
    private:
        ::std::string m_msg;
    };

    formula_lexer(const ::std::string& formula);
    ~formula_lexer();

    void tokenize();

    /** 
     * Note that this will empty the tokens stored inside the lexer instance.
     *
     * @param tokens token container to move the tokens to.
     */
    void swap_tokens(tokens_t& tokens);

private:
    formula_lexer();

    tokens_t m_tokens;
    ::std::string m_formula;
};

}

#endif
