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
#include <string_view>

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

/**
 * Check the output of a test against the expected content stored in a
 * file, and print the first differing line when they differ.  When the
 * environment variable IXION_TEST_REGENERATE is set to a non-empty value,
 * the file gets overwritten with the output instead.
 *
 * @param output Output of a test.
 * @param expected_path Path to the file that stores the expected output.
 *
 * @return True if the output matches the expected content or the file got
 *         regenerated, otherwise false.
 */
bool check_expected_output(std::string_view output, const std::string& expected_path);

}}

#define IXION_TEST_FUNC_SCOPE ixion::test::stack_printer __sp__(__func__)

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
