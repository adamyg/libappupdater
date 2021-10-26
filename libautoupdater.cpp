/* -*- mode: c; indent-width: 4; -*- */
/* $Id: libautoupdater.cpp,v 1.19 2021/10/26 13:14:05 cvsuser Exp $
 *
 *  libautoupdater cdecl interface.
 *
 *  Externals:
 *      autoupdate_version
 *      autoupdate_logger_stdout
 *      autoupdate_logger_path
 *      autoupdate_set_console_mode
 *      autoupdate_language_set
 *      autoupdate_hosturl_set
 *      autoupdate_application_set
 *      autoupdate_regpath_set
 *      autoupdate_isavailable
 *      autoupdate_execute
 *
 *  Copyright (c) 2012 - 2021 Adam Young
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x601                      // latest features
#endif
#if !defined(BUILDING_LIBAUTOUPDATER)
#define  BUILDING_LIBAUTOUPDATER
#endif

#include "common.h"
#include <commctrl.h>

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <cassert>

#include "libautoupdater.h"
#include "AutoUpdater.h"

#include "AutoDialog.h"
#include "AutoConsole.h"

#include "AutoConfig.h"
#include "AutoError.h"
#include "AutoLogger.h"
#include "AutoThread.h"
#include "AutoDownload.h"

#include "localisation/NSLocalizedString.h"

#pragma comment(lib, "Comctl32.lib")

/////////////////////////////////////////////////////////////////////////////////////////
//  DLLMain
//

using namespace Updater;                        // implementation namespace

struct tls_value {
    DWORD tid;
    void *data;
};

static CriticalSection tls_lock;                // access lock.
static TlsDestroy_t tls_destroy = NULL;         // destroy callback.
static struct tls_value tls_values[32] = {0};   // thread local storage.

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
        // DLL is loading due to process initialization or a call to LoadLibrary.
        case DLL_PROCESS_ATTACH: {
                INITCOMMONCONTROLSEX InitCtrls = {0};
                InitCtrls.dwSize = sizeof(InitCtrls);
                InitCtrls.dwICC = ICC_WIN95_CLASSES;
                InitCommonControlsEx(&InitCtrls);
                CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

                assert(NULL == tls_destroy);
                memset(tls_values, 0, sizeof(tls_values));
                tls_destroy = NULL;
            }
            break;

        // attached process creates a new thread.
        case DLL_THREAD_ATTACH:
            break;

        // thread of the attached process terminates.
        case DLL_THREAD_DETACH: {
                const DWORD tid = GetCurrentThreadId();
                CriticalSection::Guard guard(tls_lock);
                if (tls_destroy) {
                    // Running under MFC DllMain,
                    // no simple method of trapping thread completion, cleanup on dll detach.
                    unsigned i;
                    for (i = 0; i < _countof(tls_values); ++i) {
                        if (tid == tls_values[i].tid) {
                            (tls_destroy)(tls_values[i].data);
                            tls_values[i].data = NULL;
                            break;
                        } else if (0 == tls_values[i].tid) {
                            break; //eol
                        }
                    }
                }
            }
            break;

        // DLL unload due to process termination or FreeLibrary.
        case DLL_PROCESS_DETACH: {
                CriticalSection::Guard guard(tls_lock);
                if (tls_destroy) {
                    // Running under MFC DllMain,
                    // no simple method of trapping thread completion, cleanup on dll detach.
                    unsigned i;
                    for (i = 0; i < _countof(tls_values); ++i) {
                        if (tls_values[i].data) {
                            (tls_destroy)(tls_values[i].data);
                        } else if (0 == tls_values[i].tid) {
                            break; //eol
                        }
                    }
                }
                memset(tls_values, 0, sizeof(tls_values));
                tls_destroy = NULL;
            }
            break;

        default:
            break;
    }

#if defined(_MSC_VER)
    UNREFERENCED_PARAMETER(hinstDLL);
    UNREFERENCED_PARAMETER(lpvReserved);
#endif

    return TRUE;
}


bool
Updater::LoggerTlsSetValue(void *data, TlsDestroy_t destroy)
{
    assert(NULL == tls_destroy || destroy == tls_destroy);
    if (NULL == tls_destroy || destroy == tls_destroy) {
        const DWORD tid = GetCurrentThreadId();
        unsigned i;

        assert(tid);
        CriticalSection::Guard guard(tls_lock);
        for (i = 0; i < _countof(tls_values); ++i) {
            if (tid == tls_values[i].tid || 0 == tls_values[i].tid) {
                tls_values[i].tid  = tid;
                tls_values[i].data = data;
                tls_destroy = destroy;
                return true;
            }
        }
        assert(false);
    }
    return false;
}


void *
Updater::LoggerTlsGetValue(void)
{
    const DWORD tid = GetCurrentThreadId();
    unsigned i;

    assert(tid);
    CriticalSection::Guard guard(tls_lock);
    for (i = 0; i < _countof(tls_values); ++i) {
        if (tid == tls_values[i].tid) {
            return tls_values[i].data;
        } else if (0 == tls_values[i].tid) {
            break; //eol
        }
    }
    return NULL;
}


//  ShellExecute/RunDll Entry Point.
//
//  Usage:
//      rundll.exe <dllname>,<entrypoint> <optional arguments>
//
//  Example:
//      rundll32.exe libupdater.1.0.dll,ShellExecute "interactive",NULL,SW_SHOWNORMAL
//
//  References:
//      http://support.microsoft.com/kb/164787
//

__declspec(dllexport) void __stdcall            // UNICODE
ShellExecuteW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow)
{
#if defined(_MSC_VER)
    // #pragma comment(linker, "/EXPORT:"__FUNCTION__"="__FUNCTION__) // undecorated name
#pragma comment(linker, "/EXPORT:ShellExecuteW=" __FUNCDNAME__) // decorated function name
#endif

    AutoDialogUI dialog;
    AutoUpdater au(&dialog);
    int ret = au.Execute(AutoUpdater::ExecutePrompt, true);
    ExitProcess(ret);
}


/////////////////////////////////////////////////////////////////////////////////////////
//  library interface
//

extern "C" {

LIBAUTOUPDATER_LINKAGE int LIBAUTOUPDATER_ENTRY
autoupdate_version(void)
{
    return 0x1001;                              /* version 1.0.1 */
}


LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
autoupdate_logger_stdout(bool val)
{
    const char *label = "autoupdate_logger_stdout: ";
    try {
        Logger::get_instance()->SetStdout(val);
    } catch (const std::exception& e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
}


LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
autoupdate_logger_path(const char *path)
{
    const char *label = "autoupdate_logger_path: ";
    try {
        Logger::SetBasePath(path);
    } catch (const std::exception &e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
}


LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
autoupdate_set_console_mode(int val)
{
    const char *label = "autoupdate_set_console_mode: ";
    try {
        Config::SetConsoleMode(val);
    } catch (const std::exception& e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
}


LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
autoupdate_language_set(const char *language)
{
    const char *label = "autoupdate_language_set: ";
    try {
        Config::SetLanguage(language);
    } catch (const std::exception& e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
}


LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
autoupdate_hosturl_set(const char *url)
{
    const char *label = "autoupdate_hosturl_set: ";
    try {
        Config::SetHostURL(url);
    } catch (const std::exception& e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
}


LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
autoupdate_channel_set(const char *channel)
{
    const char *label = "autoupdate_channel_set: ";
    try {
        Config::SetChannel(channel);
    } catch (const std::exception& e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
}


LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
autoupdate_oslabel_set(const char *oslabel)
{
    const char *label = "autoupdate_oslabel_set: ";
    try {
        Config::SetOSLabel(oslabel);
    } catch (const std::exception& e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
}


LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
autoupdate_appname_set(const char *appname)
{
    const char *label = "autoupdate_appname_set: ";
    try {
        Config::SetAppName(appname);
    } catch (const std::exception& e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
}


LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
autoupdate_appversion_set(const char *appversion)
{
    const char *label = "autoupdate_appversion_set: ";
    try {
        Config::SetAppVersion(appversion);
    } catch (const std::exception& e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
}



LIBAUTOUPDATER_LINKAGE void LIBAUTOUPDATER_ENTRY
autoupdate_regpath_set(const char *path)
{
    const char *label = "autoupdate_regpath_set: ";
    try {
        Config::SetRegistryPath(path);
    } catch (const std::exception &e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
}


LIBAUTOUPDATER_LINKAGE int LIBAUTOUPDATER_ENTRY
autoupdate_isavailable(void)
{
    const char *label = "autoupdate_isavailable: ";
    int ret = -1;

    try {
        AutoDialogUI dialog;
        AutoUpdater au(&dialog);
        ret = au.IsAvailable();

    } catch (const std::exception &e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
    Logger::release_instance();

    return ret;
}


LIBAUTOUPDATER_LINKAGE int LIBAUTOUPDATER_ENTRY
autoupdate_execute(int mode, int interactive)
{
    const char *label = "autoupdate_execute: ";
    int ret = -1;

    try {
        enum AutoUpdater::ExecuteMode exmode;

        switch (mode) {
        case  0: exmode = AutoUpdater::ExecuteDisable; break;
        case  1: exmode = AutoUpdater::ExecuteEnable; break;
        case  2: exmode = AutoUpdater::ExecuteAuto;  break;
        case  3: exmode = AutoUpdater::ExecutePrompt; break;
        case  4: exmode = AutoUpdater::ExecuteIgnoreSkip; break;
        case  5: exmode = AutoUpdater::ExecuteReinstall; break;
        case -1: exmode = AutoUpdater::ExecuteReset; break;
        case -2: exmode = AutoUpdater::ExecuteDump; break;
        default:
            return -2;
        }

     // NSLocalizedLoadFile("../lproj/Strings/Base.strings");
     // NSLocalizedLoadResource("NSLocalized", "Localizable");
        NSLocalizedLoadResource("NSLocalized", "xx");

        if (1 == Config::GetConsoleMode()) {
            AutoConsoleUI console;
            AutoUpdater au(&console);
            ret = au.Execute(exmode, (interactive ? true : false));

        } else {
            AutoDialogUI dialog;
            AutoUpdater au(&dialog);
            ret = au.Execute(exmode, (interactive ? true : false));
        }

    } catch (const std::exception &e) {
        LOG<LOG_ERROR>() << label << e.what() << LOG_ENDL;
    } catch (...) {
        LOG<LOG_ERROR>() << label << "Unknown exception" << LOG_ENDL;
    }
    Logger::release_instance();

    return ret;
}

}   // extern "C"


#if defined(__WATCOMC__)
int autosprintf_s( char * s, size_t n, const char * format, ... ) {
    va_list ap;
    va_start(ap, format);
    int ret = std::vsnprintf(s, n, format, ap);
    if (ret < 0 || ret >= n) s[n-1] = 0;
    va_end(ap);
    return ret;
}

int autosnprintf_s( char * s, size_t n, const char * format, ... ) {
    va_list ap;
    va_start(ap, format);
    int ret = std::vsnprintf(s, n, format, ap);
    if (ret < 0 || ret >= n) s[n-1] = 0;
    va_end(ap);
    return ret;
}

int autovsprintf_s( char * s, size_t n, const char * format, va_list ap ) {
    int ret = std::vsnprintf(s, n, format, ap);
    if (ret < 0 || ret >= n) s[n-1] = 0;
    return ret;
}

#endif  //__WATCOMC__

/*end*/
