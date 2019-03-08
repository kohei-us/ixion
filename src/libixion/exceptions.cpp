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

general_error::~general_error() throw() {}

const char* general_error::what() const throw()
{
    return m_msg.c_str();
}

void general_error::set_message(const std::string& msg)
{
    m_msg = msg;
}

file_not_found::file_not_found(const std::string& fpath) :
    general_error(fpath)
{
    std::ostringstream os;
    os << "specified file not found: " << fpath;
    set_message(os.str());
}

file_not_found::~file_not_found() throw() {}

model_context_error::model_context_error(const std::string& msg, error_type type) :
    general_error(msg), m_type(type) {}

model_context_error::~model_context_error() throw() {}

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
