#pragma once
//  $Id: CProgressDialog.h,v 1.4 2021/08/14 15:38:10 cvsuser Exp $
//
//  AutoUpdater: progress dialog.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2021 Adam Young
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

#include "AutoMisc.h"

#if !defined(WINDOWS_MEAN_AND_LEAN)
#define  WINDOWS_MEAN_AND_LEAN
#endif
#include <Windows.h>

class CProgressDialog {
public:
    CProgressDialog(HINSTANCE hInst = 0);
    ~CProgressDialog();

    void                SetTitle(LPCWSTR text);
    void                SetLine(DWORD dwLineNum, LPCWSTR text, BOOL compat, void *reserved);
    void                SetCancelMsg(LPCWSTR text, void *reserved);
    void                StartProgressDialog(HWND parent, void *reserved1, DWORD dwFlags, void *reserved2);
    void                SetProgress(DWORD completed, DWORD total);
    void                StopProgressDialog();
    BOOL                HasUserCancelled();
    void                Release();

    //implementation
    void                Update(DWORD dirty = 0);
    HWND                Window()                { return d_hWnd; }
    void                SetWindow(HWND hWnd)    { d_hWnd = hWnd; }

    DWORD               Flags()                 { return d_dwFlags; }
    void                Cancelled()             { d_cancelled = true; }

    Updater::CriticalSection& Lock()            { return d_lock; }

public:
    enum {
        DIRTY_TITLE     = (1 << 0),
        DIRTY_LINE1     = (1 << 1),
        DIRTY_LINE2     = (1 << 2),
        DIRTY_LINE3     = (1 << 3),
        DIRTY_PROGRESS  = (1 << 4)
    };

private:
    Updater::CriticalSection d_lock;
    unsigned            d_references;
    HINSTANCE           d_hInst;
    HWND                d_hWnd;
    DWORD               d_dwFlags;
    std::wstring        d_title;
    std::wstring        d_lines[3];
    std::wstring        d_cancelmsg;
    bool                d_cancelled;
    DWORD               d_complete, d_total;
    DWORD               d_dirty;
};

//end
