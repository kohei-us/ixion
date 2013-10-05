/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/exceptions.hpp"

#include <sstream>

namespace ixion {

general_error::general_error(const std::string& msg) :
    m_msg(msg) {}

general_error::~general_error() throw() {}

const char* general_error::what() const throw()
{
    return m_msg.c_str();
}

file_not_found::file_not_found(const std::string& fpath) :
    m_fpath(fpath)
{
}

file_not_found::~file_not_found() throw()
{
}

const char* file_not_found::what() const throw()
{
    std::ostringstream oss;
    oss << "specified file not found: " << m_fpath;
    return oss.str().c_str();
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
