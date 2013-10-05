/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_HASH_CONTAINER_SET_HPP__
#define __IXION_HASH_CONTAINER_SET_HPP__

#ifdef _IXION_HASH_CONTAINER_COMPAT
#include <hash_set>
#define _ixion_unordered_set_type ::std::hash_set
#else
#include <boost/unordered_set.hpp>
#define _ixion_unordered_set_type ::boost::unordered_set
#endif

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
