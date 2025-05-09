#ifndef UPDATETOOLSHIM_H_INCLUDED
#define UPDATETOOLSHIM_H_INCLUDED
// $Id: updatetoolshim.h,v 1.4 2025/04/22 17:23:55 cvsuser Exp $
//
//  AutoUpdater: update-tool
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

#if defined(__cplusplus)
extern "C" {
#endif

struct UpdateToolArgs {
    const char *progname;       // update-tool program name; default argv.
    const char *progtitle;      // program usage title line.

    const char *appname;        // application name; registry key.
    const char *version;        // application version; x.x.x.x

    const char *hosturl;        // manifest URL.
    const char *hosturlalt;     // alternative manifest URL.

    const char *publickey;      // Ed2559 public key.
    unsigned keyversion;        // key version
};

extern int UpdateToolShim(int argc, char *argv[], const struct UpdateToolArgs *args);

#if defined(__cplusplus)
}
#endif

#endif //UPDATETOOLSHIM_H_INCLUDED

//end
