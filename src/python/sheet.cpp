/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "sheet.hpp"
#include <structmember.h>

namespace ixion { namespace python {

namespace {

struct sheet
{
    PyObject_HEAD
    PyObject* name; // sheet name

    sheet_data* m_data;
};

void sheet_dealloc(sheet* self)
{
    delete self->m_data;

    Py_XDECREF(self->name);
    self->ob_type->tp_free(reinterpret_cast<PyObject*>(self));
}

PyObject* sheet_new(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwargs*/)
{
    sheet* self = (sheet*)type->tp_alloc(type, 0);
    if (!self)
        return NULL;

    self->m_data = new sheet_data;

    self->name = PyString_FromString("");
    if (!self->name)
    {
        Py_DECREF(self);
        return NULL;
    }
    return reinterpret_cast<PyObject*>(self);
}

int sheet_init(sheet* self, PyObject* args, PyObject* kwargs)
{
    PyObject* name = NULL;
    static const char* kwlist[] = { "name", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", (char**)kwlist, &name))
        return -1;

    if (name)
    {
        PyObject* tmp = self->name;
        Py_INCREF(name);
        self->name = name;
        Py_XDECREF(tmp);
    }

    return 0;
}

PyMethodDef sheet_methods[] =
{
    { NULL }
};

PyMemberDef sheet_members[] =
{
    { (char*)"name", T_OBJECT_EX, offsetof(sheet, name), READONLY, (char*)"sheet name" },
    { NULL }
};

PyTypeObject sheet_type =
{
    PyObject_HEAD_INIT(NULL)
    0,                                        // ob_size
    "ixion.Sheet",                            // tp_name
    sizeof(sheet),                            // tp_basicsize
    0,                                        // tp_itemsize
    (destructor)sheet_dealloc,                // tp_dealloc
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
    "ixion sheet object",                     // tp_doc
    0,		                                  // tp_traverse
    0,		                                  // tp_clear
    0,		                                  // tp_richcompare
    0,		                                  // tp_weaklistoffset
    0,		                                  // tp_iter
    0,		                                  // tp_iternext
    sheet_methods,                            // tp_methods
    sheet_members,                            // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    (initproc)sheet_init,                     // tp_init
    0,                                        // tp_alloc
    sheet_new,                                // tp_new
};

}

PyTypeObject* get_sheet_type()
{
    return &sheet_type;
}

sheet_data* get_sheet_data(PyObject* obj)
{
    return reinterpret_cast<sheet*>(obj)->m_data;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
