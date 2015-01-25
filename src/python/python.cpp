/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <Python.h>

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

struct document
{
    PyObject_HEAD
};

void document_dealloc(document* self)
{
    self->ob_type->tp_free(reinterpret_cast<PyObject*>(self));
}

PyObject* document_new(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwargs*/)
{
    document* self = (document*)type->tp_alloc(type, 0);
    return reinterpret_cast<PyObject*>(self);
}

int document_init(document* self, PyObject* /*args*/, PyObject* /*kwargs*/)
{
    return 0;
}

PyObject* document_append_sheet(document* self, PyObject* args, PyObject* kwargs)
{
    char* sheet_name = NULL;
    if (!PyArg_ParseTuple(args, "s", &sheet_name))
    {
        PyErr_SetString(PyExc_TypeError, "The method must be given a sheet name string");
        return Py_None;
    }

    assert(sheet_name);
    cout << "sheet name: " << sheet_name << endl;

    return Py_None;
}

PyMethodDef document_methods[] =
{
    { "append_sheet", (PyCFunction)document_append_sheet, METH_VARARGS, "append new sheet to the document" },
    { NULL }
};

PyTypeObject document_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                                        // ob_size
    "ixion.Document",                         // tp_name
    sizeof(document),                         // tp_basicsize
    0,                                        // tp_itemsize
    (destructor)document_dealloc,             // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    0,                                        // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
    "ixion document object",                  // tp_doc
    0,		                                  // tp_traverse
    0,		                                  // tp_clear
    0,		                                  // tp_richcompare
    0,		                                  // tp_weaklistoffset
    0,		                                  // tp_iter
    0,		                                  // tp_iternext
    document_methods,                         // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    (initproc)document_init,                  // tp_init
    0,                                        // tp_alloc
    document_new,                             // tp_new
};

PyObject* info(PyObject*, PyObject*)
{
    cout << "ixion version: "
        << ixion::get_version_major() << '.'
        << ixion::get_version_minor() << '.'
        << ixion::get_version_micro() << endl;

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
    ixion::python::document_type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&ixion::python::document_type) < 0)
        return;

    PyObject* m = Py_InitModule("ixion", ixion::python::ixion_methods);

    Py_INCREF(&ixion::python::document_type);
    PyModule_AddObject(
        m, "Document",
        reinterpret_cast<PyObject*>(&ixion::python::document_type));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
