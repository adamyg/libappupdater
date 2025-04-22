#ifndef FORMAT_H_INCLUDED
#define FORMAT_H_INCLUDED
//  $Id: Format.h,v 1.1 2025/04/21 13:58:28 cvsuser Exp $
//
//  AutoUpdater: format utils
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2024 - 2025, Adam Young
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
             
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include <string>

namespace Updater {

#ifndef VA_COPY
# if defined(HAVE_VA_COPY) || defined(va_copy) /* ISO C99 and later */
#define VA_COPY(__dst, __src)   va_copy(__dst, __src)
# elif defined(HAVE___VA_COPY) || defined(__va_copy) /* gnu */
#define VA_COPY(__dst, __src)   __va_copy(__dst, __src)
# elif defined(__WATCOMC__) /* Older Watcom implementations */
#define VA_COPY(__dst, __src)   memcpy((__dst), (__src), sizeof (va_list))
# else
#define VA_COPY(__dst, __src)   (__dst) = (__src)
# endif
#define VA_COPY_LOCAL 1
#endif  /*VA_COPY*/

inline std::string 
formatv(const char *message, va_list ap)
{
    std::string msg;

    if (!message || !*message) {
        return msg;
    }

    va_list tap;

    VA_COPY(tap, ap);
    const size_t osize = vsnprintf(NULL, 0, message, tap);
    msg.resize(osize);

    const size_t size = vsprintf(const_cast<char *>(msg.data()), message, ap);
    assert(size == osize);
    va_end(tap);

    return msg;
}


inline std::string
format(const char *message, ...)
{
    va_list ap;
    va_start(ap, message);
    return formatv(message, ap);
}

#ifdef VA_COPY_LOCAL
#undef VA_COPY_LOCAL
#undef VA_COPY
#endif

} // namespace Updater

#endif //FORMAT_H_INCLUDED

//end

