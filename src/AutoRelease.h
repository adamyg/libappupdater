#ifndef AUTORELEASE_H_INCLUDED
#define AUTORELEASE_H_INCLUDED
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2025, Adam Young
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

#define AUTORELEASE_VERSION         1,0,3,0
#define AUTORELEASE_FILEVERSION     "1.0.3.0"

#if defined(_MSC_VER)
#if   (_MSC_VER >= 1930)                        /* (Visual Studio 2022 / MSVC++ 17.x) */
#define AUTORELEASE_FILENAME        "libappupdater.vs170.dll"
#elif (_MSC_VER >= 1920)                        /* (Visual Studio 2019 / MSVC++ 16.x) */
#define AUTORELEASE_FILENAME        "libappupdater.vs160.dll"
#elif (_MSC_VER >= 1910)                        /* (Visual Studio 2017 / MSVC++ 15.x) */
#define AUTORELEASE_FILENAME        "libappupdater.vs150.dll"
#elif (_MSC_VER >= 1900)                        /* (Visual Studio 2015 / MSVC++ 14.0) */
#define AUTORELEASE_FILENAME        "libappupdater.vs140.dll"
#elif (_MSC_VER >= 1800)                        /* (Visual Studio 2013 / MSVC++ 12.0) */
#define AUTORELEASE_FILENAME        "libappupdater.vs120.dll"
#elif (_MSC_VER >= 1700)                        /* (Visual Studio 2012 / MSVC++ 11.0) */
#define AUTORELEASE_FILENAME        "libappupdater.vs110.dll"
#elif (_MSC_VER >= 1600)                        /* (Visual Studio 2010 / MSVC++ 10.0) */
#define AUTORELEASE_FILENAME        "libappupdater.vs110.dll"
#elif (_MSC_VER >= 1500)                        /* (Visual Studio 2008 / MSVC++ 9.0)  */
#define AUTORELEASE_FILENAME        "libappupdater.vs90.dll"
#else
#define AUTORELEASE_FILENAME        "libappupdater.vs.dll"
#endif

#elif defined(__WATCOMC__)
#define AUTORELEASE_FILENAME        "libappupdater.owc.dll"

#else
#define AUTORELEASE_FILENAME        "libappupdater.dll"
#endif

#endif  /*AUTORELEASE_H_INCLUDED*/

