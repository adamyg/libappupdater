#ifndef BASE64_H_INCLUDED
#define BASE64_H_INCLUDED
//  $Id: Base64.h,v 1.1 2025/04/21 13:58:28 cvsuser Exp $
//
//  AutoUpdater: base64 utils
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

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include <string>

namespace Updater {

class Base64 {
public:
    static size_t
    encode_dstlength(size_t length)
    {
        return (length * 4 / 3 + 4) + 1; /* 3-byte blocks to 4-byte */
    }

    static int
    encode(const void* src, size_t length, char* dst, size_t dstlength)
    {
        const char *encodetable = b64encodetable();
        const uint8_t *in = (uint8_t*)src, * end = in + length;
        const size_t osize = Base64::encode_dstlength(length);
        int count = 0;

        assert(dstlength >= osize);
        if (dstlength < osize) {
            return -1;
        }

        while ((end - in) >= 3) {
            uint32_t triple = (in[0] << 0x10) + (in[1] << 0x08) + in[2];

            in += 3;
            count += 4;

            *dst++ = encodetable[(triple >> 3 * 6) & 0x3f];
            *dst++ = encodetable[(triple >> 2 * 6) & 0x3f];
            *dst++ = encodetable[(triple >> 1 * 6) & 0x3f];
            *dst++ = encodetable[(triple >> 0 * 6) & 0x3f];
        }

        const size_t remainder = end - in;
        if (remainder) {
            *dst++ = encodetable[in[0] >> 2];
            if (remainder == 1) {
                *dst++ = encodetable[(in[0] & 0x03) << 4];
                *dst++ = '=';
            } else {
                *dst++ = encodetable[((in[0] & 0x03) << 4) | (in[1] >> 4)];
                *dst++ = encodetable[(in[1] & 0x0f) << 2];
            }
            *dst++ = '=';
            count += 4;
        }

        *dst = 0;
        return (count + 1);
    }

    static std::string
    encode_to_string(const void *src, size_t length)
    {
        std::string result;
        result.resize(encode_dstlength(length));
        encode(src, length, const_cast<char *>(result.data()), result.size());
        return result;
    }

    static size_t
    decode_dstlength(const void *src, size_t length)
    {
        size_t dstlength = length / 4 * 3;

        const uint8_t *end = static_cast<const uint8_t *>(src) + length;
        if (end[-1] == '=') --dstlength;
        if (end[-2] == '=') --dstlength;
        return dstlength;
    }

    static int
    decode(const void *src, size_t length, uint8_t *dst, size_t dstlength)
    {
        static const uint8_t *decodetable = b64decodetable();

        if (NULL == decodetable || (length % 4 != 0)) {
            return -1;
        }

        if (length < 4) {
            return 0;
        }

        size_t needed = decode_dstlength(src, length);
        if (dstlength < needed) {
            return -1;
        }

        const uint8_t *in = static_cast<const uint8_t *>(src);
        for (uint8_t *eod = dst + needed; dst != eod;) {

            const uint32_t a = decodetable[*in++];
            const uint32_t b = decodetable[*in++];
            const uint32_t c = (*in == '=' ? 0 : decodetable[*in++]);
            const uint32_t d = (*in == '=' ? 0 : decodetable[*in++]);

            const uint32_t triple =
                (a << 3 * 6) + (b << 2 * 6)
                + (c << 1 * 6) + (d << 0 * 6);

            if (dst != eod) *dst++ = (triple >> 2 * 8) & 0xff;
            if (dst != eod) *dst++ = (triple >> 1 * 8) & 0xff;
            if (dst != eod) *dst++ = (triple >> 0 * 8) & 0xff;
        }

        return (int)needed;
    }

    static std::string
    decode_to_string(const void *src, size_t length)
    {
        std::string result;
        if (length >= 4) {
            result.resize(decode_dstlength(src, length));
            decode(src, length, (uint8_t *)(result.data()), result.size());
        }
        return result;
    }

private:
    static const char *
    b64encodetable()
    {
        return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    }

    static const uint8_t *
    b64decodetable()
    {
        const char *encodetable = b64encodetable();
        uint8_t *decodetable;

        decodetable = static_cast<uint8_t*>(calloc(1, 256));
        if (NULL != decodetable) {
            for (uint8_t n = 0; n < 64; ++n) {
                decodetable[(uint8_t)encodetable[n]] = n;
            }
        }
        return decodetable;
    }
};

} // namespace Updater

#endif //BASE64_H_INCLUDED

//end