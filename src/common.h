//  $Id: common.h,v 1.11 2021/10/26 13:24:54 cvsuser Exp $
//
//  AutoUpdater: Common definitions.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2022, Adam Young
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//

#if defined(__WATCOMC__)
#if !defined(__STDC_WANT_LIB_EXT1__)
#define __STDC_WANT_LIB_EXT1__ 1                // enable ISO TR24731, sprintf_s etc
#endif
#define NOMINMAX
#endif

#if !defined(WINDOWS_MEAN_AND_LEAN)
#define  WINDOWS_MEAN_AND_LEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif
#if defined(__WATCOMC__) && !defined(NTDDI_VERSION)
#define NTDDI_VERSION 0x05010000                /* Shlobj requirement */
#endif
#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <climits>
#include <new>

#if defined(_MSC_VER)
#pragma warning(disable:4267)                   // 'argument': conversion from 'size_t' to 'int', possible loss of data.
#endif

#if defined(__WATCOMC__)
#if !defined(__cplusplus)
#pragma disable_message(124)                    /* Comparison result always 0 */
#pragma disable_message(136)                    /* Comparison equivalent to 'unsigned == 0' */
#pragma disable_message(303)                    /* Parameter 'enc' has been defined, but not referenced */
#endif
#pragma disable_message(13)                     /* Unreachable code */
#pragma disable_message(201)                    /* Unreachable code */
#pragma disable_message(202)                    /* Symbol 'xxx' has been defined, but not referenced */
#pragma disable_message(367)                    /* Conditional expression in if statement is always true */
#endif

#if defined(__WATCOMC__) && (__WATCOMC__ <= 1300) && !defined(__WATCOMC__NOTHROW__)
#define __WATCOMC__NOTHROW__
#if defined(__cplusplus)                        /* std::nothrow() emulation */
    #include <cstddef>
    #include <string>
    #include <ostream>
#pragma warning (push)
#pragma warning 14 9  // no reference to symbol 'operator new'
    namespace std {
        struct nothrow_t { };
        static nothrow_t const nothrow;
    }
    static inline void* operator new (size_t size, std::nothrow_t const&) {
        return malloc(size);                    /* see: opnew.cpp/rtlibrary */
            // Note: OpenWatcom (1.9 or 2.0) wont throw std::bad_alloc.
    }
    static inline void* operator new [] (size_t _Size, std::nothrow_t const&) {
        return malloc(_Size);
    }
    
/*missing string operators*/
std::ostream& operator<<(std::ostream &stream, const std::string &s);
inline std::ostream& operator<<(std::ostream &stream, const std::string &s) {
    stream.write(s.c_str(), s.length()); return stream;
}

/*missing*/
extern int autosprintf_s( char * s, size_t n, const char * __format, ... );
extern int autovsprintf_s( char * s, size_t n, const char * __format, va_list ap );
extern int autosnprintf_s( char * s, size_t n, const char * __format, ... );
#define sprintf_s autosprintf_s
#define vsprintf_s autovsprintf_s
#define _snprintf_s autosnprintf_s

#pragma warning (pop)
#endif
#endif  //__WATCOMC__

#if defined(__WATCOMC__) && !defined(_countof)
#define _countof(array) (sizeof(array) / sizeof(array[0]))
#endif
