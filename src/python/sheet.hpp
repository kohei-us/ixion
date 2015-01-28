/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_PYTHON_SHEET_HPP
#define INCLUDED_IXION_PYTHON_SHEET_HPP

#include <Python.h>

namespace ixion {

class model_context;

}

namespace ixion { namespace python {

struct sheet_data
{
    ixion::model_context* m_cxt;
};

PyTypeObject* get_sheet_type();

sheet_data* get_sheet_data(PyObject* obj);

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
