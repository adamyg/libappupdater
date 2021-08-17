#pragma once
#ifndef LIBAUTOUPDATER_H_INCLUDED
#define LIBAUTOUPDATER_H_INCLUDED
/* $Id: libautoupdater.h,v 1.8 2021/08/17 05:11:59 cvsuser Exp $
 *
 *  libappupdatee public binding
 *
 *  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
 *
 *  Copyright (c) 2012 - 2021 Adam Young
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
#  else
#       define LIBAUTOUPDATER_LINKAGE __declspec(dllimport)
#  endif
#   define LIBAUTOUPDATER_ENTRY __cdecl
#else
#   define LIBAUTOUPDATER_LINKAGE
#   define LIBAUTOUPDATER_ENTRY
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
    autoupdate_set_console_mode(int val);

LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
    autoupdate_logger_stdout(bool val);

LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
    autoupdate_logger_path(const char *path);

LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
    autoupdate_hosturl_set(const char *url);

LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
    autoupdate_channel_set(const char *url);

LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
    autoupdate_appname_set(const char *appname);

LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
    autoupdate_appversion_set(const char *appversion);

LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
    autoupdate_regpath_set(const char *path);

LIBAUTOUPDATER_LINKAGE int  LIBAUTOUPDATER_ENTRY
    autoupdate_isavailable(void);

LIBAUTOUPDATER_LINKAGE int  LIBAUTOUPDATER_ENTRY
    autoupdate_execute(int mode, int interactive);

#if defined(__cplusplus)
}
#endif

#endif /*LIBAUTOUPDATER_H_INCLUDED*/
