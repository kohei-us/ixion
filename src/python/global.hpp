/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_PYTHON_GLOBAL_HPP
#define INCLUDED_IXION_PYTHON_GLOBAL_HPP

#include <Python.h>

#include "ixion/model_context.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/address.hpp"

namespace ixion { namespace python {

struct document_global
{
    model_context m_cxt;

    /**
     * positions of all modified cells (formula and non-formula cells) since
     * last calculation.
     */
    abs_address_set_t m_modified_cells;

    /** positions of all dirty formula cells since last calculation. */
    abs_address_set_t m_dirty_formula_cells;

    std::unique_ptr<formula_name_resolver> m_resolver;

    document_global();
};

PyObject* get_python_document_error();
PyObject* get_python_sheet_error();
PyObject* get_python_formula_error();

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
