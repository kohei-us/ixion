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
    explicit general_error(const std::string& msg);
    ~general_error() throw();
    virtual const char* what() const throw();

protected:
    void set_message(const std::string& msg);

private:
    std::string m_msg;
};

class IXION_DLLPUBLIC file_not_found : public general_error
{
public:
    explicit file_not_found(const std::string& fpath);
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
        sheet_name_conflict
    };

    explicit model_context_error(const std::string& msg, error_type type);

    error_type get_error_type() const;

private:
    error_type m_type;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
