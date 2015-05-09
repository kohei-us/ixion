/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "global.hpp"

namespace ixion { namespace python {

document_global::document_global() :
    m_cxt(),
    m_resolver(ixion::formula_name_resolver::get(formula_name_resolver_excel_a1, &m_cxt))
{}

PyObject* get_python_document_error()
{
    static PyObject* p = PyErr_NewException(const_cast<char*>("ixion.DocumentError"), NULL, NULL);
    return p;
}

PyObject* get_python_sheet_error()
{
    static PyObject* p = PyErr_NewException(const_cast<char*>("ixion.SheetError"), NULL, NULL);
    return p;
}

PyObject* get_python_formula_error()
{
    static PyObject* p = PyErr_NewException(const_cast<char*>("ixion.FormulaError"), NULL, NULL);
    return p;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
