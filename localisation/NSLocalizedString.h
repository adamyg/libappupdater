#pragma once
#ifndef NSLocalizedString_h_included
#define NSLocalizedString_h_included
/*  $Id: NSLocalizedString.h,v 1.5 2023/10/17 12:33:56 cvsuser Exp $
 *
 *  NSLocalization - Interface.
 *
 *  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
 *
 *  Copyright (c) 2021 - 2025, Adam Young
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

#include "../libautoupdater.h"

#if defined(__cplusplus)
extern "C" {
#endif

LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
NSLocalizedLoadFile(const char *filename);

LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
NSLocalizedLoadResource(const char *resourceName, const char *tableName);

LIBAUTOUPDATER_LINKAGE const char * LIBAUTOUPDATER_ENTRY
NSLocalizedString(const char *key, const char *comment);

LIBAUTOUPDATER_LINKAGE const char * LIBAUTOUPDATER_ENTRY
NSLocalizedStringFromTable(const char *key, const char *tableName, const char *comment);

#if defined(__cplusplus)
}
#endif

#endif /*NSLocalizedString_h_included*/
