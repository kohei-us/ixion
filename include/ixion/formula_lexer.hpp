/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_FORMULA_LEXER_HPP__
#define __IXION_FORMULA_LEXER_HPP__

#include <string>
#include <exception>
#include <boost/noncopyable.hpp>

#include "ixion/lexer_tokens.hpp"
#include "ixion/mem_str_buf.hpp"

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

    formula_lexer(const char* p, size_t n);
    ~formula_lexer();

    void tokenize();

    /**
     * Note that this will empty the tokens stored inside the lexer instance.
     *
     * @param tokens token container to move the tokens to.
     */
    void swap_tokens(lexer_tokens_t& tokens);

private:
    formula_lexer();

    lexer_tokens_t m_tokens;
    const char* mp_first;
    size_t m_size;
    mem_str_buf m_formula;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
