//  $Id: AutoError.cpp,v 1.10 2021/08/14 05:23:47 cvsuser Exp $
//
//  AutoUpdater: exception interface.
//
//  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2021 Adam Young
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

#include <string>

#include "AutoError.h"

namespace Updater {

namespace {

//  ---------------------------------------------------------------------------
//  System exceptions

std::string
SysErrorMessage(const char *message, DWORD err)
{
    std::string msg;
    LPSTR buf;

    if (message && *message) {
        msg = message, msg += ":\n\n";
    }
    if (::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                0, err, 0, (LPSTR)&buf, 0, NULL)) {
        msg += buf;
        LocalFree(buf);
    }
    return msg;
}
}   // anonymous namespace


SysException::SysException(DWORD dwStatus, const char *message)
    : std::runtime_error(SysErrorMessage(message, dwStatus))
{
}


SysException::SysException(const std::string &message)
    : std::runtime_error(SysErrorMessage(message.c_str(), GetLastError()))
{
}


SysException::SysException(const char *message)
    : std::runtime_error(SysErrorMessage(message, GetLastError()))
{
}


//  ---------------------------------------------------------------------------
//  Application exceptions

AppException::AppException(const std::string &message)
    : std::runtime_error(message.c_str())
{
}


AppException::AppException(const char *message)
    : std::runtime_error(message)
{
}

}   // namespace Updater
