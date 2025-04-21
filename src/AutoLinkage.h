#pragma once
/* $Id: AutoLinkage.h,v 1.1 2025/04/21 13:58:28 cvsuser Exp $
 *
 *  libappupdatee public binding
 *
 *  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
 *
 *  Copyright (c) 2012 - 2025 Adam Young
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#if !defined(LIBAUTOUPDATER_LINKAGE)
#if defined(AUTOUPDATER_STATIC)
#   define LIBAUTOUPDATER_LINKAGE
#   define LIBAUTOUPDATER_ENTRY
#elif defined(WIN32) || defined(_WIN32)
#   if defined(BUILDING_LIBAUTOUPDATER)
#       define LIBAUTOUPDATER_LINKAGE __declspec(dllexport)
#       if !defined(ED25519_BUILD_DLL)
#           define ED25519_BUILD_DLL
#       endif
#  else
#       define LIBAUTOUPDATER_LINKAGE __declspec(dllimport)
#       if !defined(ED25519_DLL)
#           define ED25519_DLL
#       endif
#  endif
#   define LIBAUTOUPDATER_ENTRY __cdecl
#else
#   define LIBAUTOUPDATER_LINKAGE
#   define LIBAUTOUPDATER_ENTRY
#endif
#endif

//end
