/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_COMPUTE_ENGINE_HPP
#define INCLUDED_IXION_COMPUTE_ENGINE_HPP

#include <ixion/env.hpp>
#include <ixion/module.hpp>

#include <memory>
#include <string>

namespace ixion { namespace draft {

enum class array_type { unknown, float32, float64, uint32 };

struct array
{
    union
    {
        float* float32;
        double* float64;
        uint32_t* uint32;
        void* data;
    };

    array_type type = array_type::unknown;
    std::size_t size = 0u;
};

/**
 * Default compute engine class that uses CPU for all its computations.
 *
 * <p>This class also serves as the fallback for its child classes in case
 * they don't support the function being requested or the function doesn't
 * meet the criteria that it requires.</p>
 *
 * <p>Each function of this class should not modify the state of the class
 * instance.</p>
 */
class IXION_DLLPUBLIC compute_engine
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    /**
     * Create a compute engine instance.
     *
     * @param name name of the compute engine, or an empty name for the default
     *             one.
     *
     * @return compute engine instance associted with the specified name. Note
     *         that if no compute engine is registered with the specified
     *         name, the default one is created.
     */
    static std::shared_ptr<compute_engine> create(std::string_view name = std::string_view());

    /**
     * Add a new compute engine class.
     *
     * @param hdl handler for the dynamically-loaded module in which the
     *            compute engine being registered resides.
     * @param name name of the compute engine.
     * @param func_create function that creates a new instance of this compute
     *                    engine class.
     * @param func_destroy function that destroyes the instance of this
     *                     compute engine class.
     */
    static void add_class(
        void* hdl, std::string_view name, create_compute_engine_t func_create, destroy_compute_engine_t func_destroy);

    compute_engine();
    virtual ~compute_engine();

    virtual std::string_view get_name() const;

    virtual void compute_fibonacci(array& io);
};

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
