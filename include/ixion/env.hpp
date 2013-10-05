/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_ENV_HPP__
#define __IXION_ENV_HPP__

#ifdef _WIN32
  #ifdef IXION_BUILD
     #ifdef DLL_EXPORT
       #define IXION_DLLPUBLIC __declspec(dllexport)
       #define IXION_DLLPUBLIC_VAR extern __declspec(dllexport)
     #else
       #define IXION_DLLPUBLIC
       #define IXION_DLLPUBLIC_VAR extern
     #endif
  #else
     #define IXION_DLLPUBLIC
     #define IXION_DLLPUBLIC_VAR extern __declspec(dllimport)
  #endif
  #define IXION_DLLLOCAL
#else
  #if defined __GNUC__ && __GNUC__ >= 4
    #define IXION_DLLPUBLIC __attribute__ ((visibility ("default")))
    #define IXION_DLLLOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define IXION_DLLPUBLIC
    #define IXION_DLLLOCAL
  #endif
  #define IXION_DLLPUBLIC_VAR IXION_DLLPUBLIC extern
#endif

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
