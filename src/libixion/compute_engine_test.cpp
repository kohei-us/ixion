/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.

#include <ixion/compute_engine.hpp>
#include <ixion/module.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <functional>

using std::cout;
using std::endl;

namespace {

using test_func_t = std::function<void()>;

std::vector<std::pair<std::string, test_func_t>> tests;

#define REGISTER_TEST(x) tests.emplace_back(#x, x);

template<typename T>
void print_values(std::string_view msg, const T& values)
{
    cout << msg << ": ";

    if (values.size() <= 20)
        std::copy(values.begin(), values.end(), std::ostream_iterator<typename T::value_type>(cout, " "));
    else
    {
        // Print only the first 15 and last 5 values.
        auto it = values.begin();
        auto it_end = it + 15;

        std::copy(it, it_end, std::ostream_iterator<typename T::value_type>(cout, " "));

        cout << "... ";

        it_end = values.end();
        it = it_end - 5;

        std::copy(it, it_end, std::ostream_iterator<typename T::value_type>(cout, " "));
    }

    cout << endl;
}

void print_summary(const std::shared_ptr<ixion::draft::compute_engine>& engine)
{
    cout << "--" << endl;
    cout << "name: " << engine->get_name() << endl;

    std::vector<uint32_t> values(16384u);

    uint32_t n = 0;
    std::generate(values.begin(), values.end(), [&n] { return n++; });

    ixion::draft::array io{};
    io.uint32 = values.data();
    io.size = values.size();
    io.type = ixion::draft::array_type::uint32;

    print_values("fibonacci input", values);
    {
        std::ostringstream os;
        os << "fibonacci (n=" << values.size() << ")";
        ixion::test::stack_printer __stack_printer__(os.str());
        engine->compute_fibonacci(io);
    }
    print_values("fibonacci output", values);
}

}

void test_create_default()
{
    IXION_TEST_FUNC_SCOPE;

    std::shared_ptr<ixion::draft::compute_engine> p = ixion::draft::compute_engine::create();
    assert(p);
    assert(p->get_name() == "default");
    print_summary(p);
}

void test_create_vulkan()
{
    IXION_TEST_FUNC_SCOPE;

    std::shared_ptr<ixion::draft::compute_engine> p = ixion::draft::compute_engine::create("vulkan");
    assert(p);
    assert(p->get_name() == "vulkan");
    print_summary(p);
}

int main()
{
    ixion::draft::init_modules();

    REGISTER_TEST(test_create_default);
#ifdef BUILD_VULKAN
    REGISTER_TEST(test_create_vulkan);
#endif

    for (auto test : tests)
    {
        cout << "--------------------------" << endl;
        cout << "  " << test.first << endl;
        cout << "--------------------------" << endl;
        test.second();
    }

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
