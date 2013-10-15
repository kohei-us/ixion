/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_INTERFACE_MODEL_CONTEXT_HPP__
#define __IXION_INTERFACE_MODEL_CONTEXT_HPP__

#include "ixion/formula_tokens_fwd.hpp"
#include "ixion/types.hpp"
#include "ixion/exceptions.hpp"

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

namespace ixion {

class formula_cell;
class formula_name_resolver;
class cell_listener_tracker;
class matrix;
struct abs_address_t;
struct abs_range_t;
struct config;

class model_context_error : public general_error
{
public:
    model_context_error(const std::string& msg) : general_error(msg) {}
};

namespace iface {

class session_handler;

/**
 * Interface for model context.  The client code needs to provide concrete
 * implementation of this interface in order to provide access to its
 * content.
 */
class model_context : boost::noncopyable
{
public:
    virtual ~model_context() {}

    virtual const config& get_config() const = 0;
    virtual const formula_name_resolver& get_name_resolver() const = 0;
    virtual cell_listener_tracker& get_cell_listener_tracker() = 0;

    virtual bool is_empty(const abs_address_t& addr) const = 0;
    virtual celltype_t get_celltype(const abs_address_t& addr) const = 0;
    virtual double get_numeric_value(const abs_address_t& addr) const = 0;
    virtual string_id_t get_string_identifier(const abs_address_t& addr) const = 0;
    virtual const formula_cell* get_formula_cell(const abs_address_t& addr) const = 0;
    virtual formula_cell* get_formula_cell(const abs_address_t& addr) = 0;

    virtual const formula_cell* get_named_expression(const ::std::string& name) const = 0;
    virtual const ::std::string* get_named_expression_name(const formula_cell* expr) const = 0;

    /**
     * Obtain range value in matrix form.  Multi-sheet ranges are not
     * supported.  If the specified range consists of multiple sheets, it
     * throws an exception.
     *
     * @param range absolute, single-sheet range address.  Multi-sheet ranges
     *              are not allowed.
     *
     * @return range value represented as matrix.
     */
    virtual matrix get_range_value(const abs_range_t& range) const = 0;

    /**
     * Session handler instance receives various events from the formula
     * interpretation run, in order to respond to those events.  This is
     * optional; the model context implementation is not required to provide a
     * handler.
     *
     * @return non-NULL pointer of the model context provides a session
     *         handler, otherwise a NULL pointer is returned.
     */
    virtual session_handler* get_session_handler() const = 0;

    virtual const formula_tokens_t* get_formula_tokens(sheet_t sheet, size_t identifier) const = 0;
    virtual const formula_tokens_t* get_shared_formula_tokens(sheet_t sheet, size_t identifier) const = 0;
    virtual abs_range_t get_shared_formula_range(sheet_t sheet, size_t identifier) const = 0;

    virtual string_id_t append_string(const char* p, size_t n) = 0;
    virtual string_id_t add_string(const char* p, size_t n) = 0;
    virtual const std::string* get_string(string_id_t identifier) const = 0;

    /**
     * Get the index of sheet from sheet name.
     *
     * @param p pointer to the first character of the sheet name string.
     * @param n length of the sheet name string.
     *
     * @return sheet index
     */
    virtual sheet_t get_sheet_index(const char* p, size_t n) const = 0;

    virtual std::string get_sheet_name(sheet_t sheet) const = 0;
};

}}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
