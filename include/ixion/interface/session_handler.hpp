/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "../formula_opcode.hpp"
#include "../formula_function_opcode.hpp"
#include "../env.hpp"

#include <cstdlib>
#include <string_view>

namespace ixion {

class formula_cell;
class formula_result;
struct address_t;
struct range_t;
struct abs_address_t;
struct table_ref_t;
enum class formula_error_t : std::uint8_t;

namespace iface {

/**
 * Interface for receiving the events of one formula cell interpretation,
 * for tracing or debugging.  The interpreter creates one handler per cell
 * through the factory set with model_context::set_session_handler_factory(),
 * calls begin_cell_interpret() first, then the push methods for each token
 * as it gets processed, then one of set_result(), set_invalid_expression()
 * or set_formula_error(), and end_cell_interpret() last.
 */
class IXION_DLLPUBLIC session_handler
{
public:
    virtual ~session_handler();

    /**
     * Called when the interpretation of a formula cell begins.
     *
     * @param pos Position of the formula cell.
     */
    virtual void begin_cell_interpret(const abs_address_t& pos) = 0;

    /** Called when the interpretation of the formula cell ends. */
    virtual void end_cell_interpret() = 0;

    /**
     * Called when the interpretation has produced a result.
     *
     * @param result Result of the formula cell.
     */
    virtual void set_result(const formula_result& result) = 0;

    /**
     * Called when the interpretation has failed because the formula
     * expression is not valid.
     *
     * @param msg Message describing the problem.
     */
    virtual void set_invalid_expression(std::string_view msg) = 0;

    /**
     * Called when the interpretation has failed with a formula error.
     *
     * @param msg Message describing the error.
     */
    virtual void set_formula_error(std::string_view msg) = 0;

    /**
     * Called for an operator or a structural token, such as a parenthesis
     * or a separator.
     *
     * @param fop Opcode of the token.
     */
    virtual void push_token(fopcode_t fop) = 0;

    /**
     * Called for a numeric literal.
     *
     * @param val Numeric value.
     */
    virtual void push_value(double val) = 0;

    /**
     * Called for an error literal.
     *
     * @param err Error value.
     */
    virtual void push_error(formula_error_t err) = 0;

    /**
     * Called for a string literal.
     *
     * @param s String value.
     */
    virtual void push_string(std::string_view s) = 0;

    /**
     * Called for a cell reference.
     *
     * @param addr Cell reference, whose components, if relative, are offsets
     *             from the position of the formula cell.
     * @param pos Position of the formula cell.
     */
    virtual void push_single_ref(const address_t& addr, const abs_address_t& pos) = 0;

    /**
     * Called for a range reference.
     *
     * @param range Range reference, whose components, if relative, are
     *              offsets from the position of the formula cell.
     * @param pos Position of the formula cell.
     */
    virtual void push_range_ref(const range_t& range, const abs_address_t& pos) = 0;

    /**
     * Called for a table reference.
     *
     * @param table Table reference.
     */
    virtual void push_table_ref(const table_ref_t& table) = 0;

    /**
     * Called for a function, before its arguments get processed.
     *
     * @param foc Opcode of the function.
     */
    virtual void push_function(formula_function_t foc) = 0;
};

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
