#ifndef AUTOERROR_H_INCLUDED
#define AUTOERROR_H_INCLUDED
//  $Id: AutoError.h,v 1.12 2025/02/21 19:03:23 cvsuser Exp $
//
//  AutoUpdater: exception interface.
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

#include "common.h"

#include <stdexcept>
#include <string>

namespace Updater {

class SysException : public std::runtime_error {
public:
    SysException(DWORD dwStatus, const char *message = NULL);
    SysException(const std::string &message);
    SysException(const char *message = NULL);
};


class AppException : public std::runtime_error {
public:
    AppException(const std::string &message);
    AppException(const char *message);
};

}   // namespace Updater

#endif //AUTOERROR_H_INCLUDED
