/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef NDEBUG
// release build
#undef NDEBUG
#include <cassert>
#define NDEBUG
#else
// debug build
#include <cassert>
#endif

#include <iostream>
#include <chrono>
#include <string>

namespace ixion { namespace test {

class stack_printer
{
public:
    explicit stack_printer(std::string msg);
    ~stack_printer();

private:
    double get_time() const;

    std::string m_msg;
    double m_start_time;
};

}}

#define IXION_TEST_FUNC_SCOPE ixion::test::stack_printer __sp__(__func__)

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
