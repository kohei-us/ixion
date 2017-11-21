/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/compute_engine.hpp"
#include "ixion/global.hpp"

#include <iostream>
#include <unordered_map>

namespace ixion {

namespace {

struct class_factory
{
    create_compute_engine_t create;
    destroy_compute_engine_t destroy;
};

using class_factory_store_t = std::unordered_map<std::string, class_factory>;

class_factory_store_t class_factory_store;

}

struct compute_engine::impl
{
    impl() {}
};

std::shared_ptr<compute_engine> compute_engine::create(const char* name)
{
    if (!name)
        // Name is not specified. Use the default engine.
        return std::make_shared<compute_engine>();

    class_factory_store_t::iterator it = class_factory_store.find(name);
    if (it == class_factory_store.end())
        // No class factory for this name. Fall back to default.
        return std::make_shared<compute_engine>();

    const class_factory& cf = it->second;
    return std::shared_ptr<compute_engine>(cf.create(), cf.destroy);
}

void compute_engine::add_class(
    const char* name, create_compute_engine_t func_create, destroy_compute_engine_t func_destroy)
{
    class_factory cf;
    cf.create = func_create;
    cf.destroy = func_destroy;

    class_factory_store.emplace(name, cf);
}

compute_engine::compute_engine() :
    mp_impl(ixion::make_unique<impl>())
{
}

compute_engine::~compute_engine()
{
}

void compute_engine::test()
{
    std::cout << __FILE__ << ":" << __LINE__ << " (compute_engine:test): base" << std::endl;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
