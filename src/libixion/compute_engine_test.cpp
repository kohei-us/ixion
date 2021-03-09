/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/compute_engine.hpp>
#include <ixion/module.hpp>
#include <iostream>
#include <cassert>
#include <cstring>

using std::cout;
using std::endl;

void print_summary(const std::shared_ptr<ixion::draft::compute_engine>& engine)
{
    cout << "--" << endl;
    cout << "name: " << engine->get_name() << endl;
}

void test_create_default()
{
    std::shared_ptr<ixion::draft::compute_engine> p = ixion::draft::compute_engine::create(nullptr);
    assert(p);
    assert(!std::strcmp(p->get_name(), "default"));
    print_summary(p);
}

void test_create_vulkan()
{
    std::shared_ptr<ixion::draft::compute_engine> p = ixion::draft::compute_engine::create("vulkan");
    assert(p);
    assert(!std::strcmp(p->get_name(), "vulkan"));
    print_summary(p);
}

int main()
{
    ixion::draft::init_modules();

    test_create_default();
#ifdef BUILD_VULKAN
    test_create_vulkan();
#endif

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
