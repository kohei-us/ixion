/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_FORMULA_PARSER_HPP__
#define __IXION_FORMULA_PARSER_HPP__

#include "ixion/exceptions.hpp"
#include "ixion/lexer_tokens.hpp"
#include "ixion/formula_tokens.hpp"

#include <string>
#include <boost/noncopyable.hpp>

namespace ixion {

namespace iface { class formula_model_access; }

class formula_name_resolver;

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

    formula_parser(const lexer_tokens_t& tokens, iface::formula_model_access& cxt, const formula_name_resolver& resolver);
    ~formula_parser();

    void set_origin(const abs_address_t& pos);
    void parse();
    void print_tokens() const;

    formula_tokens_t& get_tokens();

private:
    formula_parser(); // disabled

    void primitive(lexer_opcode_t oc);
    void name(const lexer_token_base& t);
    void literal(const lexer_token_base& t);
    void value(const lexer_token_base& t);
    void less(const lexer_token_base& t);
    void greater(const lexer_token_base& t);

    const lexer_token_base& get_token() const;
    bool has_token() const;
    bool has_next() const;
    void next();
    void prev();

private:
    lexer_tokens_t::const_iterator m_itr_cur;
    lexer_tokens_t::const_iterator m_itr_end;

    const lexer_tokens_t&   m_tokens; // lexer tokens of this expression
    iface::formula_model_access&   m_context;
    formula_tokens_t        m_formula_tokens;
    abs_address_t           m_pos;    // reference position (usually current cell). always absolute.

    const formula_name_resolver& m_resolver;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
