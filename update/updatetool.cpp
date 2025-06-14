// $Id: updatetool.cpp,v 1.4 2025/04/22 17:23:55 cvsuser Exp $
//
//  AutoUpdater: update-tool - example
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

#include "updatetoolshim.h"

//  Function: Main
//      Application entry.
//
//  Returns:
//      0  - No check performed.
//      1  - Up-to-date.
//      2  - Installed.
//      3  - Update available.
//      99 - Usage
//

int
main(int argc, char *argv[])
{
    struct UpdateToolArgs args = {0};

    args.progname = "updatetool";

    args.appname = "libautoupdate";
//  args.version = "0.0.0.0";
 
//  args.hosturl = hosturl1;
//  args.hosturlalt = hosturl2;
//  args.privatekey = private_key_base64;
//  args.keyversion = key_version;

    return UpdateToolShim(argc, argv, &args);
}

//end
