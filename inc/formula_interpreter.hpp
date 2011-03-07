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

#ifndef __IXION_FORMULA_INTERPRETER_HPP__
#define __IXION_FORMULA_INTERPRETER_HPP__

#include "global.hpp"
#include "formula_tokens.hpp"
#include "hash_container/set.hpp"

#include <sstream>

#include <boost/noncopyable.hpp>

namespace ixion {

class formula_cell;
class model_context;

class formula_interpreter : public ::boost::noncopyable
{
    typedef _ixion_unordered_set_type< ::std::string> name_set;
public:
    typedef ::std::vector<const formula_token_base*> local_tokens_type;

    formula_interpreter(const formula_cell* cell, const model_context& cxt);
    ~formula_interpreter();

    bool interpret();
    double get_result() const;
    formula_error_t get_error() const;

private:
    /**
     * Expand all named expressions into a flat set of tokens.  This is also 
     * where we detect circular referencing of named expressions. 
     */
    void init_tokens();

    void expand_named_expression(
        const ::std::string& expr_name, const formula_cell* expr, name_set& used_names);

    bool has_token() const;
    void next();
    const formula_token_base& token() const;
    const formula_token_base& next_token();

    // The following methods are handlers.  In each handler, the initial
    // position is always set to the first unprocessed token.  Each handler is
    // responsible for setting the token position to the next unprocessed
    // position when it finishes.

    double expression();
    double named_expression();
    double term();
    double factor();
    double paren();
    double variable();
    double constant();
    double function();

private:
    const formula_cell*         m_parent_cell;
    const formula_tokens_t&     m_original_tokens;
    const model_context&        m_context;

    local_tokens_type m_tokens;
    local_tokens_type::const_iterator m_cur_token_itr;
    local_tokens_type::const_iterator m_end_token_pos;

    ::std::ostringstream        m_outbuf; // output buffer (for debug purposes)

    double m_result;
    formula_error_t m_error;
};

}

#endif
