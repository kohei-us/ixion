/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_MACROS_HPP__
#define __IXION_MACROS_HPP__

#define IXION_N_ELEMENTS(array) sizeof(array) / sizeof(array[0])

/**
 * Use this macro with a literal string and it returns the literal string
 * followed by its length.
 */
#define IXION_ASCII(literal) literal, sizeof(literal)-1

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
