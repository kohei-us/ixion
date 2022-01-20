/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "sheet.hpp"
#include "global.hpp"

#include <ixion/model_context.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula.hpp>
#include <ixion/cell.hpp>
#include <ixion/exceptions.hpp>

#include <structmember.h>

#include <iostream>

using namespace std;

namespace ixion { namespace python {

sheet_data::sheet_data() : m_global(nullptr), m_sheet_index(-1) {}

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
    Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

PyObject* sheet_new(PyTypeObject* type, PyObject* /*args*/, PyObject* /*kwargs*/)
{
    sheet* self = (sheet*)type->tp_alloc(type, 0);
    if (!self)
        return nullptr;

    self->m_data = new sheet_data;

    self->name = PyUnicode_FromString("");
    if (!self->name)
    {
        Py_DECREF(self);
        return nullptr;
    }
    return reinterpret_cast<PyObject*>(self);
}

int sheet_init(sheet* self, PyObject* args, PyObject* kwargs)
{
    PyObject* name = nullptr;
    static const char* kwlist[] = { "name", nullptr };
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
    int col = -1;
    int row = -1;
    double val = 0.0;

    static const char* kwlist[] = { "row", "column", "value", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iid", const_cast<char**>(kwlist), &row, &col, &val))
        return nullptr;

    sheet_data* sd = get_sheet_data(reinterpret_cast<PyObject*>(self));
    if (!sd->m_global)
    {
        PyErr_SetString(get_python_sheet_error(),
            "This Sheet object does not belong to a Document object.");
        return nullptr;
    }

    ixion::model_context& cxt = sd->m_global->m_cxt;
    ixion::abs_address_t pos(sd->m_sheet_index, row, col);
    sd->m_global->m_modified_cells.insert(pos);
    cxt.set_numeric_cell(pos, val);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* sheet_set_string_cell(sheet* self, PyObject* args, PyObject* kwargs)
{
    int col = -1;
    int row = -1;
    char* val = nullptr;

    static const char* kwlist[] = { "row", "column", "value", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iis", const_cast<char**>(kwlist), &row, &col, &val))
        return nullptr;

    sheet_data* sd = get_sheet_data(reinterpret_cast<PyObject*>(self));
    if (!sd->m_global)
    {
        PyErr_SetString(get_python_sheet_error(),
            "This Sheet object does not belong to a Document object.");
        return nullptr;
    }

    ixion::model_context& cxt = sd->m_global->m_cxt;
    ixion::abs_address_t pos(sd->m_sheet_index, row, col);
    sd->m_global->m_modified_cells.insert(pos);
    cxt.set_string_cell(pos, val);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* sheet_set_formula_cell(sheet* self, PyObject* args, PyObject* kwargs)
{
    int col = -1;
    int row = -1;
    char* formula = nullptr;

    static const char* kwlist[] = { "row", "column", "value", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iis", const_cast<char**>(kwlist), &row, &col, &formula))
        return nullptr;

    sheet_data* sd = get_sheet_data(reinterpret_cast<PyObject*>(self));
    if (!sd->m_global)
    {
        PyErr_SetString(get_python_sheet_error(),
            "This Sheet object does not belong to a Document object.");
        return nullptr;
    }

    ixion::model_context& cxt = sd->m_global->m_cxt;

    ixion::abs_address_t pos(sd->m_sheet_index, row, col);
    sd->m_global->m_dirty_formula_cells.insert(pos);

    ixion::formula_tokens_t tokens =
        ixion::parse_formula_string(cxt, pos, *sd->m_global->m_resolver, formula);

    auto ts = formula_tokens_store::create();
    ts->get() = std::move(tokens);
    cxt.set_formula_cell(pos, ts);

    // Put this formula cell in a dependency chain.
    ixion::register_formula_cell(cxt, pos);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* sheet_get_numeric_value(sheet* self, PyObject* args, PyObject* kwargs)
{
    int col = -1;
    int row = -1;

    static const char* kwlist[] = { "row", "column", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii", const_cast<char**>(kwlist), &row, &col))
        return nullptr;

    sheet_data* sd = get_sheet_data(reinterpret_cast<PyObject*>(self));
    if (!sd->m_global)
    {
        PyErr_SetString(get_python_sheet_error(),
            "This Sheet object does not belong to a Document object.");
        return nullptr;
    }

    ixion::model_context& cxt = sd->m_global->m_cxt;
    double val = 0.0;
    try
    {
        val = cxt.get_numeric_value(ixion::abs_address_t(sd->m_sheet_index, row, col));
    }
    catch (const formula_error&)
    {
        PyErr_SetString(PyExc_TypeError, "The formula cell has yet to be calculated");
        return nullptr;
    }

    return PyFloat_FromDouble(val);
}

PyObject* sheet_get_string_value(sheet* self, PyObject* args, PyObject* kwargs)
{
    int col = -1;
    int row = -1;

    static const char* kwlist[] = { "row", "column", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii", const_cast<char**>(kwlist), &row, &col))
        return nullptr;

    sheet_data* sd = get_sheet_data(reinterpret_cast<PyObject*>(self));
    if (!sd->m_global)
    {
        PyErr_SetString(get_python_sheet_error(),
            "This Sheet object does not belong to a Document object.");
        return nullptr;
    }

    ixion::model_context& cxt = sd->m_global->m_cxt;
    std::string_view s = cxt.get_string_value(ixion::abs_address_t(sd->m_sheet_index, row, col));
    if (s.empty())
        return PyUnicode_FromStringAndSize(nullptr, 0);

    return PyUnicode_FromStringAndSize(s.data(), s.size());
}

PyObject* sheet_get_formula_expression(sheet* self, PyObject* args, PyObject* kwargs)
{
    int col = -1;
    int row = -1;

    static const char* kwlist[] = { "row", "column", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii", const_cast<char**>(kwlist), &row, &col))
        return nullptr;

    sheet_data* sd = get_sheet_data(reinterpret_cast<PyObject*>(self));
    if (!sd->m_global)
    {
        PyErr_SetString(get_python_sheet_error(),
            "This Sheet object does not belong to a Document object.");
        return nullptr;
    }

    ixion::model_context& cxt = sd->m_global->m_cxt;
    ixion::abs_address_t pos(sd->m_sheet_index, row, col);
    const ixion::formula_cell* fc = cxt.get_formula_cell(pos);

    if (!fc)
    {
        PyErr_SetString(get_python_sheet_error(), "No formula cell at specified position.");
        return nullptr;
    }

    const ixion::formula_tokens_t& ft = fc->get_tokens()->get();

    string str = ixion::print_formula_tokens(cxt, pos, *sd->m_global->m_resolver, ft);
    if (str.empty())
        return PyUnicode_FromString("");

    return PyUnicode_FromStringAndSize(str.data(), str.size());
}

PyObject* sheet_erase_cell(sheet* self, PyObject* args, PyObject* kwargs)
{
    PyErr_SetString(PyExc_RuntimeError, "erase_cell() method has been deprecated. Please use empty_cell() instead.");
    return nullptr;
}

PyObject* sheet_empty_cell(sheet* self, PyObject* args, PyObject* kwargs)
{
    int col = -1;
    int row = -1;

    static const char* kwlist[] = { "row", "column", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii", const_cast<char**>(kwlist), &row, &col))
        return nullptr;

    sheet_data* sd = get_sheet_data(reinterpret_cast<PyObject*>(self));
    if (!sd->m_global)
    {
        PyErr_SetString(get_python_sheet_error(),
            "This Sheet object does not belong to a Document object.");
        return nullptr;
    }

    ixion::model_context& cxt = sd->m_global->m_cxt;
    abs_address_t pos(sd->m_sheet_index, row, col);
    sd->m_global->m_modified_cells.insert(pos);
    cxt.empty_cell(pos);

    Py_INCREF(Py_None);
    return Py_None;
}

PyMethodDef sheet_methods[] =
{
    { "set_numeric_cell",  (PyCFunction)sheet_set_numeric_cell,  METH_VARARGS | METH_KEYWORDS, "set numeric value to specified cell" },
    { "set_formula_cell",  (PyCFunction)sheet_set_formula_cell,  METH_VARARGS | METH_KEYWORDS, "set formula to specified cell" },
    { "set_string_cell",   (PyCFunction)sheet_set_string_cell,   METH_VARARGS | METH_KEYWORDS, "set string to specified cell" },
    { "get_numeric_value", (PyCFunction)sheet_get_numeric_value, METH_VARARGS | METH_KEYWORDS, "get numeric value from specified cell" },
    { "get_string_value",  (PyCFunction)sheet_get_string_value,  METH_VARARGS | METH_KEYWORDS, "get string value from specified cell" },
    { "get_formula_expression", (PyCFunction)sheet_get_formula_expression, METH_VARARGS | METH_KEYWORDS, "get formula expression string from specified cell position" },
    { "empty_cell", (PyCFunction)sheet_empty_cell, METH_VARARGS | METH_KEYWORDS, "empty cell at specified position" },
    { "erase_cell", (PyCFunction)sheet_erase_cell, METH_VARARGS | METH_KEYWORDS, "erase cell at specified position" },
    { nullptr }
};

PyMemberDef sheet_members[] =
{
    { (char*)"name", T_OBJECT_EX, offsetof(sheet, name), READONLY, (char*)"sheet name" },
    { nullptr }
};

PyTypeObject sheet_type =
{
    PyVarObject_HEAD_INIT(nullptr, 0)
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

PyObject* get_sheet_name(PyObject* obj)
{
    return reinterpret_cast<sheet*>(obj)->name;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
