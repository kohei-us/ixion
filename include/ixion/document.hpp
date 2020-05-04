/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_DOCUMENT_HPP
#define INCLUDED_IXION_DOCUMENT_HPP

#include "ixion/types.hpp"

#include <memory>

namespace ixion {

/**
 * Higher level document representation designed to handle both cell value
 * storage as well as formula cell calculations.
 */
class IXION_DLLPUBLIC document
{
    struct impl;
    std::unique_ptr<impl> mp_impl;
public:
    document();
    ~document();
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
