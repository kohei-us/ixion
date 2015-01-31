/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_PYTHON_GLOBAL_HPP
#define INCLUDED_IXION_PYTHON_GLOBAL_HPP

#include "ixion/model_context.hpp"
#include "ixion/formula_name_resolver.hpp"

#include <boost/scoped_ptr.hpp>

namespace ixion { namespace python {

struct document_global
{
    ixion::model_context m_cxt;
    boost::scoped_ptr<ixion::formula_name_resolver> m_resolver;

    document_global();
};

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
