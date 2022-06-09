#pragma once
//  $Id: NSLocalizedCollectionImpl.h,v 1.6 2022/06/09 08:46:30 cvsuser Exp $
//
//  NSLocalization - Collection implementation
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2021 - 2022, Adam Young
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

#include <string>
#include <map>

#if !defined(WINDOWS_MEAN_AND_LEAN)
#define WINDOWS_MEAN_AND_LEAN
#endif
#if defined(__WATCOMC__)
#define NOMINMAX
#endif
#include <Windows.h>

#include "NSLocalizedCollection.h"

class NSLocalizedCollectionImpl {
public:
    struct Exporter {
        Exporter(const char *output) {
            output_.open(output, std::ios::out|std::ios::binary);
            if (is_open()) output_.write("NSMO", 4);
        }

        bool is_open() const {
            return output_.is_open();
        }

        bool operator()(const std::string &key, const std::string &val) {
            output_ << (unsigned char)(key.length() >> 8) << (unsigned char)(key.length());
            output_ << (unsigned char)(val.length() >> 8) << (unsigned char)(val.length());
            output_.write(key.c_str(), key.length() + 1 /*nul*/);
            output_.write(val.c_str(), val.length() + 1 /*nul*/);
            return true;
        }

        std::fstream output_;
    };

    class Filename : public NSLocalizedCollection::ILogger {
    public:
        Filename(const char *filename);
        virtual void error(unsigned line_number, const char *msg);
        const char *filename_;
    };

    class Resource : public NSLocalizedCollection::ILogger {
    public:
        Resource(const char *resourceName, const char *tableName);
        virtual void error(unsigned line_number, const char *msg);
        const char *resourceName_, *tableName_;
    };

public:
    bool load(NSLocalizedCollection::ILogger &ilogger, const char *filename);
    bool load(NSLocalizedCollection::ILogger &ilogger, const char *resourceName, const char *tableName);

    const char *lookup(const char *key, const char *comment);

    template <typename Function>
    void for_each(Function &function)
    {
        Collection_t::const_iterator begin(strings_.begin()), end(strings_.end());
        for (; begin != end; ++begin) {
            if (! function(begin->first, begin->second)) {
                break;
            }
        }
    }

private:
    struct Logger {
        Logger(NSLocalizedCollection::ILogger &ilogger);
        void error(const char *fmt ...);
        NSLocalizedCollection::ILogger &ilogger_;
        unsigned line_number;
    };

    bool load_object(Logger &logger, const void *it, const void *end, const char *name);
    bool load_strings(Logger &logger, const char *filename);
    bool load_strings(Logger &logger, const char *name, const char *type, void *module = 0);
    bool load_strings(Logger &logger, const std::string &buffer, const char *name);
    bool load_strings(Logger &logger, const char *it, const char *end, const char *name);
    bool load_po(Logger &logger, const char *it, const char *end, const char *name);

private:
    static const char *get_str(Logger &logger, const char *it, const char *end, std::string &result, const char *what = 0);
    static const char *get_hex(Logger &logger, const char *it, const char *end, unsigned limit, std::string &result);
    static const char *get_tok(Logger &logger, const char *it, const char *end, std::string &result);
    static const char *get_dem(Logger &logger, const char *it, const char *end, const char tok);

    static HINSTANCE ModuleHandle();
    static std::wstring to_wstring(const std::string &str);

private:
    typedef std::map<std::string, std::string> Collection_t;
    std::map<std::string, std::string> strings_;
};
