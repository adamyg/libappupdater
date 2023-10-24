//  $Id: AutoDialog.cpp,v 1.17 2023/10/24 13:56:23 cvsuser Exp $
//
//  AutoUpdater: dialog interface
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

#include <assert.h>
#include <ShObjIdl.h>
#include <ShlObj.h>

#include "AutoDialog.h"
#include "AutoError.h"
#include "AutoLogger.h"
#include "AutoThread.h"

#include "CUpdateInstallDlg.h"
#include "CUpdatePromptDlg.h"
#include "CUptodateDlg.h"
#include "CProgressDialog.h"
#include "resource.h"

using namespace Updater;
    
    //#define USE_IPROGRESSDIALOG
#if !defined(PROGDLG_MARQUEEPROGRESS)
#define PROGDLG_MARQUEEPROGRESS 0x00000020L
#define PROGDLG_NOCANCEL 0x00000040L
#endif
#if defined(USE_IPROGRESSDIALOG)
#define IPROGRESSDIALOG IProgressDialog
#else
#define IPROGRESSDIALOG CProgressDialog
#endif


/////////////////////////////////////////////////////////////////////////////////////////
//  AutoDialogUI
//

AutoDialogUI::AutoDialogUI() : progress_(NULL)
{
}


AutoDialogUI::~AutoDialogUI()
{
    ProgressStop();
}


enum PromptResponse
AutoDialogUI::PromptDialog(AutoUpdater &owner)
{
    CUpdatePromptDlg prompt(owner);
    const INT_PTR nResponse = prompt.DoModal();
    if (nResponse == IDC_CHECK_ONCE || nResponse == IDOK) {
        return PROMPT_ONCE;
    } else if (nResponse == IDC_CHECK_AUTO) {
        return PROMPT_AUTO;
    } else if (nResponse == IDC_CHECK_NO) {
        return PROMPT_NO;
    }
    return PROMPT_CANCEL;
}


int
AutoDialogUI::InstallDialog(AutoUpdater &owner)
{
    CUpdateInstallDlg dialog(owner, true);
    const INT_PTR nResponse = dialog.DoModal();
    if (nResponse != IDCANCEL) {
        return 1;
    }
    return -1;
}


void
AutoDialogUI::UptoDateDialog(AutoUpdater &owner)
{
 // CUpdateInstallDlg dialog(owner, false);
    CUpdateUptoDateDlg dialog(owner);
    dialog.DoModal();
}


void
AutoDialogUI::WarningMessage(const char *message)
{
    ::MessageBoxA(NULL, message, "AutoUpdater", MB_ICONWARNING|MB_OK);
}


void
AutoDialogUI::ErrorMessage(const char *message)
{
    ::MessageBoxA(NULL, message, "AutoUpdater", MB_ICONERROR|MB_OK);
}


void
AutoDialogUI::ProgressStart(AutoUpdater &owner, HWND parent, bool indeterminate, const char *msg)
{
    if (0 == progress_) {
        IPROGRESSDIALOG *progress = NULL;
        DWORD dwFlags = PROGDLG_NORMAL|PROGDLG_NOMINIMIZE;
        HWND hWndParent = (parent ? parent : GetDesktopWindow());
     // HWND hWndParent = GetDesktopWindow();

#if defined(USE_IPROGRESSDIALOG)                // windows Vista and later
        HRESULT hr;
        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        hr = CoCreateInstance (CLSID_ProgressDialog,
                    NULL, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (void**) &progress);
#else                                           // generic
        progress = new(std::nothrow) IPROGRESSDIALOG(owner.ModuleHandle());
#endif
        if (progress) {
            progress->SetTitle(L"AutoUpdater ...");
            if (msg) {
                progress->SetLine(1, Updater::to_wstring(msg).c_str(), FALSE, NULL);
            }
            progress->SetCancelMsg(L"Please wait while the operation completes ...", NULL);
            if (indeterminate) {
                dwFlags |= PROGDLG_NOTIME|PROGDLG_MARQUEEPROGRESS;
            }
            progress->StartProgressDialog(hWndParent, NULL, dwFlags, NULL);
            progress_ = (void *)progress;
        }
    }
}


void
AutoDialogUI::ProgressUpdate(int completed, int total)
{
    IPROGRESSDIALOG *progress;
    if (NULL != (progress = (IPROGRESSDIALOG *)progress_)) {
        progress->SetProgress(completed, total);
        //progress->SetProgress64():
    }
}


bool        
AutoDialogUI::ProgressCancelled()
{
    IPROGRESSDIALOG *progress;
    if (NULL != (progress = (IPROGRESSDIALOG *)progress_)) {
        return (progress->HasUserCancelled() ? true : false);
    }
    return false;
}


bool
AutoDialogUI::ProgressStop()
{
    IPROGRESSDIALOG *progress;
    bool cancelled = false;
    if (NULL != (progress = (IPROGRESSDIALOG *)progress_)) {
        cancelled = (progress->HasUserCancelled() != FALSE);
        progress->StopProgressDialog();
        progress->Release();
        progress_ = NULL;
    }
    return cancelled;
}


/////////////////////////////////////////////////////////////////////////////////////////
//  AutoDialog
//

AutoDialog::AutoDialog(int idd, AutoUpdater &owner, HWND hParent)
    : d_idd(idd), d_owner(owner), d_hWnd(0), d_hParentWnd(hParent)
{
}


AutoDialog::~AutoDialog()
{  
}


INT_PTR
AutoDialog::DoModal()
{
    return ::DialogBoxParam(d_owner.ModuleHandle(), MAKEINTRESOURCE(d_idd),
                    d_hParentWnd, AutoDialog::dialog_proc, (LPARAM)this);
}


void
AutoDialog::EndDialog(UINT result)
{
    ::EndDialog(d_hWnd, result);
}


//static BOOL CALLBACK
//AboutBoxProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM /*lParam*/) 
//{
//    switch (msg) {
//    case WM_COMMAND:
//        switch (LOWORD(wParam)) { 
//        case IDOK:
//        case IDCANCEL:
//            ::EndDialog(hWnd, wParam);
//            return TRUE; 
//        }
//    }
//    return FALSE; 
//} 


INT_PTR CALLBACK
AutoDialog::dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    AutoDialog *self =
        static_cast<AutoDialog *>((void *)::GetWindowLongPtr(hWnd, DWLP_USER));

    switch (msg) {
    case WM_INITDIALOG: {
            self = static_cast<AutoDialog *>((void *)lParam);
            self->d_hWnd = hWnd;
            ::SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)self);
        }
        return self->OnInitDialog();
    case WM_GETMINMAXINFO:
        return self->OnGetMinMaxInfo((MINMAXINFO *)lParam);
    case WM_CTLCOLOREDIT:
        return self->OnCtlColorEdit(wParam, lParam);
    case WM_CTLCOLORSTATIC:
        return self->OnCtlColorStatic(wParam, lParam);
    case WM_COMMAND:
        return self->OnCommand(wParam, lParam);
    case WM_SYSCOMMAND:
        self->OnSysCommand(wParam, lParam);
        break;
    case WM_PAINT:
        self->OnPaint();
        break;
    case WM_SIZE: {
            const int cx = LOWORD(lParam), //width
                cy = HIWORD(lParam); //height
            self->OnSize((UINT)wParam, cx, cy);
        }
        break;
    case WM_QUERYDRAGICON:
        return (INT_PTR)self->OnQueryDragIcon();
    case WM_TIMER:
        break;
    case WM_DESTROY:
        self->OnDestroy();
        break;
    case WM_CLOSE:
        self->OnClose();
        return TRUE;
    }
    return FALSE;
}


//virtual
BOOL
AutoDialog::OnInitDialog()
{
    return TRUE; // return TRUE unless you set the focus to a control
}


//virtual
BOOL
AutoDialog::OnGetMinMaxInfo(MINMAXINFO * /*mmi*/)
{
    return FALSE;
}


//virtual
HCURSOR
AutoDialog::OnQueryDragIcon()
{
    return (HCURSOR)0;
}


//virtual
BOOL
AutoDialog::OnCtlColorEdit(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return FALSE;
}


//virtual
LRESULT   
AutoDialog::OnCtlColorStatic(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return 0;
}


//virtual
BOOL
AutoDialog::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
    switch (LOWORD(wParam)) { 
    case IDOK: 
        OnOK();
        return TRUE; 
    case IDCANCEL: 
        OnCANCEL();
        return TRUE; 
    }
    return FALSE;
}


//virtual
void
AutoDialog::OnSysCommand(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
}


//virtual
void        
AutoDialog::OnCANCEL()
{
    EndDialog(IDCANCEL);
}


//virtual
void
AutoDialog::OnOK()
{
    EndDialog(IDOK);
}


//virtual
void
AutoDialog::OnPaint()
{
}


//virtual
void
AutoDialog::OnSize(UINT /*type*/, int /*cx*/, int /*cy*/)
{
}


//virtual
void
AutoDialog::OnDestroy()
{
}


//virtual
void
AutoDialog::OnClose()
{
    ::DestroyWindow(d_hWnd);
}


/////////////////////////////////////////////////////////////////////////////////////////
//  AFont
//

AFont::AFont() : font_(0)
{ 
}


AFont::AFont(int height, const char *name, HWND hWnd) : font_(0)
{
    AFont::CreatePointFont(height, name, hWnd);
}


AFont::AFont(int height, const wchar_t *name, HWND hWnd) : font_(0)
{
    AFont::CreatePointFont(height, name, hWnd);
}


AFont::~AFont()
{
    if (font_) {
        ::DeleteObject(font_);
    }
}


bool
AFont::CreatePointFont(int height, const char *name, HWND hWnd)
{
    HDC hdc = (hWnd ? GetDC(hWnd) : GetDC(NULL));
    INT nFontHeight = ::MulDiv(height, ::GetDeviceCaps(hdc, LOGPIXELSY), 720);

    font_= ::CreateFontA(nFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, name);

    ::ReleaseDC(hWnd, hdc);
    return true;
}


bool
AFont::CreatePointFont(int height, const wchar_t *name, HWND hWnd)
{
    HDC hdc = (hWnd ? GetDC(hWnd) : GetDC(NULL));
    INT nFontHeight = ::MulDiv(height, ::GetDeviceCaps(hdc, LOGPIXELSY), 720);

    font_= ::CreateFontW(nFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, name);

    ::ReleaseDC(hWnd, hdc);
    return true;
}


bool
AFont::SetWindowFont(HWND hWnd)
{
    if (hWnd && font_) {
        ::SendMessage(hWnd, WM_SETFONT, (WPARAM)font_, 0);
        return true;
    }
    assert(false);
    return false;
}


bool
AFont::SetDlgItemFont(HWND hWnd, int nItem)
{
    if (hWnd && font_) {
        ::SendMessage(::GetDlgItem(hWnd, nItem), WM_SETFONT, (WPARAM)font_, 0);
        return true;
    }
    assert(false);
    return false;
}

//end
