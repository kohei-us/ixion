/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_EXCEPTIONS_HPP
#define INCLUDED_IXION_EXCEPTIONS_HPP

#include "env.hpp"

#include <exception>
#include <string>

namespace ixion {

class IXION_DLLPUBLIC general_error : public std::exception
{
public:
    general_error();
    explicit general_error(const std::string& msg);
    virtual ~general_error() throw();
    virtual const char* what() const throw() override;

protected:
    void set_message(const std::string& msg);

private:
    std::string m_msg;
};

class IXION_DLLPUBLIC file_not_found : public general_error
{
public:
    explicit file_not_found(const std::string& fpath);
    virtual ~file_not_found() throw() override;
};

class IXION_DLLPUBLIC formula_registration_error : public general_error
{
public:
    explicit formula_registration_error(const std::string& msg);
    virtual ~formula_registration_error() throw() override;
};

/**
 * This exception is thrown typically from the {@link model_context} class.
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
    virtual ~model_context_error() throw() override;

    error_type get_error_type() const;

private:
    error_type m_type;
};

class IXION_DLLPUBLIC not_implemented_error : public general_error
{
public:
    explicit not_implemented_error(const std::string& msg);
    virtual ~not_implemented_error() throw() override;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
