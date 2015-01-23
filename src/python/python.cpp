/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <Python.h>

#include "ixion/env.hpp"

#include <iostream>

using namespace std;

namespace {

PyObject* py_ixion_info(PyObject*, PyObject*)
{
    cout << "ixion version: unknown" << endl;
    return Py_None;
}

PyMethodDef ixion_methods[] = {
    {"info", py_ixion_info, 0, "Print ixion module information."},
    {NULL, NULL, 0, NULL}
};

}

PyMODINIT_FUNC IXION_DLLPUBLIC
initixion()
{
    Py_InitModule("ixion", ixion_methods);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
