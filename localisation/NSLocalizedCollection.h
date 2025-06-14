#pragma once
#ifndef NSLocalizedCollection_h_included
#define NSLocalizedCollection_h_included
//  $Id: NSLocalizedCollection.h,v 1.4 2023/10/17 12:33:56 cvsuser Exp $
//
//  NSLocalization - Collection
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2021 - 2025, Adam Young
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

#include "../libautoupdater.h"

class NSLocalizedCollectionImpl;

class LIBAUTOUPDATER_LINKAGE NSLocalizedCollection {
    NSLocalizedCollection(const NSLocalizedCollection &);
    NSLocalizedCollection& operator=(const NSLocalizedCollection &);

public:
    class ILogger {
    public:
        virtual void error(unsigned line_number, const char *msg) = 0;
    };

public:
    NSLocalizedCollection();
    virtual ~NSLocalizedCollection();

    bool load(const char *filename, ILogger *report = 0);
    bool load(const char *resourceName, const char *tableName, ILogger *report = 0);
    const char *lookup(const char *key, const char *comment);

private:
    NSLocalizedCollectionImpl *impl_;
};

#endif /*NSLocalizedCollection_h_included*/
