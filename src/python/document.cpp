/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "document.hpp"
#include "sheet.hpp"
#include "global.hpp"

#include "ixion/formula.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/config.hpp"

#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>

using namespace std;

namespace ixion { namespace python {

namespace {

/** non-python part of the document data */
struct document_data
{
    document_global m_global;
    vector<PyObject*> m_sheets;

    ~document_data();
};

struct free_pyobj
{
    void operator() (PyObject* p)
    {
        Py_XDECREF(p);
    }
};

document_data::~document_data()
{
    for_each(m_sheets.begin(), m_sheets.end(), free_pyobj());
}

/**
 * Python Document object.
 */
struct pyobj_document
{
    PyObject_HEAD

    document_data* m_data;
};

void document_dealloc(pyobj_document* self)
{
    delete self->m_data;
    Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

PyObject* document_new(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwargs*/)
{
    pyobj_document* self = (pyobj_document*)type->tp_alloc(type, 0);
    self->m_data = new document_data;
    return reinterpret_cast<PyObject*>(self);
}

int document_init(pyobj_document* self, PyObject* /*args*/, PyObject* /*kwargs*/)
{
    return 0;
}

PyObject* document_append_sheet(pyobj_document* self, PyObject* args)
{
    char* sheet_name = nullptr;
    if (!PyArg_ParseTuple(args, "s", &sheet_name))
    {
        PyErr_SetString(PyExc_TypeError, "The method must be given a sheet name string");
        return nullptr;
    }

    assert(sheet_name);

    PyTypeObject* sheet_type = get_sheet_type();
    if (!sheet_type)
        return nullptr;

    PyObject* obj_sheet = sheet_type->tp_new(sheet_type, args, 0);
    if (!obj_sheet)
    {
        PyErr_SetString(PyExc_RuntimeError,
            "Failed to allocate memory for the new sheet object.");
        return nullptr;
    }

    sheet_type->tp_init(obj_sheet, args, 0);

    // Pass model_context to the sheet object.
    sheet_data* sd = get_sheet_data(obj_sheet);
    sd->m_global = &self->m_data->m_global;
    ixion::model_context& cxt = sd->m_global->m_cxt;
    try
    {
        sd->m_sheet_index = cxt.append_sheet(sheet_name);
    }
    catch (const model_context_error& e)
    {
        // Most likely the sheet name already exists in this document.
        Py_XDECREF(obj_sheet);
        switch (e.get_error_type())
        {
            case model_context_error::sheet_name_conflict:
                PyErr_SetString(get_python_document_error(), e.what());
            break;
            default:
                PyErr_SetString(get_python_document_error(),
                    "Sheet insertion failed for unknown reason.");
        }
        return nullptr;
    }
    catch (const general_error& e)
    {
        Py_XDECREF(obj_sheet);
        ostringstream os;
        os << "Sheet insertion failed and the reason is '" << e.what() << "'";
        PyErr_SetString(get_python_document_error(), os.str().c_str());
        return nullptr;
    }

    // Append this sheet instance to the document.
    Py_INCREF(obj_sheet);
    self->m_data->m_sheets.push_back(obj_sheet);

    return obj_sheet;
}

const char* doc_document_calculate =
"Document.calculate([threads])\n"
"\n"
"(Re-)calculate all modified formula cells in the document.\n"
"\n"
"Keyword arguments:\n"
"\n"
"threads -- number of threads to use besides the main thread, or 0 if all\n"
"           calculations are to be performed on the main thread. (default 0)\n"
;

PyObject* document_calculate(pyobj_document* self, PyObject* args, PyObject* kwargs)
{
    static const char* kwlist[] = { "threads", nullptr };

    long threads = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", const_cast<char**>(kwlist), &threads))
    {
        PyErr_SetString(PyExc_TypeError, "Failed to parse the arguments for Document.calculate()");
        return nullptr;
    }

    document_global& dg = self->m_data->m_global;

    // Query additional dirty formula cells and add them to the current set.
    std::vector<abs_range_t> sorted = ixion::query_and_sort_dirty_cells(
        dg.m_cxt, dg.m_modified_cells, &dg.m_dirty_formula_cells);
    ixion::calculate_sorted_cells(dg.m_cxt, sorted, threads);

    dg.m_modified_cells.clear();
    dg.m_dirty_formula_cells.clear();

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* document_get_sheet(pyobj_document* self, PyObject* arg)
{
    const vector<PyObject*>& sheets = self->m_data->m_sheets;
    if (PyLong_Check(arg))
    {
        long index = PyLong_AsLong(arg);
        if (index == -1 && PyErr_Occurred())
            return nullptr;

        if (index < 0 || static_cast<size_t>(index) >= sheets.size())
        {
            PyErr_SetString(PyExc_IndexError, "Out-of-bound sheet index");
            return nullptr;
        }

        PyObject* sheet_obj = sheets[index];
        Py_INCREF(sheet_obj);
        return sheet_obj;
    }

    // Not a python int object.  See if it's a string object.
    const char* name = PyUnicode_AsUTF8(arg);
    if (!name)
    {
        PyErr_SetString(PyExc_TypeError,
            "The 'arg' value must be either of type int or type str.");
        return nullptr;
    }

    // Iterate through all sheets to find a match.
    // TODO : Use string hash to speed up the lookup.
    for (PyObject* sh : sheets)
    {
        PyObject* obj = get_sheet_name(sh);
        if (!obj)
            continue;

        const char* this_name = PyUnicode_AsUTF8(obj);
        if (!this_name)
            continue;

        if (!strcmp(name, this_name))
        {
            Py_INCREF(sh);
            return sh;
        }
    }

    ostringstream os;
    os << "No sheet named '" << name << "' found";
    PyErr_SetString(PyExc_IndexError, os.str().c_str());
    return nullptr;
}

PyObject* document_getter_sheet_names(pyobj_document* self, void* closure)
{
    model_context& cxt = self->m_data->m_global.m_cxt;
    const vector<PyObject*>& sheets = self->m_data->m_sheets;
    size_t n = sheets.size();
    PyObject* t = PyTuple_New(n);
    for (size_t i = 0; i < n; ++i)
    {
        std::string name = cxt.get_sheet_name(i);
        PyObject* o = PyUnicode_FromString(name.c_str());
        PyTuple_SetItem(t, i, o);
    }

    return t;
}

PyMethodDef document_methods[] =
{
    { "append_sheet", (PyCFunction)document_append_sheet, METH_VARARGS, "append new sheet to the document" },
    { "calculate", (PyCFunction)document_calculate, METH_VARARGS | METH_KEYWORDS, doc_document_calculate },
    { "get_sheet", (PyCFunction)document_get_sheet, METH_O, "get a sheet object either by index or name" },
    { nullptr }
};

PyGetSetDef document_getset[] =
{
    { "sheet_names", (getter)document_getter_sheet_names, (setter)nullptr, "A tuple of sheet names", nullptr },
    { nullptr }
};

PyTypeObject document_type =
{
    PyVarObject_HEAD_INIT(nullptr, 0)
    "ixion.Document",                         // tp_name
    sizeof(pyobj_document),                         // tp_basicsize
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
    document_getset,                          // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    (initproc)document_init,                  // tp_init
    0,                                        // tp_alloc
    document_new,                             // tp_new
};

}

PyTypeObject* get_document_type()
{
    return &document_type;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
