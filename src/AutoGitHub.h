#ifndef AUTOGITHUB_H_INCLUDED
#define AUTOGITHUB_H_INCLUDED
//  $Id: AutoGitHub.h,v 1.1 2025/04/16 11:33:48 cvsuser Exp $
//
//  AutoUpdater: github latest interface.
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

class Download;

class GitHub {
public:
    GitHub();
    virtual ~GitHub();

    // Example:
    //  https://api.github.com/repos/adamyg/mcwin32
    //
    bool IsEndpoint(const std::string &url) const;

    // Example:
    //  https://api.github.com/repos/adamyg/mcwin32/releases/latest
    // 
    // Returns:
    //  4.8.33.232
    //
    bool GetLatestRelease(const std::string& url, Download &downloader, int flags, std::string &result);
};

}   // namespace Updater

#endif //AUTOGITHUB_H_INCLUDED
