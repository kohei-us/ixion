/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "sheet.hpp"

#include "ixion/model_context.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula.hpp"

#include <structmember.h>
#include <boost/scoped_ptr.hpp>

#include <iostream>

using namespace std;

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

PyObject* sheet_set_numeric_cell(sheet* self, PyObject* args, PyObject* kwargs)
{
    long col = -1;
    long row = -1;
    double val = 0.0;

    static char* kwlist[] = { "row", "column", "value", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iid", kwlist, &row, &col, &val))
        return Py_None;

    sheet_data* sd = get_sheet_data(reinterpret_cast<PyObject*>(self));
    assert(sd->m_cxt);
    ixion::model_context& cxt = *sd->m_cxt;
    cxt.set_numeric_cell(ixion::abs_address_t(sd->m_sheet_index, row, col), val);

    return Py_None;
}

PyObject* sheet_set_formula_cell(sheet* self, PyObject* args, PyObject* kwargs)
{
    long col = -1;
    long row = -1;
    char* formula = NULL;

    static char* kwlist[] = { "row", "column", "value", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iis", kwlist, &row, &col, &formula))
        return Py_None;

    sheet_data* sd = get_sheet_data(reinterpret_cast<PyObject*>(self));
    assert(sd->m_cxt);
    ixion::model_context& cxt = *sd->m_cxt;

    // TODO : Store this resolver instance in a central place to avoid
    // creating one each time.
    boost::scoped_ptr<formula_name_resolver> resolver(
        formula_name_resolver::get(ixion::formula_name_resolver_excel_a1, &cxt));

    ixion::abs_address_t pos(sd->m_sheet_index, row, col);
    cxt.set_formula_cell(pos, formula, strlen(formula), *resolver);

    // Put this formula cell in a dependency chain.
    ixion::register_formula_cell(cxt, pos);

    return Py_None;
}

PyObject* sheet_get_numeric_value(sheet* self, PyObject* args, PyObject* kwargs)
{
    long col = -1;
    long row = -1;

    static char* kwlist[] = { "row", "column", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii", kwlist, &row, &col))
        return Py_None;

    sheet_data* sd = get_sheet_data(reinterpret_cast<PyObject*>(self));
    assert(sd->m_cxt);
    ixion::model_context& cxt = *sd->m_cxt;
    double val = cxt.get_numeric_value(ixion::abs_address_t(sd->m_sheet_index, row, col));

    return PyFloat_FromDouble(val);
}

PyObject* sheet_get_formula_expression(sheet* self, PyObject* args, PyObject* kwargs)
{
    long col = -1;
    long row = -1;

    static char* kwlist[] = { "row", "column", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii", kwlist, &row, &col))
        return Py_None;

    sheet_data* sd = get_sheet_data(reinterpret_cast<PyObject*>(self));
    assert(sd->m_cxt);
    ixion::model_context& cxt = *sd->m_cxt;
    ixion::abs_address_t pos(sd->m_sheet_index, row, col);
    const ixion::formula_cell* fc = cxt.get_formula_cell(pos);

    if (!fc)
        return Py_None;

    size_t tid = fc->get_identifier();
    const formula_tokens_t* ft = cxt.get_formula_tokens(sd->m_sheet_index, tid);
    if (!ft)
        return Py_None;

    // TODO : Store this resolver instance in a central place to avoid
    // creating one each time.
    boost::scoped_ptr<formula_name_resolver> resolver(
        formula_name_resolver::get(ixion::formula_name_resolver_excel_a1, &cxt));

    string str;
    ixion::print_formula_tokens(cxt, pos, *resolver, *ft, str);
    if (str.empty())
        return PyString_FromString("");

    return PyString_FromStringAndSize(str.data(), str.size());
}

PyMethodDef sheet_methods[] =
{
    { "set_numeric_cell",  (PyCFunction)sheet_set_numeric_cell,  METH_KEYWORDS, "set numeric value to specified cell" },
    { "set_formula_cell",  (PyCFunction)sheet_set_formula_cell,  METH_KEYWORDS, "set formula to specified cell" },
    { "get_numeric_value", (PyCFunction)sheet_get_numeric_value, METH_KEYWORDS, "get numeric value from specified cell" },
    { "get_formula_expression", (PyCFunction)sheet_get_formula_expression, METH_KEYWORDS, "get formula expression string from specified cell position" },
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
