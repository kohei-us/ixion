/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "document.hpp"
#include "sheet.hpp"

#include "ixion/env.hpp"
#include "ixion/info.hpp"

#include <iostream>
#include <string>

#define IXION_DEBUG_PYTHON 0

using namespace std;

namespace ixion { namespace python {

namespace {

#if IXION_DEBUG_PYTHON
void print_args(PyObject* args)
{
    string args_str;
    PyObject* repr = PyObject_Repr(args);
    if (repr)
    {
        Py_INCREF(repr);
        args_str = PyBytes_AsString(repr);
        Py_DECREF(repr);
    }
    cout << args_str << "\n";
}
#endif

PyObject* info(PyObject*, PyObject*)
{
    cout << "ixion version: "
        << ixion::get_version_major() << '.'
        << ixion::get_version_minor() << '.'
        << ixion::get_version_micro() << endl;

    Py_INCREF(Py_None);
    return Py_None;
}

PyMethodDef ixion_methods[] =
{
    { "info", info, 0, "Print ixion module information." },
    { NULL, NULL, 0, NULL }
};

}

}}

PyMODINIT_FUNC IXION_DLLPUBLIC
initixion()
{
    PyTypeObject* doc_type = ixion::python::get_document_type();
    if (PyType_Ready(doc_type) < 0)
        return;

    PyTypeObject* sheet_type = ixion::python::get_sheet_type();
    if (PyType_Ready(sheet_type) < 0)
        return;

    PyObject* m = Py_InitModule("ixion", ixion::python::ixion_methods);

    Py_INCREF(doc_type);
    PyModule_AddObject(m, "Document", reinterpret_cast<PyObject*>(doc_type));

    Py_INCREF(sheet_type);
    PyModule_AddObject(m, "Sheet", reinterpret_cast<PyObject*>(sheet_type));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
