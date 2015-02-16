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

#include <iostream>
#include <vector>
#include <algorithm>

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

struct document
{
    PyObject_HEAD

    document_data* m_data;
};

void document_dealloc(document* self)
{
    delete self->m_data;
    self->ob_type->tp_free(reinterpret_cast<PyObject*>(self));
}

PyObject* document_new(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwargs*/)
{
    document* self = (document*)type->tp_alloc(type, 0);
    self->m_data = new document_data;
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
        return NULL;
    }

    assert(sheet_name);

    PyTypeObject* sheet_type = get_sheet_type();
    if (!sheet_type)
        return NULL;

    PyObject* obj_sheet = sheet_type->tp_new(sheet_type, args, kwargs);
    if (!obj_sheet)
        return NULL;

    sheet_type->tp_init(obj_sheet, args, kwargs);

    // Pass model_context to the sheet object.
    sheet_data* sd = get_sheet_data(obj_sheet);
    sd->m_global = &self->m_data->m_global;
    ixion::model_context& cxt = sd->m_global->m_cxt;
    sd->m_sheet_index = cxt.append_sheet(sheet_name, strlen(sheet_name), 1048576, 1024);

    // Append this sheet instance to the document.
    Py_INCREF(obj_sheet);
    self->m_data->m_sheets.push_back(obj_sheet);

    return obj_sheet;
}

PyObject* document_calculate(document* self, PyObject*, PyObject*)
{
    model_context& cxt = self->m_data->m_global.m_cxt;
    modified_cells_t& mod_cells = self->m_data->m_global.m_modified_cells;
    dirty_formula_cells_t& dirty_fcells = self->m_data->m_global.m_dirty_formula_cells;

    ixion::get_all_dirty_cells(cxt, mod_cells, dirty_fcells);
    calculate_cells(cxt, dirty_fcells, 0);
    mod_cells.clear();
    dirty_fcells.clear();

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* document_get_sheet(document* self, PyObject* args, PyObject*)
{
    PyObject* arg = NULL;
    if (!PyArg_ParseTuple(args, "O", &arg))
        return NULL;

    long index = PyInt_AsLong(arg);
    if (index == -1 && PyErr_Occurred())
        return NULL;

    const vector<PyObject*>& sheets = self->m_data->m_sheets;
    if (index < 0 || index >= sheets.size())
    {
        PyErr_SetString(PyExc_IndexError, "Out-of-bound sheet index");
        return NULL;
    }

    PyObject* sheet_obj = sheets[index];
    Py_INCREF(sheet_obj);
    return sheet_obj;
}

PyObject* document_get_sheet_names(document* self, PyObject*, PyObject*)
{
    model_context& cxt = self->m_data->m_global.m_cxt;
    const vector<PyObject*>& sheets = self->m_data->m_sheets;
    size_t n = sheets.size();
    PyObject* t = PyTuple_New(n);
    for (size_t i = 0; i < n; ++i)
    {
        std::string name = cxt.get_sheet_name(i);
        PyObject* o = PyString_FromString(name.c_str());
        PyTuple_SetItem(t, i, o);
    }

    return t;
}

PyMethodDef document_methods[] =
{
    { "append_sheet", (PyCFunction)document_append_sheet, METH_VARARGS, "append new sheet to the document" },
    { "calculate", (PyCFunction)document_calculate, 0, "calculate formula cells" },
    { "get_sheet_names", (PyCFunction)document_get_sheet_names, 0, "get a tuple of sheet names" },
    { "get_sheet", (PyCFunction)document_get_sheet, METH_KEYWORDS, "get a sheet object either by index or name" },
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

}

PyTypeObject* get_document_type()
{
    return &document_type;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
