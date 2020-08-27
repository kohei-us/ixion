/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/config.hpp"

namespace ixion {

config::config() :
    sep_function_arg(','),
    sep_matrix_column(','),
    sep_matrix_row(';'),
    output_precision(-1)
{}

config::config(const config& r) :
    sep_function_arg(r.sep_function_arg),
    sep_matrix_column(r.sep_matrix_column),
    sep_matrix_row(r.sep_matrix_row),
    output_precision(r.output_precision) {}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
