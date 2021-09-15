/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "document.hpp"
#include "sheet.hpp"
#include "global.hpp"

#include "ixion/env.hpp"
#include "ixion/info.hpp"

#include <iostream>
#include <string>

#define IXION_DEBUG_PYTHON 0
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))

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

PyObject* column_label(PyObject* /*module*/, PyObject* args, PyObject* kwargs)
{
    int start;
    int stop;
    int resolver_index = 1; // Excel A1 by default
    static const char* kwlist[] = { "start", "stop", "resolver", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii|i", const_cast<char**>(kwlist), &start, &stop, &resolver_index))
        return nullptr;

    if (start >= stop)
    {
        PyErr_SetString(
            PyExc_IndexError,
            "Start position is larger or equal to the stop position.");
        return nullptr;
    }

    if (start < 0)
    {
        PyErr_SetString(
            PyExc_IndexError,
            "Start position should be larger than or equal to 0.");
        return nullptr;
    }

    auto resolver = formula_name_resolver::get(
        static_cast<formula_name_resolver_t>(resolver_index), nullptr);
    if (!resolver)
    {
        PyErr_SetString(
            get_python_formula_error(), "Specified resolver type is invalid.");
        return nullptr;
    }

    int size = stop - start;
    PyObject* t = PyTuple_New(size);

    for (int i = start; i < stop; ++i)
    {
        string s = resolver->get_column_name(i);
        PyObject* o = PyUnicode_FromString(s.c_str());
        PyTuple_SetItem(t, i-start, o);
    }

    return t;
}

PyMethodDef ixion_methods[] =
{
    { "info", (PyCFunction)info, METH_NOARGS, "Print ixion module information." },
    { "column_label", (PyCFunction)column_label, METH_VARARGS | METH_KEYWORDS,
      "Return a list of column label strings based on specified column range values." },
    { nullptr, nullptr, 0, nullptr }
};

struct module_state
{
    PyObject* error;
};

int ixion_traverse(PyObject* m, visitproc visit, void* arg)
{
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

int ixion_clear(PyObject* m)
{
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

}

struct PyModuleDef moduledef =
{
    PyModuleDef_HEAD_INIT,
    "ixion",
    nullptr,
    sizeof(struct module_state),
    ixion_methods,
    nullptr,
    ixion_traverse,
    ixion_clear,
    nullptr
};

}}

extern "C" {

IXION_DLLPUBLIC PyObject* PyInit_ixion()
{
    PyTypeObject* doc_type = ixion::python::get_document_type();
    if (PyType_Ready(doc_type) < 0)
        return nullptr;

    PyTypeObject* sheet_type = ixion::python::get_sheet_type();
    if (PyType_Ready(sheet_type) < 0)
        return nullptr;

    PyObject* m = PyModule_Create(&ixion::python::moduledef);

    Py_INCREF(doc_type);
    PyModule_AddObject(m, "Document", reinterpret_cast<PyObject*>(doc_type));

    Py_INCREF(sheet_type);
    PyModule_AddObject(m, "Sheet", reinterpret_cast<PyObject*>(sheet_type));

    PyModule_AddObject(
        m, "DocumentError", ixion::python::get_python_document_error());
    PyModule_AddObject(
        m, "SheetError", ixion::python::get_python_sheet_error());
    PyModule_AddObject(
        m, "FormulaError", ixion::python::get_python_formula_error());

    return m;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
