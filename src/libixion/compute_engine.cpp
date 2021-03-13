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

namespace ixion { namespace draft {

namespace {

struct class_factory
{
    void* handler;
    create_compute_engine_t create;
    destroy_compute_engine_t destroy;
};

class class_factory_store
{
    using store_type = std::unordered_map<std::string, class_factory>;
    store_type m_store;

public:

    const class_factory* get(const std::string& name) const
    {
        auto it = m_store.find(name);
        if (it == m_store.end())
            return nullptr;

        return &it->second;
    }

    void insert(void* hdl, const char* name, create_compute_engine_t func_create, destroy_compute_engine_t func_destroy)
    {
        class_factory cf;
        cf.handler = hdl;
        cf.create = func_create;
        cf.destroy = func_destroy;
        m_store.emplace(name, cf);
    }

    ~class_factory_store()
    {
        for (auto& kv : m_store)
        {
            class_factory& cf = kv.second;
            unload_module(cf.handler);
        }
    }
};

class_factory_store store;

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

    const class_factory* cf = store.get(name);
    if (!cf)
        // No class factory for this name. Fall back to default.
        return std::make_shared<compute_engine>();

    return std::shared_ptr<compute_engine>(cf->create(), cf->destroy);
}

void compute_engine::add_class(
    void* hdl, const char* name, create_compute_engine_t func_create, destroy_compute_engine_t func_destroy)
{
    store.insert(hdl, name, func_create, func_destroy);
}

compute_engine::compute_engine() :
    mp_impl(std::make_unique<impl>())
{
}

compute_engine::~compute_engine()
{
}

const char* compute_engine::get_name() const
{
    return "default";
}

void compute_engine::compute_fibonacci(array& io)
{
    // TODO : add CPU implementation
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
