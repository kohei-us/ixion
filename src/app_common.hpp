/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_SRC_APP_COMMON_HPP
#define INCLUDED_IXION_SRC_APP_COMMON_HPP

#include <string>

namespace ixion { namespace detail {

std::string_view get_formula_result_output_separator();

std::string load_file_content(const std::string& filepath);

}} // namespace ixion::detail

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
