//  $Id: NSLocalizedCollection.cpp,v 1.4 2021/08/26 12:17:38 cvsuser Exp $
//
//  NSLocalization - Collection
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
#include <cassert>
#include <fstream>
#include <iostream>
#include <cctype>
#include <map>

#include "nslocal.h"

#include "NSLocalizedString.h"
#include "NSLocalizedCollection.h"
#include "NSLocalizedCollectionImpl.h"


/////////////////////////////////////////////////////////////////////////////////////////
//  NSLocalizedCollection
// 

NSLocalizedCollection::NSLocalizedCollection()
    : impl_(new NSLocalizedCollectionImpl())
{
}


NSLocalizedCollection::~NSLocalizedCollection()
{
    delete impl_;
}


bool
NSLocalizedCollection::load(const char *filename, ILogger *logger /*=0*/)
{
    if (logger) {
        return impl_->load(*logger, filename);
    }

    NSLocalizedCollectionImpl::Filename t_filename(filename);
    return impl_->load(t_filename, filename);
}


bool 
NSLocalizedCollection::load(const char *resourceName, const char *tableName, ILogger *logger /*=0*/)
{
    if (logger) {
        return impl_->load(*logger, resourceName, tableName);
    }

    NSLocalizedCollectionImpl::Resource t_resource(resourceName, tableName);
    return impl_->load(t_resource, resourceName, tableName);
}


const char *
NSLocalizedCollection::lookup(const char *key, const char *comment)
{
    return impl_->lookup(key, comment);
}

//end
