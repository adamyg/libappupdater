#ifndef HEX_H_INCLUDED
#define HEX_H_INCLUDED
//  $Id: Hex.h,v 1.1 2025/04/21 13:58:28 cvsuser Exp $
//
//  AutoUpdater: hex utils
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

class Hex {
public:
    static std::string
    to_string(const void *src, size_t length)
    {
        const char digits[] = "0123456789abcdef";
        std::string result;
        
        result.resize(length * 2);

        const uint8_t *bin = static_cast<const uint8_t *>(src);
        char *cursor = const_cast<char *>(result.data());

        for (size_t i = 0; i < length; i++) {
            *cursor++ += digits[bin[i] >> 4];
            *cursor++ += digits[bin[i] & 0xf];
        }
        return result;
    }
};

} // namespace Updater

#endif //HEX_H_INCLUDED

//end
