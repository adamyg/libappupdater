//  $Id: NSLocalizedString.cpp,v 1.6 2023/10/17 12:33:56 cvsuser Exp $
//
//  NSLocalization - Interface
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

#include "nslocal.h"

#include "NSLocalizedString.h"
#include "NSLocalizedCollection.h"

static NSLocalizedCollection collection_;


/////////////////////////////////////////////////////////////////////////////////////////
//  API bindings
//

//  NSLocalizedLoad:
//      Laod the specified localization table as the default table.
//
//  tableName - The name of the table containing the key-value pairs. If omitted
//      the default table name "Localizable" is used.
//
void
NSLocalizedLoadFile(const char *tableName)
{
    if (!tableName || !*tableName) tableName = "Localizable";
    collection_.load(tableName);
}


void
NSLocalizedLoadResource(const char *resourceName, const char *tableName)
{
    if (!tableName || !*tableName) tableName = "Localizable";
    collection_.load(resourceName, tableName);
}


//  NSLocalizedString:
//      Returns a localized version of a string from the default table.
//
//  key - The key for a string in the table.
//  comment - The comment to place above the key-value pair in the strings file. This
//      provides the translator with some context about the localized string’s usage.
//
const char *
NSLocalizedString(const char *key, const char *comment)
{
    return collection_.lookup(key, comment);
}


//  NSLocalizedStringFromTable:
//      Returns a localized version of a string from the table that you specify.
//
//  key - The key for a string in the table.
//  tableName - The name of the table containing the key-value pairs. If omitted
//      the default table name "Localizable" is used.
//  comment - The comment to place above the key-value pair in the strings file. This
//      provides the translator with some context about the localized string’s usage.
//
const char *
NSLocalizedStringFromTable(const char *key, const char *tableName, const char * /*comment*/)
{
    if (!tableName || !*tableName) tableName = "Localizable";
    return key;
}

//end
