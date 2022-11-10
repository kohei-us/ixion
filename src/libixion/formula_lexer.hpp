/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_FORMULA_LEXER_HPP
#define INCLUDED_IXION_FORMULA_LEXER_HPP

#include <string>
#include <exception>

#include "ixion/exceptions.hpp"
#include "ixion/config.hpp"

#include "lexer_tokens.hpp"

namespace ixion {

class formula_lexer 
{
    formula_lexer() = delete;
    formula_lexer(const formula_lexer&) = delete;
    formula_lexer& operator= (const formula_lexer&) = delete;
public:
    class tokenize_error : public general_error
    {
    public:
        tokenize_error(const std::string& msg);
    };

    formula_lexer(const config& config, const char* p, size_t n);
    ~formula_lexer();

    void tokenize();

    /**
     * Note that this will empty the tokens stored inside the lexer instance.
     *
     * @param tokens token container to move the tokens to.
     */
    void swap_tokens(lexer_tokens_t& tokens);

private:
    const config& m_config;
    lexer_tokens_t m_tokens;
    const char* mp_first;
    size_t m_size;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
