/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ixion/types.hpp>

#include <iosfwd>
#include <memory>

namespace ixion {

class formula_name_resolver;
class model_context;

namespace detail {

/**
 * Dumps the content of a sheet to an output stream as a human-readable
 * text grid.  Ported from orcus's flat_dumper.
 */
class grid_dumper
{
    const model_context& m_cxt;
    std::unique_ptr<formula_name_resolver> m_own_resolver;
    const formula_name_resolver& m_resolver;

public:
    /**
     * @param cxt Model context to dump sheets from.
     * @param resolver Name resolver that determines the column label style
     *                 as well as the way formula expressions get printed in
     *                 verbose mode.  When null, an Excel A1 resolver gets
     *                 created and used internally.
     */
    grid_dumper(const model_context& cxt, const formula_name_resolver* resolver);
    ~grid_dumper();

    /**
     * Dump the data area of a sheet as a text grid with column and row
     * headers.  An empty sheet produces no output at all.
     *
     * @param os Output stream to dump the sheet content to.
     * @param sheet Index of the sheet to dump.
     * @param mode Amount of detail to include in the output.
     */
    void dump(std::ostream& os, sheet_t sheet, sheet_dump_mode_t mode) const;
};

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
