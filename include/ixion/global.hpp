/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_GLOBAL_HPP
#define INCLUDED_IXION_GLOBAL_HPP

#include "ixion/types.hpp"
#include "ixion/env.hpp"

#include <memory>
#include <string>

namespace ixion {

class IXION_DLLPUBLIC global
{
public:
    /**
     * Get current time in seconds since epoch.  Note that the value
     * representing a time may differ from platform to platform.  Use this
     * value only to measure relative time.
     *
     * @return current time in seconds since epoch.
     */
    static double get_current_time();

    static double to_double(const char* p, size_t n);

    static bool to_bool(const char* p, size_t n);
private:
    global();
    global(const global& r);
    ~global();
};

// ============================================================================

class IXION_DLLPUBLIC formula_error : public std::exception
{
    struct impl;
    std::unique_ptr<impl> mp_impl;
public:
    explicit formula_error(formula_error_t fe);
    explicit formula_error(formula_error_t fe, std::string msg);
    formula_error(formula_error&& other);

    virtual ~formula_error() throw();
    virtual const char* what() const throw();

    formula_error_t get_error() const;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
