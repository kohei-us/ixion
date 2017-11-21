/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/compute_engine.hpp>
#include <ixion/module.hpp>
#include <iostream>

void test_foo()
{
    std::shared_ptr<ixion::compute_engine> p = ixion::compute_engine::create(nullptr);
    p->test();

    p = ixion::compute_engine::create("cuda");
    p->test();
}

int main()
{
    ixion::init_modules();

    test_foo();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
