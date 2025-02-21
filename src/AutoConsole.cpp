//  $Id: AutoConsole.cpp,v 1.15 2025/02/21 19:03:23 cvsuser Exp $
//
//  AutoUpdater: console interface.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2025, Adam Young
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

#include "common.h"

#include <conio.h>
#include <iostream>

#include "AutoConsole.h"
#include "AutoUpdater.h"
#include "AutoError.h"
#include "AutoLogger.h"
#include "AutoThread.h"
#include "TProgressBar.h"


/////////////////////////////////////////////////////////////////////////////////////////
//  CConsoleUpdater
//

class CConsoleUpdater : public IInstallNow {
public:
    CConsoleUpdater(AutoConsoleUI &owner) : owner_(owner) {
    }

    virtual void operator()(const char * /*message*/) {
    }

    virtual HWND GetParent() {
        return NULL;
    }

    AutoConsoleUI &owner_;
};


/////////////////////////////////////////////////////////////////////////////////////////
//  AutoConsole
//

AutoConsoleUI::AutoConsoleUI() : progress_(NULL)
{
}


AutoConsoleUI::~AutoConsoleUI()
{
    ProgressStop();
}


enum PromptResponse
AutoConsoleUI::PromptDialog(AutoUpdater &owner)
{
    std::cout
        << "Allow "
        << owner.AppName()
        << " to automatically check for updates?\n"
        << "Alternatively check once and select at a later time [O(once), Y(es), N(o)]";
    std::cout.flush();
    std::fflush(stdout);

    enum PromptResponse rsp = PROMPT_NO;
    while (1) {
        const int ch = _getch();
        if (0 == ch || 0xE0 == ch) {            // function key, consume.
            (void) _getch();
            continue;
        }

        if ('o' == ch || 'O' == ch) {           // once
            std::cout << "\nchecking once ..." << std::endl;
            rsp = PROMPT_ONCE;
            break;

        } else if ('y' == ch || 'Y' == ch) {    // yes
            std::cout << "\nchecking automatically ..." << std::endl;
            rsp = PROMPT_AUTO;
            break;

        } else if ('n' == ch || 'N' == ch || 0x1b == ch) {
            break;                              // no/escape
        }
    }

    std::cout << std::endl;
    return rsp;
}


int
AutoConsoleUI::InstallDialog(AutoUpdater &owner)
{
    // dialogue
    if (! owner.Manifest().title.empty())
     std::cout
        << "[" << owner.Manifest().title << "]\n";
    std::cout
        << owner.AppName()
        << " ";
    if (! owner.Manifest().BuildLabel.empty())
     std::cout
        << "(" << owner.Manifest().BuildLabel << ") ";
    std::cout
        << owner.Manifest().attributeVersion
        << " is now available (you have "
        << owner.AppVersion()
        << ")\n";

    // prompt
    const bool is_critical =                    // disable skip if critical
        owner.Manifest().IsCriticalUpdate(owner.AppVersion());

    std::cout
        << "Would you like to install it now? [Y(es), L(ater)"
            << (is_critical ? "" : ", S(kip)")
        << "]";
    std::cout.flush();
    std::fflush(stdout);

    while (1) {
        const int ch = _getch();
        if (0 == ch || 0xE0 == ch) {            // function key, consume.
            (void) _getch();
            continue;
        }

        if ('y' == ch || 'Y' == ch) {           // yes
            CConsoleUpdater updater(*this);

            std::cout << "\ninstalling now ...\n" << std::endl;
            if (owner.InstallNow(updater, true)) {
                return 1;
            }
            return 0;

        } else if ('s' == ch || 'S' == ch) {    // skip
            if (is_critical) continue;
            std::cout << "\nskipping release ...\n" << std::endl;
            owner.InstallSkip();
            return 0;

        } else if ('l' == ch || 'L' == ch) {    // later
            std::cout << "\nshall prompt again later ...\n" << std::endl;
            owner.InstallLater();
            return 0;

        } else if ('r' == ch || 'R' == ch) {
            //  --- show release notes ---

        } else if (0x1b == ch) {                // cancel
            std::cout << std::endl;
            break;
        }
    }
    return -1;
}


void
AutoConsoleUI::UptoDateDialog(AutoUpdater &owner)
{
    std::cout << owner.AppName();
    std::cout << " is up-to-date, you are running version ";
    std::cout << owner.AppVersion();
    std::cout << std::endl;
}


void
AutoConsoleUI::WarningMessage(const char *message)
{
  //::MessageBoxA(NULL, message, "AutoUpdater", MB_ICONWARNING|MB_OK);
    std::cout << "WARNING: " << message << std::endl;
}


void
AutoConsoleUI::ErrorMessage(const char *message)
{
  //::MessageBoxA(NULL, message, "AutoUpdater", MB_ICONERROR|MB_OK);
    std::cout << "ERROR: " << message << std::endl;
}


void
AutoConsoleUI::ProgressStart(AutoUpdater & /*owner*/, HWND /*parent*/, bool /*indeterminate*/, const char *msg)
{
    if (0 == progress_) {
        if (TProgressBar *progress = new(std::nothrow) TProgressBar()) {
            progress->SetTextMsg(msg);
            progress->SetCancelMsg("Please wait while the operation completes ...");
            progress->Start(false, true);
            progress_ = (void *)progress;
        }
    }
}


void
AutoConsoleUI::ProgressUpdate(int completed, int total)
{
    TProgressBar *progress;
    if (NULL != (progress = (TProgressBar *)progress_)) {
        progress->SetProgress(completed, total);
    }
}


bool
AutoConsoleUI::ProgressCancelled()
{
    TProgressBar *progress;
    if (NULL != (progress = (TProgressBar *)progress_)) {
        return progress->HasUserCancelled();
    }
    return false;
}


bool
AutoConsoleUI::ProgressStop()
{
    TProgressBar *progress;
    bool cancelled = false;
    if (NULL != (progress = (TProgressBar *)progress_)) {
        cancelled = progress->HasUserCancelled();
        progress->Stop();
        progress->Release();
        progress_ = NULL;
    }
    return cancelled;
}

//end
