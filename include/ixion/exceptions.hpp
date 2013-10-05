/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_EXCEPTIONS_HPP__
#define __IXION_EXCEPTIONS_HPP__

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
private:
    std::string m_msg;
};

class IXION_DLLPUBLIC file_not_found : public std::exception
{
public:
    explicit file_not_found(const std::string& fpath);
    ~file_not_found() throw();
    virtual const char* what() const throw();
private:
    std::string m_fpath;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
