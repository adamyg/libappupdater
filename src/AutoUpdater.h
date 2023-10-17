#ifndef AUTOUPDATER_H_INCLUDED
#define AUTOUPDATER_H_INCLUDED
//  $Id: AutoUpdater.h,v 1.19 2023/10/17 12:33:57 cvsuser Exp $
//
//  AutoUpdater: application interface.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2023, Adam Young
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
#include <memory>

#include "IAutoUpdaterUI.h"
#include "AutoManifest.h"

#if !defined(LIBAUTOUPDATER_LINKAGE)
#if defined(AUTOUPDATER_STATIC)
#   define LIBAUTOUPDATER_LINKAGE
#   define LIBAUTOUPDATER_ENTRY
#elif defined(WIN32) || defined(_WIN32)
#   if defined(BUILDING_LIBAUTOUPDATER)
#       define LIBAUTOUPDATER_LINKAGE __declspec(dllexport)
#  else
#       define LIBAUTOUPDATER_LINKAGE __declspec(dllimport)
#  endif
#   define LIBAUTOUPDATER_ENTRY __cdecl
#else
#   define LIBAUTOUPDATER_LINKAGE
#   define LIBAUTOUPDATER_ENTRY
#endif
#endif

class IAutoUpdaterUI;
class CUpdateInstallDlg;                        // dialog implementation

class IInstallNow {
public:
    void operator()(const std::string &message) {
        (*this)(message.c_str());
    }
    void message(const char *format, ...) {
        char buffer[1024];
        va_list ap;

        va_start(ap, format);
        vsprintf_s(buffer, sizeof(buffer), format, ap);
        (*this)(buffer);
        va_end(ap);
    }
    virtual void operator()(const char *message) = 0;
    virtual HWND GetParent() = 0;
};

class LIBAUTOUPDATER_LINKAGE AutoUpdater {
public:
    AutoUpdater(IAutoUpdaterUI *dialog = NULL);
    virtual ~AutoUpdater();

    // Configuration
    HINSTANCE           ModuleHandle();         // resource handle.
    void                AppName(const char *appname);
    const char *        AppName() const;
    void                AppVersion(const char *appverion);
    const char *        AppVersion() const;
    void                HostURL(const char *hosturl);
    const char *        HostURL() const;

    // Execute autoupdater
    enum ExecuteMode {
        ExecuteDump = -2,                       // Dump the updater status.
        ExecuteReset = -1,                      // Reset updater status.
        ExecuteDisable,                         // Disable automatic periodic checks.
        ExecuteEnable,                          // Enable automatic checks.
        ExecuteAuto,                            // Automatically check if interval has been exceeded.
        ExecutePrompt,                          // Prompt if automatic checks are disabled.
        ExecuteIgnoreSkip,                      // Prompt ignoring skip status.
        ExecuteReinstall                        // Prompt unconditionally, even if up-to-date/skipped.
    };

    void                EnableDialog();
    void                EnableConsole();
    int                 Execute(const enum ExecuteMode mode = ExecuteAuto, bool interactive = false);

    // Retrieve application details
    const Updater::AutoManifest& Manifest() const;

    // Public registry interface
    bool                GetAuto() const;
    void                SetAuto(bool state);

public:
    // Dialog actions
    bool                InstallNow(IInstallNow &updater, bool interactive = false);
    void                InstallLater();
    void                InstallSkip();

    // Execute tests
    bool                IsSkipped();
    int                 IsAvailable(bool interactive = false);

private:
    // Support functions
    const std::string&  GetTargetName();
    bool                Verify(const std::string &filename);

    // Registry functions
    enum UpdateStatus {
        STATUS_PROMPT   = -1,                   // not configured and/or prompt user.
        STATUS_DISABLED = 0,                    // auto-update disabled.
        STATUS_ENABLED  = 1,                    // auto-update check required.
        STATUS_ALREADY  = 2                     // already performed within the last check interval.
    };

    enum UpdateStatus   Status(const enum ExecuteMode mode);
    bool                Once() const;
    void                SetOnce(bool val);
    void                Dump();
    void                Reset();

    enum PromptResponse PromptDialog();
    int                 InstallDialog();
    void                UptoDateDialog();

    void                ProgressStart(HWND parent, bool indeterminate = false, const char *msg = 0);
    void                ProgressUpdate(int percentage, int total = 0);
    bool                ProgressCancelled();
    bool                ProgressStop();

private:
    virtual int         DownloadFlags()
            { return 0; }

private:
    friend class AutoUpdaterImpl;
    friend class AutoUpdaterSink;
    class AutoUpdaterImpl *d_impl;              // implementation.
};

#endif  /*AUTOUPDATER_H_INCLUDED*/
