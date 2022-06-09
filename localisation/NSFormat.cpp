/*  $Id: NSFormat.cpp,v 1.4 2021/10/19 15:53:57 cvsuser Exp $
 *
 *  NSLocalization - String.
 *
 *  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
 *
 *  Copyright (c) 2021 - 2022, Adam Young
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

/*-
 *  OpenBSD: vfprintf.c
 *
 *  Copyright (c) 1990 The Regents of the University of California.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to Berkeley by
 *  Chris Torek.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the University nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 */

#include <cstdarg>
#include <cerrno>
#include <cassert>
#include <climits>

#include "NSFormat.h"
#include "nslocal.h"

#include <iostream>
#if defined(__WATCOMC__)
#include <stdlib.h> // C header; <cstdlib> requires std namespace.
#include <string.h> // C header; <cstring> requires std namespace.
#include <strstream>
#else
#pragma warning(push)
#pragma warning(disable:4244)
#pragma warning(disable:4355)
#include "BufferStream.hpp"
#pragma warning(pop)
#endif

/*
 *  Type ids for argument type table.
 */

enum {
	T_UNUSED = 0,
	T_SHORT,
	T_U_SHORT,
	T_INT,
	T_U_INT,
	T_LONG,
	T_U_LONG,
	T_LLONG,
	T_U_LLONG,
	TP_CHAR,
	TP_VOID,
	T_PTRINT,
	T_SIZEINT,
	T_SSIZEINT,
	T_MAXINT,
	T_MAXUINT,
	T_CHAR,
	T_U_CHAR,
#ifdef FLOATING_POINT
	T_DOUBLE,
	T_LONG_DOUBLE,
#endif
#ifdef PRINTF_WIDE_CHAR
	T_WINT,
	TP_WCHAR,
#endif
	TP_OBJECT,	/* object */

#ifndef NO_PRINTF_PERCENT_N
	TP_SHORT,
	TP_INT,
	TP_LONG,
	TP_LLONG,
	TP_PTRINT,
#ifdef HAVE_SSIZE_T
	TP_SSIZEINT,
#endif
	TP_MAXINT
#endif //!NO_PRINTF_PERCENT_N
};


/*
 * Macros for converting digits to letters and vice versa
 */
#define to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define to_char(n)	((n) + '0')


/*
 * Flags used during conversion.
 */
#define ALT		0x0001		/* alternate form */
#define LADJUST		0x0004		/* left adjustment */
#define LONGDBL		0x0008		/* long double */
#define LONGINT		0x0010		/* long integer */
#define LLONGINT	0x0020		/* long long integer */
#define SHORTINT	0x0040		/* short integer */
#define ZEROPAD		0x0080		/* zero (as opposed to blank) pad */
#define FPT		0x0100		/* Floating point number */
#define PTRINT		0x0200		/* (unsigned) ptrdiff_t */
#define SIZEINT		0x0400		/* (signed) size_t */
#define CHARINT		0x0800		/* 8 bit integer */
#define MAXINT		0x1000		/* largest integer size (intmax_t) */

#define PLUSSIGN	0x2000		/* leading +/- */
#define SPACESIGN	0x4000		/* space pad sign */

#define ISZ		0x8000		/* I64 or I64 */
#if (LONG_MAX == 2147483647L)
#define SIZE32		LONGINT
#endif
#if (LLONG_MAX > 2147483647L)
#define SIZE64		LLONGINT
#endif


int
NSFormat::format(char *buffer, unsigned size, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = NSFormat::vformat(buffer, size, fmt, ap);
	va_end(ap);
	return ret;
}


int
NSFormat::vformat(char *buffer, unsigned size, const char *fmt, va_list ap)
{
	struct NSFormat::Argument arguments[32] = {0};
	struct NSFormat::Field fields[32] = {0};
	int fieldno;

	if ((fieldno = NSFormat::parse_format(fmt, ap, fields, _countof(fields), arguments, _countof(arguments))) < 0) {
		return -1;
	}
	return NSFormat::exec_format(buffer, size, fieldno, fields, arguments);
}


#if defined(__WATCOMC__)
static size_t
strnlen(const char *s, size_t maxlen)
{
    size_t len;
    for (len = 0; len < maxlen; ++len, ++s) {
        if (! *s) {
            break;
        }
    }
    return (len);
}
#endif  //__WATCOMC__


int
NSFormat::exec_format(char *buffer, unsigned size, unsigned fieldno, const struct Field *fields, const struct Argument *arguments)
{
#if defined(__WATCOMC__)
	std::ostrstream text(buffer, size - 1);
#define STELL() (buffer + text.pcount())
#else
	boost::interprocess::obufferstream text(buffer, size - 1);
#define STELL() (buffer + (int)text.tellp())
#endif

	for (const struct Field *f = fields, *fend = f + fieldno; f < fend; ++f) {

		// leading text
		if (NULL == f->fmt) {
			if (f->start) {
				text.write(f->start, f->end - f->start);
			}
			continue;
		}
		text.write(f->start, f->fmt - f->start);

		// field
		char *start = NULL;
		int width = (f->width_arg ? arguments[f->width_arg].val.intarg : f->width);
		int precision = (f->precision_arg ? arguments[f->precision_arg].val.intarg : f->precision);
		unsigned short flags = f->flags;
		const char spec = f->spec;
		int ioflags = f->ioflags;

		text.unsetf(std::ios::adjustfield | std::ios::basefield | std::ios::floatfield);

		if (width < 0) {
			/*
			 * ``A negative field width argument is taken as a flag followed by a positive field width.'' -- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			flags |= LADJUST;
			width = -width;
		}

		//  strings and characters
                //
		if ('s' == spec /*TODO: || 'S' == spec*/) {
			const char *s = arguments[f->value_arg].val.pchararg;

			assert(TP_CHAR == arguments[f->value_arg].argtype);
			assert(0 == (flags & (PLUSSIGN|SPACESIGN|ZEROPAD)));

			text.fill(' ');
			if (NULL == s) s = "(null)";
			if (precision >= 0) {
				const int length = (int)strnlen(s, precision);
				if (length < width && (0 == (LADJUST & flags))) {
					//text.fill((ZEROPAD == ((ZEROPAD | LADJUST) & flags)) ? '0' : ' ');
					text.width(width - length);
					text << "";
				}
				text.write(s, length);
				if (length < width && (0 != (LADJUST & flags))) {
					//text.fill(' ');
					text.width(width - length);
					text << "";
				}
			} else {
				if (LADJUST & flags)
					text.setf(std::ios::left);
				//text.fill((ZEROPAD == ((ZEROPAD | LADJUST) & flags)) ? '0' : ' ');
				text.width(width);
				text << s;
			}
			continue;

		} else if ('c' == spec /*TODO: || 'C' == spec*/) {
			const char c = (char)arguments[f->value_arg].val.intarg;
			if (LADJUST & flags)
				text.setf(std::ios::left);
			text.fill((ZEROPAD == ((ZEROPAD | LADJUST) & flags)) ? '0' : ' ');
			text.width(width);
			text << c;
			continue;

		} else if ('%' == spec) {
			const char *fmt = f->fmt + 1;
			assert('%' == fmt[-1]);
			text.write(fmt, f->end - fmt);
			continue;
		}

		//  numeric fields
		//
		const Argument *arg = arguments + f->value_arg;

		if (LADJUST  & flags)
			ioflags |= std::ios::left;

		if ((PLUSSIGN|SPACESIGN) & flags)
			ioflags |= std::ios::showpos;

		if (ZEROPAD == ((ZEROPAD|LADJUST) & flags))
			ioflags |= std::ios::internal;
				// Note: ignored when left-aligned.

		if (FPT & flags) { //float
			if (ALT & flags)
				ioflags |= std::ios::showpoint;

			if (precision < 0) { //default precison
				precision = 6;
				if ('a' == spec || 'A' == spec) {
					precision = 13;
				}
			}

			text.width(width);
			if (precision >= 0)
				text.precision(precision);

		} else { //numeric
			int t_width = width;

			if (ALT & flags)
				ioflags |= std::ios::showbase;

			if (precision >= 0) { //zeropad
				if ((ALT & flags) && (std::ios::hex & ioflags)) {
					precision += 2; //0x, prefix
				} else {
					switch (arg->argtype) { //sign; explicit or implicit
					case T_SHORT: case T_INT:
						if (((PLUSSIGN|SPACESIGN) & flags) || arg->val.intarg < 0)
							++precision; //sign, prefix
						break;
					case T_LONG:
						if (((PLUSSIGN|SPACESIGN) & flags) || arg->val.longarg < 0) 
							++precision; //sign, prefix
						break;
					case T_LLONG:
						if (((PLUSSIGN|SPACESIGN) & flags) || arg->val.longlongarg < 0)
							++precision; //sign, prefix
						break;
					case T_PTRINT:
						if (((PLUSSIGN|SPACESIGN) & flags) || arg->val.ptrdiffarg < 0)
							++precision; //sign, prefix
						break;
					}
				}
				if (width < precision)
					width = precision;
				t_width = precision;
				ioflags |=  std::ios::internal; //pad style
				ioflags &= ~std::ios::left; //manually applied
			}
			if ('p' == spec) t_width = 0;
			text.width(t_width);
		}

		text.setf(ioflags);
		text.fill((ioflags & std::ios::internal) ? '0' : ' ');

		start = STELL();
		switch (arg->argtype) {
		case T_CHAR: 
		case T_SHORT: 
		case T_INT:
			text << arg->val.intarg;
			break;
		case T_U_CHAR:
		case T_U_SHORT:
		case T_U_INT:
			text << arg->val.uintarg;
			break;
		case T_LONG:
			text << arg->val.longarg;
			break;
		case T_U_LONG:
			text << arg->val.ulongarg;
			break;
		case T_LLONG:
			text << arg->val.longlongarg;
			break;
		case T_U_LLONG:
			text << arg->val.ulonglongarg;
			break;
		case TP_VOID:
			text << arg->val.pvoidarg;
			break;
		case T_PTRINT:
			text << arg->val.ptrdiffarg;
			break;
		case T_SIZEINT:
			text << arg->val.sizearg;
			break;
#ifdef HAVE_SSIZE_T
		case T_SSIZEINT:
			text << arg->val.ssizearg;
			break;
#endif
		case T_MAXINT:
			text << arg->val.intmaxarg;
			break;
		case T_MAXUINT:
			text << arg->val.uintmaxarg;
			break;
#ifdef FLOATING_POINT
		case T_DOUBLE:
			text << arg->val.doublearg;
			break;
		case T_LONG_DOUBLE:
			text << arg->val.longdoublearg;
			break;
#endif
		case T_UNUSED:
			assert(false);
			/*FALLTHRU*/
		default:
			assert(false);
			break;
		}

		int length = (int)(STELL() - start);

		if (((PLUSSIGN|SPACESIGN) & flags) == SPACESIGN) { //float and numeric 
			for (char *cursor = start, *cend = start + length; cursor < cend; ++cursor) {
				if ('+' == *cursor) {
					*cursor = ' ';
					break;
				} else if (' ' != *cursor) {
					break;
				}
			}
		}

		if (0 == (FPT & flags)) { //numeric
			if ('p' == spec) {
				/*
				 *  0X prefix.
				 */
				if (ALT & flags) {
					text << "  ";
					memmove(start + 2, start, length);
					start[0] = '0';
					start[1] = 'X';
					length += 2;
				}
			} else if (precision >= 0) {
				/*
				 *  ``The result of converting a zero value with an
				 *    explicit precision of zero is no characters.'' -- ANSI X3J11
				 */
				if (0 == precision && length && '0' == *start) {
					if (1 == length && 0 == width) {
						buffer[0] = 0;
						return 0;
					}
					*start = ' ';
				}
			}

			/*
			 *  numeric left/right space fill.
			 */
			if (width > 0 && length < width) {
				const int diff = width - length;
				for (int spacepad = diff; spacepad--;)
					text << ' '; //space-pad
				if (0 == (LADJUST & flags)) { //right adjust
					if ((STELL() - start) == width) { //overflow?
						memmove(start + diff, start, length);
						memset(start, ' ', (diff > length ? length : diff));
					}
				}
			}
		}
	}

#if defined(__WATCOMC__) //strstream
	int ret = (int)text.pcount();
#else
	int ret = (int)text.tellp();
#endif
	buffer[ret] = 0;
	assert(ret == (int)strlen(buffer));
	return ret;
}


/*
 *  Find all arguments when a positional parameter is encountered.
 *  Returns fields and an args table, indexed by argument number, of pointers to each arguments.
 */

int
NSFormat::parse_format(const char *fmt, va_list ap, struct Field *fields, unsigned fieldno, struct Argument *args, unsigned argno)
{
	int ch; 			/* character from fmt */
	int n, n2;			/* handy integer (short term usage) */
	const char *cp; 		/* handy char pointer (short term usage) */
	const char *pp; 		/* position index */
	unsigned short flags;		/* flags as above */
	int ioflags;			/* ostream flags */
	int nextarg = 1;		/* 1-based argument index */
	int maxarg = 1; 		/* upper bound */

	/*
	 * Add an argument type to the table, expanding if necessary.
	 */

#define ADDTYPE(type) \
	f->value_arg = nextarg, args[nextarg++].argtype = type

#define __ADDTYPE(type) \
	args[nextarg++].argtype = type

#define ADDSARG() \
	(f->value_arg = nextarg, \
	    ((flags&MAXINT)   ? __ADDTYPE(T_MAXINT) : \
	    ((flags&PTRINT)   ? __ADDTYPE(T_PTRINT) : \
	    ((flags&SIZEINT)  ? __ADDTYPE(T_SSIZEINT) : \
	    ((flags&LLONGINT) ? __ADDTYPE(T_LLONG) : \
	    ((flags&LONGINT)  ? __ADDTYPE(T_LONG) : \
	    ((flags&SHORTINT) ? __ADDTYPE(T_SHORT) : \
	    ((flags&CHARINT)  ? __ADDTYPE(T_CHAR) : __ADDTYPE(T_INT)))))))))

#define ADDUARG() \
	(f->value_arg = nextarg, \
	    ((flags&MAXINT)   ? __ADDTYPE(T_MAXUINT) : \
	    ((flags&PTRINT)   ? __ADDTYPE(T_PTRINT) : \
	    ((flags&SIZEINT)  ? __ADDTYPE(T_SIZEINT) : \
	    ((flags&LLONGINT) ? __ADDTYPE(T_U_LLONG) : \
	    ((flags&LONGINT)  ? __ADDTYPE(T_U_LONG) : \
	    ((flags&SHORTINT) ? __ADDTYPE(T_U_SHORT) : \
	    ((flags&CHARINT)  ? __ADDTYPE(T_U_CHAR) : __ADDTYPE(T_U_INT)))))))))

	/*
	 * Add * arguments to the type array.
	 */
#define APPEND_DIGIT(val, dig) do { \
	if ((val) > INT_MAX / 10) \
		goto overflow; \
	(val) *= 10; \
	if ((val) > INT_MAX - to_digit((dig))) \
		goto overflow; \
	(val) += to_digit((dig)); \
} while (0)

#define ADDASTER(__field) \
	n2 = 0; \
	pp = cp = fmt; \
	while (is_digit(*cp)) { \
		APPEND_DIGIT(n2, *cp); \
		++cp; \
	} \
	if (*cp == '$') { \
		int hold = nextarg; \
		nextarg = n2; \
		if (nextarg >= (int)argno) \
			goto done; \
		f->__field = n2; \
		__ADDTYPE(T_INT); \
		nextarg = hold; \
		fmt = ++cp; \
	} else { \
		f->__field = nextarg; \
		__ADDTYPE(T_INT); \
	} \
	if (nextarg >= (int)argno) \
		goto done;

	/*
	 * Scan the format for conversions (`%' character).
	 */
	struct Field *f = fields;
	for (;*fmt && nextarg < (int)argno;) {

		if (fieldno-- <= 0) {
			goto overflow;
		}
		memset(f, 0, sizeof(*f));
		f->start = fmt;

		for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; ++fmt)
			continue;
		if (ch == '\0') {
			f->fmt = NULL;
			f->end = fmt;
			++f;
			goto done;
		}

		f->precision = -1;
		f->value_arg = nextarg;
		f->fmt = fmt;
		++fmt;		/* skip over '%' */

		flags = 0;
		ioflags = 0;

rflag:		ch = *fmt++;
reswitch:	switch (ch) {
		case ' ':
			if (0 == (flags & PLUSSIGN)) //'+' priority
				flags |= SPACESIGN;
			goto rflag;
		case '#':
			flags |= ALT;
			goto rflag;
		case '\'':	/* grouping not implemented */
			goto rflag;
		case '*':	/* width */
			ADDASTER(width_arg);
			goto rflag;
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			flags &= ~SPACESIGN; //'+' priority
			flags |= PLUSSIGN;
			goto rflag;
		case '.':	/* precision */
			if ((ch = *fmt++) == '*') {
				ADDASTER(precision_arg);
				goto rflag;
			}
			n = 0;
			while (is_digit(ch)) {
				APPEND_DIGIT(n, ch);
				ch = *fmt++;
			}
			f->precision = n;
			goto reswitch;
		case '0':
			flags |= ZEROPAD;
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				APPEND_DIGIT(n ,ch);
				ch = *fmt++;
			} while (is_digit(ch));
			if (ch == '$') {
				nextarg = n;
				f->width_arg = n;
				goto rflag;
			}
			f->width = n;
			goto reswitch;

		//  Length modifiers:
		//
		//  h		Length modifier specifying that a following d, o, u, x, or X conversion specifier applies to a short or unsigned short argument.
		//  hh		Length modifier specifying that a following d, o, u, x, or X conversion specifier applies to a signed char or unsigned char argument.
		//  j		Length modifier specifying that a following d, o, u, x, or X conversion specifier applies to a intmax_t or uintmax_t argument.
		//  l		Length modifier specifying that a following d, o, u, x, or X conversion specifier applies to a long or unsigned long argument.
		//  ll, q	Length modifiers specifying that a following d, o, u, x, or X conversion specifier applies to a long long or unsigned long long argument.
		//  L		Length modifier specifying that a following a, A, e, E, f, F, g, or G conversion specifier applies to a long double argument.
		//  t		Length modifier specifying that a following d, o, u, x, or X conversion specifier applies to a ptrdiff_t.
		//  z		Length modifier specifying that a following d, o, u, x, or X conversion specifier applies to a size_t.
		//
		case 'h':
			if (*fmt == 'h') {
				fmt++;
				flags |= CHARINT;
			} else {
				flags |= SHORTINT;
			}
			goto rflag;
		case 'j':
			flags |= MAXINT;
			goto rflag;
		case 'l':
			if (*fmt == 'l') { // see 'q'
				fmt++;
				flags |= LLONGINT;
			} else {
				flags |= LONGINT;
			}
			goto rflag;
		case 'q':
			flags |= LLONGINT;
			goto rflag;
#ifdef FLOATING_POINT
		case 'L':
			flags |= LONGDBL;
			goto rflag;
#endif
		case 't':
			flags |= PTRINT;
			goto rflag;
		case 'z':
			flags |= SIZEINT;
			goto rflag;
		case 'I': //I32|I64, MSVC (d, i, o, u, x, or X)
			if (is_digit(*fmt)) {
				const char *t_fmt = fmt;
				int isz = 0;
				while (is_digit(*fmt)) {
					APPEND_DIGIT(isz, *fmt);
					++fmt;
				}
				if (64 == isz) {
					flags |= ISZ|SIZE64;
					goto rflag;
				} else if (32 == isz) {
					flags |= ISZ|SIZE32;
					goto rflag;
				}
				fmt = t_fmt;
			} //otherwise ignore.
			goto unknown;

		//  Format specifiers:
		//
		//  @		Object; NSObject().
		//  %C		16-bit UTF-16 code unit (unichar).
		//  %c		8-bit unsigned character (unsigned char).
		//  %d,%D,%i	Signed 32-bit integer (int).
		//  %a		64-bit floating-point number (double), printed in scientific notation with a leading 0x and one hexadecimal digit before the decimal point using a lowercase p to introduce the exponent.
		//  %A		64-bit floating-point number (double), printed in scientific notation with a leading 0X and one hexadecimal digit before the decimal point using a uppercase P to introduce the exponent.
		//  %e		64-bit floating-point number (double), printed in scientific notation using a lowercase e to introduce the exponent.
		//  %E		64-bit floating-point number (double), printed in scientific notation using an uppercase E to introduce the exponent.
		//  %f		64-bit floating-point number (double).
		//  %F		64-bit floating-point number (double), printed in decimal notation.
		//  %g		64-bit floating-point number (double), printed in the style of %e if the exponent is less than –4 or greater than or equal to the precision, in the style of %f otherwise.
		//  %G		64-bit floating-point number (double), printed in the style of %E if the exponent is less than –4 or greater than or equal to the precision, in the style of %f otherwise.
		//  %o, %O	Unsigned 32-bit integer (unsigned int), printed in octal.
		//  %p		Void pointer (void *), printed in hexadecimal with the digits 0–9 and lowercase a–f, with a leading 0x.
		//  %S		Null-terminated array of 16-bit UTF-16 code units.
		//  %s		Null-terminated array of 8-bit unsigned characters.
		//  %u, %U	Unsigned 32-bit integer (unsigned int).
		//  %X		Unsigned 32-bit integer (unsigned int), printed in hexadecimal using the digits 0–9 and uppercase A–F.
		//  %x		Unsigned 32-bit integer (unsigned int), printed in hexadecimal using the digits 0–9 and lowercase a–f.
		//  %%		'%' character.
		//
		case '@':
			ADDTYPE(TP_OBJECT);
			break;
		case 'C':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'c':
#ifdef PRINTF_WIDE_CHAR
			if (flags & LONGINT)
				ADDTYPE(T_WINT);
			else
#endif
				ADDTYPE(T_INT);
			flags &= ~(PLUSSIGN | SPACESIGN);
			break;
		case 'D':
			if (ISZ & flags) goto unknown;
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			ADDSARG();
			ioflags |= std::ios::dec;
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			ADDUARG();
			ioflags |= std::ios::dec;
			flags &= ~(PLUSSIGN | SPACESIGN);
			break;
#ifdef FLOATING_POINT
		case 'A':
			ioflags |= std::ios::uppercase;
			/*FALLTHROUGH*/
		case 'a':
			ioflags |= std::ios::fixed | std::ios::scientific; //hexfloat
			goto floating_point;
		case 'E':
			ioflags |= std::ios::uppercase;
			/*FALLTHROUGH*/
		case 'e':
			ioflags |= std::ios::scientific;
			goto floating_point;
		case 'F':
			ioflags |= std::ios::uppercase;
			/*FALLTHROUGH*/
		case 'f':
			ioflags |= std::ios::fixed;
			goto floating_point;
		case 'G':
			ioflags |= std::ios::uppercase;
			/*FALLTHROUGH*/
		case 'g':
floating_point: 	flags |= FPT;
			if (flags & LONGDBL)
				ADDTYPE(T_LONG_DOUBLE);
			else
				ADDTYPE(T_DOUBLE);
			break;
#endif /* FLOATING_POINT */
		case 'n':
#ifndef NO_PRINTF_PERCENT_N
			if (ioflags & LLONGINT)
				ADDTYPE(TP_LLONG);
			else if (ioflags & LONGINT)
				ADDTYPE(TP_LONG);
			else if (ioflags & SHORTINT)
				ADDTYPE(TP_SHORT);
			else if (ioflags & PTRINT)
				ADDTYPE(TP_PTRINT);
			else if (ioflags & SIZEINT)
				ADDTYPE(TP_SSIZEINT);
			else if (ioflags & MAXINT)
				ADDTYPE(TP_MAXINT);
			else
				ADDTYPE(TP_INT);
			goto next; /* no output */
#else
			goto invalid;
#endif /* NO_PRINTF_PERCENT_N */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			ioflags |= std::ios::oct;
			ADDUARG();
			break;
		case 'S':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 's':
#ifdef PRINTF_WIDE_CHAR
			if (flags & LONGINT)
				ADDTYPE(TP_WCHAR);
			else
#endif
				ADDTYPE(TP_CHAR);
			flags &= ~(PLUSSIGN | SPACESIGN | ZEROPAD);
			break;
		case 'X':
			ioflags |= std::ios::uppercase;
		case 'x':
			ADDUARG();
			ioflags |= std::ios::hex;
			flags &= ~(PLUSSIGN | SPACESIGN);
			break;
		case 'p':
			ADDTYPE(TP_VOID);
			ioflags |= std::ios::hex;
			f->precision = -1;
			flags &= ~(PLUSSIGN | SPACESIGN);
			break;
		case '%':
			if ((f->fmt + 2) != fmt) {
				--fmt; /* unterminated specification */
			}
				/*FALLTHROUGH*/
		default:	
unknown:		if (ch == '\0') --fmt; /* "%?" prints ?, unless ? is NUL */
			ch = '%';
			break;
		}

#ifndef NO_PRINTF_PERCENT_N
next:;
#endif
		if (nextarg > maxarg) {
			maxarg = nextarg;
			if (maxarg >= (int)argno) {
				break;
			}
		}
		f->spec = ch;
		f->flags = flags;
		f->ioflags = ioflags;
		f->end = fmt;
		++f;
	}

done:	if (maxarg >= (int)argno) {
		errno = E2BIG;
		return (-1);
	}

	/*
	 * Build the argument table.
	 */
	for (n = 1; n < maxarg; ++n) {
		Argument *arg = args + n;
		switch (arg->argtype) {
		case T_UNUSED:
			assert(0);
			/*FALLTHRU*/
		case T_CHAR:
			arg->val.intarg = (char) va_arg(ap, int);
			break;
		case T_SHORT:
			arg->val.intarg = (short) va_arg(ap, int);
			break;
		case T_INT:
			arg->val.intarg = va_arg(ap, int);
			break;
		case T_U_CHAR:
			arg->val.uintarg = (unsigned char) va_arg(ap, unsigned int);
			break;
		case T_U_SHORT:
			arg->val.uintarg = (unsigned short) va_arg(ap, unsigned int);
			break;
		case T_U_INT:
			arg->val.uintarg = va_arg(ap, unsigned int);
			break;
		case T_LONG:
			arg->val.longarg = va_arg(ap, long);
			break;
		case T_U_LONG:
			arg->val.ulongarg = va_arg(ap, unsigned long);
			break;
		case T_LLONG:
			arg->val.longlongarg = va_arg(ap, long long);
			break;
		case T_U_LLONG:
			arg->val.ulonglongarg = va_arg(ap, unsigned long long);
			break;
		case TP_CHAR:
			arg->val.pchararg = va_arg(ap, const char *);
			break;
		case TP_VOID:
			arg->val.pvoidarg = va_arg(ap, void *);
			break;
		case T_PTRINT:
			arg->val.ptrdiffarg = va_arg(ap, ptrdiff_t);
			break;
		case T_SIZEINT:
			arg->val.sizearg = va_arg(ap, size_t);
			break;
#ifdef HAVE_SSIZE_T
		case T_SSIZEINT:
			arg->val.ssizearg = va_arg(ap, ssize_t);
			break;
#endif
		case T_MAXINT:
			arg->val.intmaxarg = va_arg(ap, intmax_t);
			break;
		case T_MAXUINT:
			arg->val.uintmaxarg = va_arg(ap, uintmax_t);
			break;
#ifdef FLOATING_POINT
		case T_DOUBLE:
			arg->val.doublearg = va_arg(ap, double);
			break;
		case T_LONG_DOUBLE:
			arg->val.longdoublearg = va_arg(ap, long double);
			break;
#endif
#ifdef PRINTF_WIDE_CHAR
		case T_WINT:
			arg->val.wintarg = va_arg(ap, wint_t);
			break;
		case TP_WCHAR:
			arg->val.pwchararg = va_arg(ap, const wchar_t *);
			break;
#endif
#ifndef NO_PRINTF_PERCENT_N
		case TP_SHORT:
			arg->val.pshortarg = va_arg(ap, short *);
			break;
		case TP_INT:
			arg->val.pintarg = va_arg(ap, int *);
			break;
		case TP_PTRINT:
			arg->val.pptrdiffarg = va_arg(ap, ptrdiff_t *);
			break;
		case TP_LONG:
			arg->val.plongarg = va_arg(ap, long *);
			break;
		case TP_LLONG:
			arg->val.plonglongarg = va_arg(ap, long long *);
			break;
#ifdef HAVE_SSIZE_T
		case TP_SSIZEINT:
			arg->val.pssizearg = va_arg(ap, ssize_t *);
			break;
#endif
		case TP_MAXINT:
			arg->val.pintmaxarg = va_arg(ap, intmax_t *);
			break;
#endif	//NO_PRINTF_PERCENT_N
		case TP_OBJECT:
			arg->val.pobjectarg = va_arg(ap, void *);
			break;
		}
	}
	return (int)(f - fields);

invalid:
	errno = EINVAL;
	return (-1);

overflow:
#if !defined(EOVERFLOW)
	errno = ERANGE;
#else
	errno = EOVERFLOW;
#endif
	return (-1);
}
