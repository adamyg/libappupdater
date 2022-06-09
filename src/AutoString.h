#ifndef AUTOSTRING_H_INCLUDED
#define AUTOSTRING_H_INCLUDED
//  $Id: AutoString.h,v 1.6 2022/06/09 08:46:30 cvsuser Exp $
//
//  AutoUpdater: string
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

#include <string>
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
#include <codecvt>
#endif

/////////////////////////////////////////////////////////////////////////////////////////
//  AString
//

class AString : public std::wstring {
public:
    AString& operator +=(const std::string &str) {
        const char *cursor = str.c_str();
        while (*cursor) {
            std::wstring::push_back(*cursor++);
        }
        return *this;
    }
    AString& operator +=(const char *str) {
        if (const char *cursor = str) {
            while (*cursor) {
                std::wstring::push_back(*cursor++);
            }
        }
        return *this;
    }
    AString& operator +=(const wchar_t *str) {
        append(str);
        return *this;
    }
};


/////////////////////////////////////////////////////////////////////////////////////////
//  String conversion
//

namespace Updater {

// string to wstring conversion.
inline std::string
to_string(const wchar_t *wstr) {
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
    using convert_t = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_t, wchar_t> converter;
    return converter.to_bytes(wstr);

#else
    const size_t size = (wcslen(wstr) * MB_LEN_MAX) + 1;
    std::string result;

    result.resize(size);
    result.resize(std::wcstombs((char *)result.data(), wstr, size));
    return result;
#endif
}

// wstring to string conversion.
inline std::string
to_string(const std::wstring& wstr) {
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
    using convert_t = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_t, wchar_t> converter;
    return converter.to_bytes(wstr);

#else
    const size_t size = (wstr.length() * MB_LEN_MAX) + 1;
    std::string result;

    result.resize(size);
    result.resize(std::wcstombs((char *)result.data(), wstr.c_str(), size));
    return result;
#endif
}

// string to wstring conversion.
inline std::wstring
to_wstring(const std::string& str) {
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
    using convert_t = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_t, wchar_t> converter;
    return converter.from_bytes(str);

#else
    const size_t size = str.length() + 1;
    std::wstring wresult;

    wresult.resize(size);
    wresult.resize(std::mbstowcs((wchar_t *)wresult.data(), str.c_str(), size));
    return wresult;
#endif
}

}   // namespace Updater

#endif  //AUTOSTRING_H_INCLUDED
