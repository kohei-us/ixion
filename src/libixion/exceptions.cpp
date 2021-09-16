/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/exceptions.hpp"

#include <sstream>

namespace ixion {

general_error::general_error() : m_msg() {}

general_error::general_error(const std::string& msg) :
    m_msg(msg) {}

general_error::~general_error() {}

const char* general_error::what() const noexcept
{
    return m_msg.c_str();
}

void general_error::set_message(const std::string& msg)
{
    m_msg = msg;
}

struct formula_error::impl
{
    formula_error_t error;
    std::string msg;
    std::string buffer;

    impl(formula_error_t _error) :
        error(_error) {}

    impl(formula_error_t _error, std::string _msg) :
        error(_error), msg(std::move(_msg)) {}
};

formula_error::formula_error(formula_error_t fe) :
    mp_impl(std::make_unique<impl>(fe)) {}

formula_error::formula_error(formula_error_t fe, std::string msg) :
    mp_impl(std::make_unique<impl>(fe, std::move(msg))) {}

formula_error::formula_error(formula_error&& other) :
    mp_impl(std::move(other.mp_impl))
{
    other.mp_impl = std::make_unique<impl>(formula_error_t::no_error);
}

formula_error::~formula_error()
{
}

const char* formula_error::what() const noexcept
{
    const char* error_name = get_formula_error_name(mp_impl->error);
    if (mp_impl->msg.empty())
        return error_name;

    std::ostringstream os;
    os << mp_impl->msg << " (type: " << error_name << ")";
    mp_impl->buffer = os.str();
    return mp_impl->buffer.data();
}

formula_error_t formula_error::get_error() const
{
    return mp_impl->error;
}

formula_registration_error::formula_registration_error(const std::string& msg)
{
    std::ostringstream os;
    os << "formula_registration_error: " << msg;
    set_message(os.str());
}

formula_registration_error::~formula_registration_error() {}

file_not_found::file_not_found(const std::string& fpath) :
    general_error(fpath)
{
    std::ostringstream os;
    os << "specified file not found: " << fpath;
    set_message(os.str());
}

file_not_found::~file_not_found() {}

model_context_error::model_context_error(const std::string& msg, error_type type) :
    general_error(msg), m_type(type) {}

model_context_error::~model_context_error() {}

model_context_error::error_type model_context_error::get_error_type() const
{
    return m_type;
}

not_implemented_error::not_implemented_error(const std::string& msg)
{
    std::ostringstream os;
    os << "not_implemented_error: " << msg;
    set_message(os.str());
}

not_implemented_error::~not_implemented_error() {}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
