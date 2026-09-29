/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "env.hpp"
#include "types.hpp"

#include <exception>
#include <string>
#include <string_view>
#include <memory>

namespace ixion {

/**
 * Base class of the exceptions thrown by the library, other than
 * formula_error.  It carries a message describing the error.
 */
class IXION_DLLPUBLIC general_error : public std::exception
{
public:
    /** Construct an exception with an empty message. */
    general_error();
    /** Construct an exception with a message. */
    explicit general_error(std::string msg);
    virtual ~general_error();
    /** Get the message describing the error. */
    virtual const char* what() const noexcept override;

protected:
    /** Set the message describing the error. */
    void set_message(std::string msg);

private:
    std::string m_msg;
};

/**
 * Exception carrying a formula error value.  The interpreter throws it
 * while evaluating a formula, in which case the error value becomes the
 * result of the formula cell, and the cell value accessors throw it when
 * asked for a value the cell cannot provide, such as the numeric value of a
 * formula cell in error or of a cell that has not been calculated yet.
 */
class IXION_DLLPUBLIC formula_error : public std::exception
{
    struct impl;
    std::unique_ptr<impl> mp_impl;
public:
    /** Construct an exception with an error value and no message. */
    explicit formula_error(formula_error_t fe);
    /** Construct an exception with an error value and a message. */
    explicit formula_error(formula_error_t fe, std::string msg);
    /** Copy constructor. */
    formula_error(const formula_error& other);
    /** Move constructor. */
    formula_error(formula_error&& other);

    virtual ~formula_error();

    /**
     * Get a description of the error.  Without a message, it is the name of
     * the error value, such as "#DIV/0!".  With a message, it is the message
     * followed by the name of the error value in parentheses.
     */
    virtual const char* what() const noexcept override;

    /** Get the error value. */
    formula_error_t get_error() const;
};

/**
 * Exception thrown when a file to be opened does not exist.
 */
class IXION_DLLPUBLIC file_not_found : public general_error
{
public:
    /**
     * Constructor.
     *
     * @param fpath Path of the file that was not found.
     */
    explicit file_not_found(std::string_view fpath);
    virtual ~file_not_found() override;
};

/**
 * Exception thrown by register_formula_cell() and unregister_formula_cell()
 * when a formula cell cannot be registered with or unregistered from the
 * dirty cell tracker, for instance when the position is not the top-left
 * cell of its formula group.
 */
class IXION_DLLPUBLIC formula_registration_error : public general_error
{
public:
    /** Construct an exception with a message. */
    explicit formula_registration_error(std::string_view msg);
    virtual ~formula_registration_error() override;
};

/**
 * This exception is thrown typically from the ixion::model_context class.
 */
class IXION_DLLPUBLIC model_context_error: public general_error
{
public:
    /** Kind of error. */
    enum error_type
    {
        /** The name of a named expression is not valid. */
        invalid_named_expression,
        /** A sheet by the same name already exists. */
        sheet_name_conflict,
        /** The sheet size cannot be changed once a sheet exists. */
        sheet_size_locked,
        /** A table by the same name already exists. */
        table_name_conflict,
        /** A view by the same name already exists on the sheet. */
        sheet_view_name_conflict,
        /** A formula references a sheet that does not exist. */
        invalid_sheet_reference,
        /** A write covers only part of a formula group. */
        partial_formula_group,
        /** A model_context_loader got finalized a second time. */
        loader_already_finalized
    };

    /**
     * Constructor.
     *
     * @param msg Message describing the error.
     * @param type Kind of error.
     */
    explicit model_context_error(std::string msg, error_type type);
    virtual ~model_context_error() override;

    /** Get the kind of error. */
    error_type get_error_type() const;

private:
    error_type m_type;
};

/**
 * Exception thrown when a requested operation is not implemented.
 */
class IXION_DLLPUBLIC not_implemented_error : public general_error
{
public:
    /** Construct an exception with a message. */
    explicit not_implemented_error(std::string_view msg);
    virtual ~not_implemented_error() override;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
