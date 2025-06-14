//  $Id: NSLocalizedDefault.cpp,v 1.4 2023/10/17 12:33:56 cvsuser Exp $
//
//  NSLocalization - Default locale
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

#include <cstdlib>

#include "nslocal.h"

#if !defined(WINDOWS_MEAN_AND_LEAN)
#define WINDOWS_MEAN_AND_LEAN
#endif
#include <Windows.h>

static const struct ISOName {
    LANGID langid;
    const char *name;
} Win32ISONames[] = {
    { 0x0401, "ar_SA"   },
    { 0x0402, "bg"      },
    { 0x0403, "ca"      },
    { 0x0404, "zh_TW"   },
    { 0x0405, "cs"      },
    { 0x0406, "da"      },
    { 0x0407, "de"      },
    { 0x0408, "el"      },
    { 0x0409, "en_US"   },
    { 0x040a, "es"      },
    { 0x040b, "fi"      },
    { 0x040c, "fr"      },
    { 0x040d, "he"      },
    { 0x040e, "hu"      },
    { 0x040f, "is"      },
    { 0x0410, "it"      },
    { 0x0411, "ja"      },
    { 0x0412, "ko"      },
    { 0x0413, "nl"      },
    { 0x0414, "no"      },
    { 0x0415, "pl"      },
    { 0x0416, "pt_BR"   },
    { 0x0418, "ro"      },
    { 0x0419, "ru"      },
    { 0x041a, "hr"      },
    { 0x041c, "sq"      },
    { 0x041d, "sv"      },
    { 0x041e, "th"      },
    { 0x041f, "tr"      },
    { 0x0420, "ur"      },
    { 0x0421, "in"      },
    { 0x0422, "uk"      },
    { 0x0423, "be"      },
    { 0x0425, "et"      },
    { 0x0426, "lv"      },
    { 0x0427, "lt"      },
    { 0x0429, "fa"      },
    { 0x042a, "vi"      },
    { 0x042d, "eu"      },
    { 0x042f, "mk"      },
    { 0x0436, "af"      },
    { 0x0438, "fo"      },
    { 0x0439, "hi"      },
    { 0x043e, "ms"      },
    { 0x0458, "mt"      },
    { 0x0801, "ar_IQ"   },
    { 0x0804, "zh_CN"   },
    { 0x0807, "de_CH"   },
    { 0x0809, "en_GB"   },
    { 0x080a, "es_MX"   },
    { 0x080c, "fr_BE"   },
    { 0x0810, "it_CH"   },
    { 0x0812, "ko"      },
    { 0x0813, "nl_BE"   },
    { 0x0814, "no"      },
    { 0x0816, "pt"      },
    { 0x081a, "sr"      },
    { 0x081d, "sv_FI"   },
    { 0x0c01, "ar_EG"   },
    { 0x0c04, "zh_HK"   },
    { 0x0c07, "de_AT"   },
    { 0x0c09, "en_AU"   },
    { 0x0c0a, "es"      },
    { 0x0c0c, "fr_CA"   },
    { 0x0c1a, "sr"      },
    { 0x1001, "ar_LY"   },
    { 0x1004, "zh_SG"   },
    { 0x1007, "de_LU"   },
    { 0x1009, "en_CA"   },
    { 0x100a, "es_GT"   },
    { 0x100c, "fr_CH"   },
    { 0x1401, "ar_DZ"   },
    { 0x1407, "de_LI"   },
    { 0x1409, "en_NZ"   },
    { 0x140a, "es_CR"   },
    { 0x140c, "fr_LU"   },
    { 0x1801, "ar_MA"   },
    { 0x1809, "en_IE"   },
    { 0x180a, "es_PA"   },
    { 0x1c01, "ar_TN"   },
    { 0x1c09, "en_ZA"   },
    { 0x1c0a, "es_DO"   },
    { 0x2001, "ar_OM"   },
    { 0x2009, "en_JM"   },
    { 0x200a, "es_VE"   },
    { 0x2401, "ar_YE"   },
    { 0x2409, "en"      },
    { 0x240a, "es_CO"   },
    { 0x2801, "ar_SY"   },
    { 0x2809, "en_BZ"   },
    { 0x280a, "es_PE"   },
    { 0x2c01, "ar_JO"   },
    { 0x2c09, "en_TT"   },
    { 0x2c0a, "es_AR"   },
    { 0x3001, "ar_LB"   },
    { 0x300a, "es_EC"   },
    { 0x3401, "ar_KW"   },
    { 0x340a, "es_CL"   },
    { 0x3801, "ar_AE"   },
    { 0x380a, "es_UY"   },
    { 0x3c01, "ar_BH"   },
    { 0x3c0a, "es_PY"   },
    { 0x4001, "ar_QA"   },
    { 0x400a, "es_BO"   },
    { 0x440a, "es_SV"   },
    { 0x480a, "es_HN"   },
    { 0x4c0a, "es_NI"   },
    { 0x500a, "es_PR"   }
    };


static const char *
Search(int l, int r, const LANGID langid)
{
    if (r >= l) {
        const int mid = l + (r - l) / 2;
        const LANGID val = Win32ISONames[mid].langid;

        if (val == langid) {
            return Win32ISONames[mid].name;
        }

        if (val > langid) {
            r = mid - 1;
        } else {
            l = mid + 1;
        }
    }
    return "";
}


const char *
NSLocalizedDefault()
{
    return Search(0, _countof(Win32ISONames) - 1, ::GetUserDefaultUILanguage());
        //WINVER >= 0x0500 (Windows 2000/XP)
}

