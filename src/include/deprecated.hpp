/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

// Macros used to temporarily silence [[deprecated]] markers.  Only to be used
// internally; public headers should keep their [[deprecated]] markers visible
// to downstream callers.

#if defined(__GNUC__) || defined(__clang__)
#  define IXION_DEPRECATED_DECL_PUSH \
     _Pragma("GCC diagnostic push") \
     _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#  define IXION_DEPRECATED_DECL_POP _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#  define IXION_DEPRECATED_DECL_PUSH \
     __pragma(warning(push)) \
     __pragma(warning(disable: 4996))
#  define IXION_DEPRECATED_DECL_POP __pragma(warning(pop))
#else
#  define IXION_DEPRECATED_DECL_PUSH
#  define IXION_DEPRECATED_DECL_POP
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
