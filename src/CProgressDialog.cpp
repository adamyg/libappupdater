//  $Id: CProgressDialog.cpp,v 1.19 2025/02/21 19:03:23 cvsuser Exp $
//
//  AutoUpdater: Progress dialog.
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
//

#include "common.h"

#include <assert.h>
#include <ShlObj.h>
#include <ShObjidl.h>

#include "AutoLogger.h"
#include "AutoThread.h"

#include "CProgressDialog.h"
#include "resource.h"

using namespace Updater;

#if !defined(PROGDLG_MARQUEEPROGRESS)
#define PROGDLG_MARQUEEPROGRESS 0x00000020L
#define PROGDLG_NOCANCEL 0x00000040L
#endif

#define WM_DLG_UPDATE   (WM_APP + 1)            // update dialog content.
#define WM_DLG_DESTROY  (WM_APP + 2)            // self destroy.
#define IDT_TIMER1      (WM_APP + 3)            // delay timer identifier.

struct create_params {
    HINSTANCE module;                           // module instance.
    HWND parent;                                // parent window.
    HANDLE trigger;                             // start completion trigger.
    CProgressDialog *self;                      // our instance.
};

static DWORD WINAPI     dialog_thread(LPVOID lpParameter);
static INT_PTR CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


///////////////////////////////////////////////////////////////////////////////
//  ProcessDialog
//

CProgressDialog::CProgressDialog(HINSTANCE hInst)
    : d_references(1), d_hInst(hInst), d_hWnd(0), d_cancelled(false), d_complete(0), d_total(0), d_speed(100)
{
}


CProgressDialog::~CProgressDialog()
{
    assert(0 == d_references);
}


void
CProgressDialog::SetTitle(LPCWSTR text)
{
    if (text) {
        HWND hWnd;

        {   CriticalSection::Guard guard(d_lock);
            hWnd = Window();
            d_title = text;
            d_dirty |= DIRTY_TITLE;
        }

        if (hWnd) {
            SendMessageW(hWnd, WM_DLG_UPDATE, 0, 0);
        }
    }
}


void
CProgressDialog::SetAnimationSpeed(DWORD speed)
{
    if (0 == speed) speed = 30; // default
    d_speed = speed;
}


void
CProgressDialog::SetLine(DWORD dwLineNum, LPCWSTR text, BOOL /*compat*/, void * /*reserved*/)
{
    if (text) {
        HWND hWnd = 0;

        {   CriticalSection::Guard guard(d_lock);
            if (1 == dwLineNum || 2 == dwLineNum ||
                    (3 == dwLineNum && 0 == (d_dwFlags & PROGDLG_AUTOTIME))) {
                d_lines[--dwLineNum] = text;
                d_dirty |= DIRTY_LINE1 << dwLineNum;
                hWnd = Window();
            }
        }

        if (hWnd) {
            SendMessageW(hWnd, WM_DLG_UPDATE, 0, 0);
        }
    }
}


void
CProgressDialog::SetCancelMsg(LPCWSTR text, void * /*reserved*/)
{
    if (text) {
        HWND hWnd = 0;

        {   CriticalSection::Guard guard(d_lock);
            if (d_cancelled) hWnd = Window();
            d_cancelmsg = text;
        }

        if (hWnd) {
            ::SendMessageW(hWnd, WM_DLG_UPDATE, 0, 0);
        }
    }
}


void
CProgressDialog::StartProgressDialog(HWND parent, void * /*reserved1*/, DWORD dwFlags, void * /*reserved2*/)
{
    struct create_params params = {0};
    HANDLE hThread = 0;

    {   CriticalSection::Guard guard(d_lock);

        if (d_hWnd) return;
        d_dwFlags = dwFlags;
        params.self = this;
        params.module = (d_hInst ? d_hInst : ::GetModuleHandle(NULL));
        params.parent = parent;
        params.trigger = ::CreateEventW(NULL, TRUE, FALSE, NULL);
        assert(params.trigger);
        if (params.trigger) {
            if (0 != (hThread = ::CreateThread(NULL, 0, dialog_thread, &params, 0, NULL))) {
                ++d_references;
            }
            assert(hThread);
        }
    }

    if (hThread) {
        MSG msg = {0};

        for (bool done = false; !done;) {       // message pump loop.
            if (MsgWaitForMultipleObjects(1, &params.trigger,
                        FALSE, 100, QS_ALLEVENTS) == WAIT_OBJECT_0) {
                done = true;                    // trigger complete or quit.
            }

            while (::PeekMessage(&msg, (HWND)(-1), 0, 0, PM_NOREMOVE)) {
                if (msg.message == WM_QUIT) {
                    done = true;
                    break;
                }
            }
        }
        ::CloseHandle(hThread);
    }
    ::CloseHandle(params.trigger);
}


void
CProgressDialog::SetProgress(DWORD complete, DWORD total)
{
    HWND hWnd;

    {   CriticalSection::Guard guard(d_lock);
        hWnd = Window();
        d_complete = complete; d_total = total;
        d_dirty |= DIRTY_PROGRESS;
    }

    if (hWnd) {
        ::SendMessageW(hWnd, WM_DLG_UPDATE, 0, 0);
    }
}


void
CProgressDialog::StopProgressDialog()
{
    HWND hWnd = 0;

    {   CriticalSection::Guard guard(d_lock);
        if (0 != (hWnd = Window())) {
            SetWindow(0);
        }
    }

    if (hWnd) {
        ::SendMessageW(hWnd, WM_DLG_DESTROY, 0, 0);
    }
}


BOOL
CProgressDialog::HasUserCancelled()
{
    return (d_cancelled ? TRUE : FALSE);
}


void
CProgressDialog::Release()
{
    unsigned t_references;
    {   CriticalSection::Guard guard(d_lock);
        assert(d_references);
        t_references = --d_references;
    }
    if (0 == t_references) {
        delete this;
    }
}


static int
progress(char *buffer, int buflen, int complete, int total) 
{
    const char *suffix[] = {"B", "KB", "MB", "GB", "TB"};

    if (total > 0 && complete >= 0) {
        const int percentage = 
                (int)(((double)complete / total) * 100.0);
        double unit = (double)(total);
        int s = 0;

        if (total > 1024) {
            while ((total / 1024) > 0 && s < static_cast<int>(_countof(suffix) - 1)) {
                unit = total / 1024.0;
                total /= 1024;
                ++s;
            }
        }
        return sprintf_s(buffer, buflen, "Downloaded %d%% of %.02lf%s ", percentage, unit, suffix[s]);
    }

    buffer[0] = 0;
    return 0;
}


void
CProgressDialog::Update(DWORD dirty)
{
    dirty |= d_dirty, d_dirty = 0;

    if (dirty & DIRTY_TITLE) ::SetWindowTextW(d_hWnd, d_title.c_str());
    if (dirty & DIRTY_LINE1) ::SetDlgItemTextW(d_hWnd, IDC_PROGRESS_TEXT1, (d_cancelled ? L"" : d_lines[0].c_str()));
    if (dirty & DIRTY_LINE2) ::SetDlgItemTextW(d_hWnd, IDC_PROGRESS_TEXT2, (d_cancelled ? L"" : d_lines[1].c_str()));
    if (dirty & DIRTY_LINE3) {
        if (d_cancelled) {
            ::SetDlgItemTextW(d_hWnd, IDC_PROGRESS_TEXT3, d_cancelmsg.c_str());
        } else if (0 == d_total && 0 == d_complete) {
            ::SetDlgItemTextW(d_hWnd, IDC_PROGRESS_TEXT3, d_lines[2].c_str());
        }
    }

    if (dirty & DIRTY_PROGRESS) {
        char buffer[64];

        if (!d_cancelled && progress(buffer, sizeof(buffer), d_complete, d_total)) {
            ::SetDlgItemTextA(d_hWnd, IDC_PROGRESS_TEXT3, buffer);
        }

        if (0 == (Flags() & PROGDLG_MARQUEEPROGRESS)) {
            ULONGLONG total = d_total, complete = d_complete;
            while (total >> 32) {
                total >>= 1, complete >>= 1;
            }
            ::SendDlgItemMessageW(d_hWnd, IDC_PROGRESS_BAR, PBM_SETRANGE32, 0, (DWORD)total);
            ::SendDlgItemMessageW(d_hWnd, IDC_PROGRESS_BAR, PBM_SETPOS, (DWORD)complete, 0);
        }
    }
}


static DWORD WINAPI
dialog_thread(LPVOID lpParameter)
{
    struct create_params params = *((struct create_params *)lpParameter); //copy
    CProgressDialog *me = params.self;
    HWND hWnd;

    hWnd = CreateDialogParamW(params.module, MAKEINTRESOURCE(IDD_PROGRESS_DIALOG),
                    params.parent, dialog_proc, (LPARAM)&params);
    if (hWnd) {                                 // dialog message loop.
        MSG msg;

        while (::GetMessage(&msg, NULL, 0, 0) > 0) {
            if (! IsWindow(hWnd))
                break;
            if (! ::IsDialogMessage(hWnd, &msg)) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
        }
    } else {
        LOG<LOG_INFO>()
            << "AutoUpdate::Progress() creation error=" << GetLastError() << LOG_ENDL;
        if (params.trigger) {                   // WM_INITDIALOG exec'd ?
            ::SetEvent(params.trigger);
        }
    }
    me->Release();
    return 0;
}


static void
SetMarquee(HWND hWnd, DWORD speed)
{
    HWND hProgress = GetDlgItem(hWnd, IDC_PROGRESS_BAR);
    ::SetWindowLong(hProgress, GWL_STYLE, ::GetWindowLong(hProgress, GWL_STYLE)|PBS_MARQUEE);
    ::SendMessage(hProgress, PBM_SETMARQUEE, (speed ? 1 : 0), speed);
}


static void
EnableWindow(HWND hWnd, DWORD dwFlags, DWORD speed)
{
    ::SetWindowLong(hWnd, GWL_STYLE, ::GetWindowLong(hWnd, GWL_STYLE) | WS_VISIBLE);
    ::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_INTERNALPAINT);
    if (dwFlags & PROGDLG_MARQUEEPROGRESS) {
        SetMarquee(hWnd, speed);
    }
    ::SetFocus(hWnd);
}


static INT_PTR CALLBACK
dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CProgressDialog *self =
        static_cast<CProgressDialog *>((void *)::GetWindowLongPtr(hWnd, DWLP_USER));

    switch (msg) {
    case WM_INITDIALOG: {
            struct create_params *params = (struct create_params *)lParam;
            DWORD dwFlags;

            self = params->self;                // associate dialog.
            self->SetWindow(hWnd);
            ::SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)self);
            dwFlags = self->Flags();

            ::ShowWindow(GetDlgItem(hWnd, IDC_PROGRESS_BAR), (dwFlags & PROGDLG_NOPROGRESSBAR) ? SW_HIDE : SW_NORMAL);
            if (dwFlags & PROGDLG_NOCANCEL)
                ::ShowWindow(GetDlgItem(hWnd, IDCANCEL), SW_HIDE);
            if (dwFlags & PROGDLG_NOMINIMIZE)
                ::SetWindowLong(hWnd, GWL_STYLE, GetWindowLongW(hWnd, GWL_STYLE) & (~WS_MINIMIZEBOX));

            self->Update((DWORD)-1);            // apply fields.

            if (! SetTimer(hWnd, IDT_TIMER1, 250 /*ms*/, NULL)) {
                EnableWindow(hWnd, dwFlags);
            }

            HANDLE trigger = params->trigger;   // startup complete event.
            params->trigger = (HANDLE)0;        // stop re-trigger.
            SetEvent(trigger);
        }
        return TRUE;

    case WM_TIMER:
        if (IDT_TIMER1 == wParam) {             // display delay timer.
            EnableWindow(hWnd, self->Flags(), self->MarqueeAnimationSpeed());
            ::KillTimer(hWnd, IDT_TIMER1);
            return TRUE;
        }
        break;

    case WM_DLG_UPDATE: {
            CriticalSection::Guard(self->Lock());
            self->Update();
        }
        return TRUE;

    case WM_DLG_DESTROY:
        if (0 == (::GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE)) {
            ::KillTimer(hWnd, IDT_TIMER1);
        }
        DestroyWindow(hWnd);
        ::PostThreadMessage(GetCurrentThreadId(), WM_NULL, 0, 0);
        self->SetWindow((HWND) 0);
        return TRUE;

    case WM_CLOSE:
    case WM_COMMAND:
        if (msg == WM_CLOSE || wParam == IDCANCEL) {
            CriticalSection::Guard(self->Lock());

            self->Cancelled();
            SetMarquee(hWnd, 0);
            EnableWindow(GetDlgItem(hWnd, IDCANCEL), FALSE);
            self->Update(CProgressDialog::DIRTY_LINE1|CProgressDialog::DIRTY_LINE2|CProgressDialog::DIRTY_LINE3);
        }
        return TRUE;
    }
    return FALSE;
}

//end
