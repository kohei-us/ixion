/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "app_common.hpp"

#include <ixion/exceptions.hpp>

#include <fstream>
#include <sstream>

namespace ixion { namespace detail {

std::string_view get_formula_result_output_separator()
{
    static const char* sep =
        "---------------------------------------------------------";
    return sep;
}

std::string load_file_content(const std::string& filepath)
{
    std::ifstream file(filepath.c_str());
    if (!file)
        // failed to open the specified file.
        throw file_not_found(filepath);

    std::ostringstream os;
    os << file.rdbuf();
    file.close();

    return os.str();
}

}} // namespace ixion::detail

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
