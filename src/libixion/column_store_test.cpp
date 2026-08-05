/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.
#include "column_store_type.hpp"
#include "calc_status.hpp"

#include <ixion/cell.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/formula_tokens.hpp>

#include <cassert>
#include <cstdlib>

void test_column_store_clone_formula_cells()
{
    IXION_TEST_FUNC_SCOPE;

    // Build a column store directly without a model context.  The content of
    // the formula token stores is immaterial here since cloning only shares
    // them, so they can remain empty.
    ixion::column_store_t col(6);

    // Set a non-grouped formula cell in row 0, with a cached result of 3.
    col.set(0, new ixion::formula_cell(ixion::formula_tokens_store::create()));
    col.get<ixion::formula_cell*>(0)->set_result_cache(ixion::formula_result(3.0));

    // Set a group of formula cells in rows 2 to 4, sharing one calc status
    // and one token store, with cached results of 32, 33 and 34.
    ixion::calc_status_ptr_t cs(new ixion::calc_status({3, 1}));
    auto ts = ixion::formula_tokens_store::create();

    for (ixion::row_t r = 2; r <= 4; ++r)
    {
        col.set(r, new ixion::formula_cell(r - 2, 0, cs, ts));
        col.get<ixion::formula_cell*>(r)->set_result_cache(ixion::formula_result(30.0 + r));
    }

    // Clone the whole store.  With COW enabled this only borrows the source's
    // element blocks.
    ixion::column_store_t cloned = col.clone();
    assert(cloned.size() == col.size());
    assert(cloned.is_empty(1));
    assert(cloned.is_empty(5));

    if constexpr (ixion::column_store_traits::enable_cow)
    {
        assert(cloned.is_shared());
        assert(col.is_shared());

        // While sharing, both stores return the same formula cell instance.
        assert(col.get<ixion::formula_cell*>(0) == cloned.get<ixion::formula_cell*>(0));

        // Detaching clones the formula element block via formula_cell::cloner.
        cloned.detach();
        assert(!cloned.is_shared());
        assert(col.get<ixion::formula_cell*>(0) != cloned.get<ixion::formula_cell*>(0));

        // The source holds on to the shared store until it detaches itself.
        assert(col.is_shared());
    }
    else
    {
        // Without COW the clone gets its own copy of the blocks up front.
        assert(!cloned.is_shared());
        assert(!col.is_shared());
        assert(col.get<ixion::formula_cell*>(0) != cloned.get<ixion::formula_cell*>(0));
    }

    constexpr auto wait_policy = ixion::formula_result_wait_policy_t::throw_exception;

    // Check the non-grouped formula cell in row 0.
    const ixion::formula_cell* src = col.get<ixion::formula_cell*>(0);
    const ixion::formula_cell* dst = cloned.get<ixion::formula_cell*>(0);
    assert(src != dst);
    assert(src->get_tokens().get() == dst->get_tokens().get());
    assert(!dst->get_group_properties().grouped);
    assert(dst->get_group_properties().identity != src->get_group_properties().identity);
    assert(dst->get_result_cache(wait_policy).get_value() == 3.0);

    // Check the grouped formula cells in rows 2 to 4.
    for (ixion::row_t r = 2; r <= 4; ++r)
    {
        src = col.get<ixion::formula_cell*>(r);
        dst = cloned.get<ixion::formula_cell*>(r);
        assert(src != dst);
        assert(src->get_tokens().get() == dst->get_tokens().get());

        ixion::formula_group_t src_group = src->get_group_properties();
        ixion::formula_group_t dst_group = dst->get_group_properties();
        assert(dst_group.grouped);
        assert(dst_group.identity != src_group.identity);
        assert(dst_group.size.row == 3);
        assert(dst_group.size.column == 1);
        assert(dst->get_result_cache(wait_policy).get_value() == 30.0 + r);
    }

    // All cloned members of the group must share the same calc status.
    auto get_identity = [&cloned](ixion::row_t r)
    {
        return cloned.get<ixion::formula_cell*>(r)->get_group_properties().identity;
    };

    assert(get_identity(2) == get_identity(3));
    assert(get_identity(3) == get_identity(4));

    // Updating results on the cloned cells must not affect the source cells.
    cloned.get<ixion::formula_cell*>(0)->set_result_cache(ixion::formula_result(99.0));

    for (ixion::row_t r = 2; r <= 4; ++r)
        cloned.get<ixion::formula_cell*>(r)->set_result_cache(ixion::formula_result(100.0 + r));

    assert(col.get<ixion::formula_cell*>(0)->get_result_cache(wait_policy).get_value() == 3.0);
    assert(cloned.get<ixion::formula_cell*>(0)->get_result_cache(wait_policy).get_value() == 99.0);

    for (ixion::row_t r = 2; r <= 4; ++r)
    {
        assert(col.get<ixion::formula_cell*>(r)->get_result_cache(wait_policy).get_value() == 30.0 + r);
        assert(cloned.get<ixion::formula_cell*>(r)->get_result_cache(wait_policy).get_value() == 100.0 + r);
    }
}

void test_column_store_cow_implicit_detach()
{
    IXION_TEST_FUNC_SCOPE;

    if constexpr (!ixion::column_store_traits::enable_cow)
        return; // this scenario only exists with copy-on-write

    constexpr auto wait_policy = ixion::formula_result_wait_policy_t::throw_exception;

    // Simulate an edit on a copied sheet: a mutation on a borrowing store
    // detaches it implicitly and leaves the source store unaffected.
    ixion::column_store_t col(3);
    col.set(0, new ixion::formula_cell(ixion::formula_tokens_store::create()));
    col.get<ixion::formula_cell*>(0)->set_result_cache(ixion::formula_result(3.0));
    col.set(1, 1.5);

    ixion::column_store_t cloned = col.clone();
    assert(cloned.is_shared());
    const auto* shared_cell = cloned.get<ixion::formula_cell*>(0);
    assert(shared_cell == col.get<ixion::formula_cell*>(0));

    // Overwrite a numeric cell in the clone, which triggers a detach.
    cloned.set(1, 99.0);
    assert(!cloned.is_shared());
    assert(cloned.get<double>(1) == 99.0);

    // The detach re-instantiated the clone's formula cell with the cached
    // result carried over...
    const auto* detached_cell = cloned.get<ixion::formula_cell*>(0);
    assert(detached_cell != shared_cell);
    assert(detached_cell->get_result_cache(wait_policy).get_value() == 3.0);

    // ...while the source store keeps the original cells and values.
    assert(col.get<ixion::formula_cell*>(0) == shared_cell);
    assert(col.get<double>(1) == 1.5);
}

int main()
{
    test_column_store_clone_formula_cells();
    test_column_store_cow_implicit_detach();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
