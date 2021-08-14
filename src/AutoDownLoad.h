#ifndef AUTODOWNLOAD_H_INCLUDED
#define AUTODOWNLOAD_H_INCLUDED
//  $Id: AutoDownLoad.h,v 1.10 2021/08/14 05:23:47 cvsuser Exp $
//
//  AutoUpdater: download/inet functionality.
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

namespace Updater {

struct IDownloadSink {
    virtual void set_size(size_t size) = 0;
    virtual bool open() = 0;
    virtual void append(const void *data, size_t len) = 0;
};


class StringDownloadSink : public IDownloadSink {
    StringDownloadSink(const StringDownloadSink &rsh);
    StringDownloadSink& operator=(const StringDownloadSink &rsh);

public:
    StringDownloadSink() {
    }

    virtual void set_size(size_t size) {
        data.reserve(size);
    }

    virtual bool open() {
        return true;
    }

    virtual void append(const void *data, size_t len) {
        this->data.append(reinterpret_cast<const char*>(data), len);
    }

    std::string data;                       // payload.
};


class DownloadContext;
class Download {
public:
    enum Flags {
        NOCACHED = 1                        // ignore cache.
    };

public:
    Download();
    ~Download();

    bool get(const std::string &url, IDownloadSink &sink, unsigned flags = 0);
    bool get(const std::string &url, const char *localfile, unsigned flags = 0);
    bool completion(bool pump = true);
    void cancel();

private:
    friend class DownloadContext;
    DownloadContext *context_;              // download context.
    int connect_timeout_;
    int response_timeout_;
    bool enable_login_;
};

}   // namespace Updater

#endif  //AUTODOWNLOAD_H_INCLUDED
