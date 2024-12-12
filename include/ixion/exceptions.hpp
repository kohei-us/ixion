/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_EXCEPTIONS_HPP
#define INCLUDED_IXION_EXCEPTIONS_HPP

#include "env.hpp"
#include "types.hpp"

#include <exception>
#include <string>
#include <memory>

namespace ixion {

class IXION_DLLPUBLIC general_error : public std::exception
{
public:
    general_error();
    explicit general_error(const std::string& msg);
    virtual ~general_error();
    virtual const char* what() const noexcept override;

protected:
    void set_message(const std::string& msg);

private:
    std::string m_msg;
};

class IXION_DLLPUBLIC formula_error : public std::exception
{
    struct impl;
    std::unique_ptr<impl> mp_impl;
public:
    explicit formula_error(formula_error_t fe);
    explicit formula_error(formula_error_t fe, std::string msg);
    formula_error(const formula_error& other);
    formula_error(formula_error&& other);

    virtual ~formula_error();
    virtual const char* what() const noexcept override;

    formula_error_t get_error() const;
};

class IXION_DLLPUBLIC file_not_found : public general_error
{
public:
    explicit file_not_found(const std::string& fpath);
    virtual ~file_not_found() override;
};

class IXION_DLLPUBLIC formula_registration_error : public general_error
{
public:
    explicit formula_registration_error(const std::string& msg);
    virtual ~formula_registration_error() override;
};

/**
 * This exception is thrown typically from the ixion::model_context class.
 */
class IXION_DLLPUBLIC model_context_error: public general_error
{
public:
    enum error_type
    {
        circular_dependency,
        invalid_named_expression,
        sheet_name_conflict,
        sheet_size_locked,
        not_implemented
    };

    explicit model_context_error(const std::string& msg, error_type type);
    virtual ~model_context_error() override;

    error_type get_error_type() const;

private:
    error_type m_type;
};

class IXION_DLLPUBLIC not_implemented_error : public general_error
{
public:
    explicit not_implemented_error(const std::string& msg);
    virtual ~not_implemented_error() override;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
