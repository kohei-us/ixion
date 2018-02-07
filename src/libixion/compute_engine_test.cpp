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

void test_create_default()
{
    std::shared_ptr<ixion::draft::compute_engine> p = ixion::draft::compute_engine::create(nullptr);
    assert(p);
    assert(!std::strcmp(p->get_name(), "default"));
}

void test_create_cuda()
{
    std::shared_ptr<ixion::draft::compute_engine> p = ixion::draft::compute_engine::create("cuda");
    assert(p);
    assert(!std::strcmp(p->get_name(), "cuda"));
}

int main()
{
    ixion::draft::init_modules();

    test_create_default();
#ifdef BUILD_CUDA
    test_create_cuda();
#endif

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
