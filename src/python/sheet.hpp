/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_PYTHON_SHEET_HPP
#define INCLUDED_IXION_PYTHON_SHEET_HPP

#include <Python.h>

#include "ixion/types.hpp"

namespace ixion { namespace python {

struct document_global;

struct sheet_data
{
    document_global* m_global;
    ixion::sheet_t m_sheet_index;

    sheet_data();
};

PyTypeObject* get_sheet_type();

sheet_data* get_sheet_data(PyObject* obj);

/**
 * Get the sheet name of a python sheet object.
 *
 * @param obj python sheet object.
 *
 * @return python string object storing the sheet name.
 */
PyObject* get_sheet_name(PyObject* obj);

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
