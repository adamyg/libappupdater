//  $Id: NSLocalizedCollectionImpl.cpp,v 1.13 2024/05/15 08:28:33 cvsuser Exp $
//
//  NSLocalization - Collection implementation
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2022 - 2023, Adam Young
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

#include <iostream>
#include <fstream>
#include <cctype>
#include <cstdarg>
#if defined(__WATCOMC__) && (__WATCOMC__ >= 1300)
#include <cstdio>
#endif
#include <cassert>
#include <ios>

#include "nslocal.h"
#include "NSLocalizedCollectionImpl.h"

#include "NSFormat.h"

#if defined(_MSC_VER)
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4244) // possible loss of data
#endif

extern void NSFormatTests(void);


/////////////////////////////////////////////////////////////////////////////////////////
//  NSLocalizedCollectionImpl::ILogger's
//

NSLocalizedCollectionImpl::Filename::Filename(const char *filename)
    : filename_(filename)
{
}


//virtual
void
NSLocalizedCollectionImpl::Filename::error(unsigned line_number, const char *msg)
{
    if (line_number) {
        std::cout << filename_ << " (" << line_number << ") : " << msg << std::endl;
    } else {
        std::cout << filename_ << " : " << msg << std::endl;
    }
}


NSLocalizedCollectionImpl::Resource::Resource(const char *resourceName, const char *tableName)
    : resourceName_(resourceName), tableName_(tableName)
{
}


//virtual
void
NSLocalizedCollectionImpl::Resource::error(unsigned line_number, const char *msg)
{
    if (line_number) {
        std::cout << resourceName_ << "::" << tableName_ << " (" << line_number << ") : " << msg << std::endl;
    } else {
        std::cout << resourceName_ << "::" << tableName_ << " : " << msg << std::endl;
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
//  NSLocalizedCollectionImpl::Logger
//

NSLocalizedCollectionImpl::Logger::Logger(NSLocalizedCollection::ILogger &ilogger)
    : ilogger_(ilogger), line_number(0) {
}


void
NSLocalizedCollectionImpl::Logger::error(const char *fmt, ...)
{
    char buffer[1024] = {0};
    va_list ap;

    va_start(ap, fmt);
#if defined(_MSC_VER) && (_MSC_VER <= 1600)
    _vsnprintf(buffer, sizeof(buffer)-1, fmt, ap);
#else
    std::vsnprintf(buffer, sizeof(buffer)-1, fmt, ap);
#endif
    ilogger_.error(line_number, buffer);
    va_end(ap);
}


/////////////////////////////////////////////////////////////////////////////////////////
//  NSLocalizedCollectionImpl
//

bool
NSLocalizedCollectionImpl::load(NSLocalizedCollection::ILogger &ilogger, const char *filename)
{
    Logger logger(ilogger);
    return load_strings(logger, filename);
}


bool
NSLocalizedCollectionImpl::load(NSLocalizedCollection::ILogger &ilogger, const char *resourceName, const char *tableName)
{
    Logger logger(ilogger);
    return load_strings(logger, resourceName, tableName);
}


const char *
NSLocalizedCollectionImpl::lookup(const char *key, const char * /*comment*/)
{
    Collection_t::const_iterator it = strings_.find(std::string(key));
    if (it != strings_.end()) {                 // exists
        if (it->second.length()) {              // and non empty
            return it->second.c_str();
        }
    }
    return key;
}


bool
NSLocalizedCollectionImpl::load_strings(Logger &logger, const char *filename)
{
    std::string t_filename(filename);

    if (t_filename.find('.') == std::string::npos) {
        t_filename.append(".strings");          // default extension.
    }

    std::ifstream strs(t_filename.c_str(), std::ios::in|std::ios::binary);
    if (! strs.is_open()) {
        logger.error("unable to open : %s (%d)", strerror(errno), errno);
        return false;
    }

    std::string buffer;

    strs.seekg(0, std::ios::end);               // size image.
    buffer.reserve((size_t) strs.tellg());      // create working storage.
    strs.seekg(0, std::ios::beg);               // reposition for read.
#if defined(__WATCOMC__)
    {
        char t_buffer[4096];
        while (strs.read(t_buffer, sizeof(t_buffer))) {
            buffer.append(t_buffer, sizeof(t_buffer));
        }
        buffer.append(t_buffer, (size_t)strs.gcount());
    }
#else
    buffer.assign((std::istreambuf_iterator<char>(strs)), std::istreambuf_iterator<char>());
#endif

    if (strs.bad()) {
        logger.error("unable to read : %s (%d)", strerror(errno), errno);
        return false;
    }

    return load_strings(logger, buffer, filename);
}


bool
NSLocalizedCollectionImpl::load_strings(Logger &logger, const char *name, const char *type, void *module /*= 0*/)
{
    HINSTANCE t_module = (HINSTANCE) module;
    HRSRC res;

    if (0 == (res = ::FindResourceA(t_module, name, type)) &&
                0 == t_module && 0 != (t_module = ModuleHandle())) {
        res = ::FindResourceA(t_module, name, type);
    }

    if (res) {
        HGLOBAL global = ::LoadResource(t_module, res);
        if (global) {
            const char *data = (const char *) ::LockResource(global);
            size_t size = ::SizeofResource(t_module, res);

            if (data && size) {
                                                // localisation object.
                if (size > 4 && 0 == memcmp(data, "NSMO", 4)) {
                    return load_object(logger, (unsigned char *)data, (unsigned char *)data + size, "resource");
                }

                if (data[size-1] == '\0') {     // null-terminated string.
                    --size; //consume
                }
                return load_strings(logger, data, data + size, "resource");
            }
        }
    }

    logger.error("unable to access : %u", (unsigned)GetLastError());
    return false;
}


/////////////////////////////////////////////////////////////////////////////////////////
//  Parsers

bool
NSLocalizedCollectionImpl::load_object(Logger & /*logger*/, const void *it, const void *end, const char * /*name*/)
{
    //  Import elements
    //
    //  Example:
    //
    //      NSMO
    //      <hibyte+lobyte=strlen1><hibyte+lobyte=strlen2>
    //      <bytes=string1>\0<bytes=string2>\0
    //
    const unsigned char *cursor = static_cast<const unsigned char *>(it);

#if defined(_DEBUG)
#if defined(BUILDING_LIBAUTOUPDATER)
    NSFormatTests();
#endif
#endif

    if ((cursor + 4) >= end || 0 != memcmp(cursor, "NSMO", 4)) {
        return false;
    }

    cursor += 4;

    while ((cursor + 2 + 2 + 1 + 1) < end) {
        const unsigned
            length1 = ((unsigned)cursor[0] << 8) + cursor[1],
            length2 = ((unsigned)cursor[2] << 8) + cursor[3];

        cursor += 4;
        if ((cursor + length1 + length2 + 2 /*nuls*/) > end) {
            break;
        }

        assert(0 == cursor[length1]);
        std::string key((const char *)cursor, length1);
        cursor += length1 + 1;

//#if defined(_DEBUG)
//      std::cout << "\"" << key << "\"=\"" << std::string((const char *)it, length2) << "\"" << std::endl;
//#endif

        assert(0 == cursor[length2]);
        strings_[key].assign((const char *)cursor, length2);
        cursor += length2 + 1;
    }
    return true;
}


bool
NSLocalizedCollectionImpl::load_strings(Logger &logger, const std::string &buffer, const char *name)
{
    const char *it = static_cast<const char *>(buffer.data());
    return load_strings(logger, it, it + buffer.length(), name);
}



bool
NSLocalizedCollectionImpl::load_strings(Logger &logger, const char *it, const char *end, const char * /*name*/)
{
    //  Import elements
    //
    //  Example:
    //
    //      /* Insert Element menu item */
    //      "Insert Element" = "Insert Element";
    //
    //      /* Error string used for unknown error types. */
    //      "ErrorString_1" = "An unknown error occurred.";
    //
    bool newline = true;                        // newline + white-space.

    logger.line_number = 1;
    while (it < end) {
        const char ch = *it++;

        if ('/' == ch && it < end) {
            if (*it == '/') {                   // C++ style comment, consume.
                ++it;
                while (it < end && *it != '\n') {
                    // consume until eol.
                    ++it;
                }
                continue;

            } else if (*it == '*') {            // C style comment, consume.
                ++it;
                while (it < end) {
                    if ('*' == *it &&           // end-of-comment.
                            (it + 1) < end && '/' == it[1]) {
                        it += 2;
                        break;
                    }
                    ++it;
                }
                continue;
            }

        } else if (' ' == ch || '\t' == ch) {
            continue;

        } else if ('\n' == ch) {
            if (it < end && *it == '\r')
                ++it;
            ++logger.line_number;
            newline = true;
            continue;

        } else if ('"' == ch && newline) {      // string.
            std::string key, val;

            if (NULL == (it = get_str(logger, it - 1, end, key)) ||
                NULL == (it = get_dem(logger, it, end, '=')) ||
                NULL == (it = get_str(logger, it, end, val)) ||
                NULL == (it = get_dem(logger, it, end, ';'))) {
                return false;
            }

            strings_[key] = val;
            newline = false;
            continue;
        }

        if (std::isprint(ch)) {
            logger.error("syntax error, unexpected character '%c'", ch);
        } else {
            logger.error("syntax error, unexpected character '0x%x'", ch);
        }
        return false;
    }
    return true;
}


bool
NSLocalizedCollectionImpl::load_po(Logger &logger, const char *it, const char *end, const char * /*name*/)
{
    // white-space
    // #  translator-comments
    // #. extracted-comments
    // #: reference…
    // #, flag…
    // #| msgid previous-untranslated-string
    //
    // msgid untranslated-string
    // msgstr translated-string
    //
    // msgid untranslated-string-singular
    // msgid_plural untranslated-string-plural
    // msgstr[0] translated-string-case-0
    // ...
    // msgstr[N] translated-string-case-n

    std::string msgid, msgstr;
    bool newline = true;                        // newline + white-space.
    enum {Open, MsgId, Plurals} state = Open;

    logger.line_number = 1;
    while (it < end) {
        const char ch = *it++;

        if ('#' == ch) {                        // comment
            while (it < end && '\n' != *it) {
                ++it;
            }
            continue;

        } else if (' ' == ch || '\t' == ch) {   // whitespace
            continue;

        } else if ('\n' == ch) {                // newline
            if (it < end && *it == '\r') ++it;
            ++logger.line_number;
            newline = true;
            continue;

        } else {    //token
            std::string token;

            if (NULL != (it = get_tok(logger, it - 1, end, token))) {
                if (newline) {
                    newline = false;
                    if (Open == state || Plurals == state) {
                        if ("msgid" == token) {
                            msgid.clear();
                            if (NULL == (it = get_str(logger, it, end, msgid, "msgid untranslated-string"))) {
                                return false;
                            }
                            state = MsgId;
                            continue;
                        }

                    } else if (MsgId == state) {
                        if ("msgstr" == token) {
                            msgstr.clear();
                            if (NULL == (it = get_str(logger, it, end, msgstr, "msgstr translated-string"))) {
                                return false;
                            }
                            strings_[msgid] = msgstr;
                            state = Open;
                            continue;

                        } else if ("msgid_plural" == token) {
                            msgstr.clear();
                            if (NULL == (it = get_str(logger, it, end, msgstr, "msgstr translated-string"))) {
                                return false;
                            }
                            strings_[msgid] = msgstr;
                            state = Plurals;
                            continue;
                        }

                    } else if (state >= Plurals) {
                        std::string plural;
                        unsigned idx = 0;

                        if (1 == std::sscanf(token.c_str(), "msgstr[%u]", &idx)) {
                            if (NULL == (it = get_str(logger, it, end, plural, "msgstr[x] translated-string"))) {
                                return false;
                            } else if ((int)idx != (state - 2)) {
                                logger.error("unexpected plural index [%u]", idx);
                                return false;
                            }

                         // plurals_[msgid] = msgstr;
                //TODO:     logger.warning("plurals not supported, ignored");
                            continue;
                        }
                    }
                }

                logger.error("unexpected token <%s>", token.c_str());
                return false;
            }
        }

        if (std::isprint(ch)) {
            logger.error("syntax error, unexpected character '%c'", ch);
        } else {
            logger.error("syntax error, unexpected character '0x%x'", ch);
        }
        return false;
    }

    if (MsgId == state || Plurals == state) {
        logger.error("incomplete 'msgid' definition");
        return false;
    }
    return true;
}


//static
const char *
NSLocalizedCollectionImpl::get_str(Logger &logger, const char *it, const char *end, std::string &result, const char *what)
{
    while (it < end && (' ' == *it || '\t' == *it)) {
        ++it;                                   // leading whitespace.
    }

    if (it >= end || '"' != *it++) {
        if (what) {
            logger.error("expected '%s'", what);
        } else {
            logger.error("expected a string");
        }
        return NULL;
    }

    result.reserve(128);

    while (it < end) {
        const char ch = *it++;

        if ('"' == ch) {                        // eos.
            if (result.empty()) {
                if (what) {
                    logger.error("empty '%s'", what);
                } else {
                    logger.error("empty string");
                }
                return NULL;
            }
            return it;
        }

        if ('\\' == ch && it < end) {           // escape.
            char ch2 = *it++;
            char val = 0;

            switch (ch2) {
                // Punctuation characters
            case '\\': val = '\\'; break;       // \\ = backslash
            case '"':  val = '\"'; break;       // \" = quotation mark (backslash not required for '"')
            case '\'': val = '\''; break;       // \' = apostrophe (backslash not required for "'")
            case '?':  val = '?';  break;       // \? = question mark (used to avoid trigraphs)

                // Control characters
            case 'a':  val = '\a'; break;       // \a = \x07 = alert (bell)
            case 'b':  val = '\b'; break;       // \b = \x08 = backspace
            case 't':  val = '\t'; break;       // \t = \x09 = horizonal tab
            case 'n':  val = '\n'; break;       // \n = \x0A = newline (or line feed)
            case 'v':  val = '\v'; break;       // \v = \x0B = vertical tab
            case 'f':  val = '\f'; break;       // \f = \x0C = form feed
            case 'r':  val = '\r'; break;       // \r = \x0D = carriage return
            case 'e':  val = 0x1b; break;       // \e = \x1B = escape (non-standard GCC extension)

                // Numeric character references
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7': {
                    unsigned n = 0;             // \  + up to 3 octal digits

                    n = (n << 3) + (ch2 - '0');
                    if ((ch2 = *it) != 0 && (ch2 >= '0' && ch2 <= '7')) {
                        n = (n << 3) + (ch - '0'); ++it;
                        if ((ch2 = *it) != 0 && (ch2 >= '0' && ch2 <= '7')) {
                            n = (n << 3) + (ch - '0'); ++it;
                        }
                    }
                    result += (char)n;
                }
                break;
            case 'x':                           // \x + any number of hex digits
                if (NULL == (it = get_hex(logger, it, end, 0, result))) {
                    return NULL;
                }
                break;
            case 'u':                           // \u + 4 hex digits (Unicode BMP, new in C++11)
                if (NULL == (it = get_hex(logger, it, end, 4, result))) {
                    return NULL;
                }
                break;
            case 'U':                           // \U + 8 hex digits (Unicode astral planes, new in C++11)
                if (NULL == (it = get_hex(logger, it, end, 8, result))) {
                    return NULL;
                }
                break;

            case '\n': case '\r':
                if (what) {
                    logger.error("unterminated '%s'", what);
                } else {
                    logger.error("unterminated string");
                }
                return NULL;

            default:
                logger.error("illegal escape \\%c", ch2);
                return NULL;
            }

            if (val) result += val;
            continue;
        }
        result += ch;
    }

    logger.error("unterminated string");
    return NULL;
}


//static
const char *
NSLocalizedCollectionImpl::get_hex(Logger &logger, const char *it, const char *end, unsigned limit, std::string &result)
{
    unsigned length = 0, n = 0;

    while (it < end && (0 == limit || length < limit)) {
        const char ch = *it++;

        if (!std::isxdigit(ch)) {
            if (length && 0 == limit) {
                --it;
                break;
            }

            if (std::isprint(ch)) {
                logger.error("invalid hexadecimal character '%c'", ch);
            } else {
                logger.error("invalid hexadecimal character '0x%x'", ch);
            }
            return NULL; // error
        }

        if (std::isdigit(ch)) {                 // '0' .. '9'
            n = (n << 4U) + (ch - '0');
        } else if (std::isupper(ch)) {          // 'A' .. 'F'
            n = (n << 4U) + (ch - 'A' + 10);
        } else {                                // 'a' .. 'f'
            n = (n << 4U) + (ch - 'a' + 10);
        }
        ++length;
    }

    if (n >= 0xff) {
        const wchar_t wc = (wchar_t) n;
        char t_buffer[16] = {0};

        ::WideCharToMultiByte(CP_UTF8, 0, &wc, 1, t_buffer, sizeof(t_buffer), NULL, NULL);
        result += t_buffer;
    } else {
        result += n;                            // result
    }
    return it;
}


//static
const char *
NSLocalizedCollectionImpl::get_tok(Logger &logger, const char *it, const char *end, std::string &result)
{
    if (it < end) {
        while (it < end && !(' ' == *it || '\t' == *it || '\n' == *it)) {
            result += *it++;
        }
        return it;
    }

    logger.error("expected a token");
    return NULL;
}


//static
const char *
NSLocalizedCollectionImpl::get_dem(Logger &logger, const char *it, const char *end, const char tok)
{
    while (it < end && (' ' == *it || '\t' == *it)) {
        ++it;                                   // leading whitespace.
    }

    if (it < end && tok == *it++) {
        return it;                              // token.
    }

    logger.error("missing '%c' delimiter", tok);
    return NULL;
}


/////////////////////////////////////////////////////////////////////////////////////////
//  Support

//static
HINSTANCE
NSLocalizedCollectionImpl::ModuleHandle()
{
    static HMODULE hm = NULL;
    if (0 == hm) {
        if (::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&ModuleHandle, &hm) != 0) {
            return hm;
        }
        assert(false);
    }
    return hm;
}


//static
std::wstring
NSLocalizedCollectionImpl::to_wstring(const std::string &str)
{
    const int wlength =
        ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.length(), 0, 0);
    std::wstring wstr;

    if (wlength > 0) {
        wstr.resize(wlength);
        ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.length(), const_cast<wchar_t *>(wstr.data()), wlength);
    }
    return wstr;
}

//end
