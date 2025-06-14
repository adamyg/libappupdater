#pragma once
//  $Id: NSFormat.h,v 1.9 2025/02/21 19:03:23 cvsuser Exp $
//
//  NSLocalization - String
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2021 - 2025, Adam Young
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

#include <cstddef>

#if defined(__WATCOMC__)
#include <stdarg.h>
#if (__WATCOMC__ >= 1300)
#include <stddef.h>                             /* ptrdiff_t */
#endif
#else
#include <cstdarg>
#endif

#include <vector>
#include <iostream>

#if defined(HAVE_INTTYPES_H) || defined(__MINGW32__)
#include <inttypes.h>                           /* intmax_t */
#endif

#if (defined(_MSC_VER) && (_MSC_VER <= 1600) /*2010*/) || defined(__WATCOMC__)
typedef long long intmax_t;
typedef unsigned long long uintmax_t;
#endif
#if defined(__WATCOMC__)
typedef wchar_t wint_t;
#endif

class NSFormat {
public:
	NSFormat() {
	}

	static int format(char *buf, unsigned size, const char *fmt, ...);
	static int vformat(char *buf, unsigned size, const char *fmt, va_list ap);

public:
#define FLOATING_POINT
#define PRINTF_WIDE_CHAR
#define NO_PRINTF_PERCENT_N

	union arg {
		int		intarg;
		unsigned int	uintarg;
		long		longarg;
		unsigned long	ulongarg;
		long long	longlongarg;
		unsigned long long ulonglongarg;
		ptrdiff_t	ptrdiffarg;
		size_t		sizearg;
#ifdef HAVE_SSIZE_T
		ssize_t		ssizearg;
#endif
		intmax_t	intmaxarg;
		uintmax_t	uintmaxarg;
		void		*pvoidarg;
		const char	*pchararg;
#ifdef FLOATING_POINT
		double		doublearg;
		long double	longdoublearg;
#endif
#ifdef PRINTF_WIDE_CHAR
		wint_t		wintarg;
		const wchar_t	*pwchararg;
#endif
#ifndef NO_PRINTF_PERCENT_N
		signed char	*pschararg;
		short		*pshortarg;
		int		*pintarg;
		long		*plongarg;
		long long	*plonglongarg;
		ptrdiff_t	*pptrdiffarg;
#ifdef HAVE_SSIZE_T
		ssize_t		*pssizearg;
#endif
		intmax_t	*pintmaxarg;
#endif
		void		*pobjectarg;
	};
        
#if defined(__WATCOMC__)
        typedef unsigned ioflags_t;
#else
        typedef std::ios_base::fmtflags ioflags_t;
#endif
        
	struct Argument {
		union arg val;
		unsigned argtype;
	};

	struct Field {
		const char *start, *fmt, *end;
		unsigned short flags;
		int width_arg, width;
		int precision_arg, precision;
		int value_arg;
		ioflags_t ioflags;
		char spec;
	};

	static int parse_format(const char *fmt, va_list ap, struct Field *fields, unsigned fieldno, struct Argument *args, unsigned argno);
	static int exec_format(char *buffer, unsigned size, unsigned fieldno, const struct Field *fields, const struct Argument *args);
};
