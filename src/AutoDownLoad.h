#ifndef AUTODOWNLOAD_H_INCLUDED
#define AUTODOWNLOAD_H_INCLUDED
//  $Id: AutoDownLoad.h,v 1.13 2022/06/09 08:46:30 cvsuser Exp $
//
//  AutoUpdater: download/inet functionality.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2022, Adam Young
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
    virtual void close() = 0;
};


class StringDownloadSink : public IDownloadSink {
    StringDownloadSink(const StringDownloadSink &rsh);
    StringDownloadSink& operator=(const StringDownloadSink &rsh);

public:
    StringDownloadSink() : data_(&t_destination) {
    }

    StringDownloadSink(std::string *destination) : data_(destination) {
    }

    virtual ~StringDownloadSink() {
    }

    virtual void set_size(size_t size) {
        data_->reserve(size);
    }

    virtual bool open() {
        return true;
    }

    virtual void append(const void *data, size_t len) {
        data_->append(reinterpret_cast<const char*>(data), len);
    }

    virtual void close() {
    }

    const std::string &data() {
        return *data_;
    }

private:
    std::string t_destination;                  // local payload.
    std::string *data_;
};


struct FileDownloadSink : public IDownloadSink {
    FileDownloadSink(const FileDownloadSink &rsh);
    FileDownloadSink& operator=(const FileDownloadSink &rsh);

public:
    FileDownloadSink(const char *filename = NULL);
    virtual ~FileDownloadSink();

    virtual bool open();
    virtual void set_size(size_t size);
    virtual void append(const void *data, size_t len);
    virtual void close();

    std::string filename_;
    size_t filesize_;
    HANDLE handle_;
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
