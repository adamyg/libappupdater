//  $Id: CUpdatePromptDlg.cpp,v 1.11 2021/08/17 15:27:10 cvsuser Exp $
//
//  AutoUpdater: Prompt dialog.
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

#include "common.h"

#include <assert.h>

#include "AutoThread.h"
#include "AutoUpdater.h"

#include "CUpdatePromptDlg.h"
#include "resource.h"


///////////////////////////////////////////////////////////////////////////////
//  AboutDlg
//

namespace UpdaterPrompt {

class AboutDlg : public AutoDialog {
public:
    AboutDlg(AutoUpdater &owner, HWND hParent) :
        AutoDialog(IDD, owner, hParent) { }
    enum { IDD = IDD_UPDATE_ABOUT };
};

}   // namespace UpdaterPrompt


///////////////////////////////////////////////////////////////////////////////
//  CUpdatePromptDlg
//

CUpdatePromptDlg::CUpdatePromptDlg(AutoUpdater &owner)
    : AutoDialog(IDD, owner)
{
    d_bold.CreatePointFont(UPDATE_BOLDSIZE, UPDATE_BOLDFONT);
    d_norm.CreatePointFont(UPDATE_NORMSIZE, UPDATE_NORMFONT);
}


CUpdatePromptDlg::~CUpdatePromptDlg()
{
}


BOOL
CUpdatePromptDlg::OnInitDialog()
{
    AutoDialog::OnInitDialog();
    HWND hWnd = GetSafeHwnd();

    // about box; if sysmenu enabled
    assert((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    assert(IDM_ABOUTBOX < 0xF000);
    {   HMENU hMenu = ::GetSystemMenu(hWnd, FALSE);
        if (hMenu) {
            TCHAR t_buffer[128];
            if (::LoadString(d_owner.ModuleHandle(), IDS_ABOUTBOX, t_buffer, _countof(t_buffer))) {
                ::AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                ::AppendMenu(hMenu, MF_STRING, IDM_ABOUTBOX, t_buffer);
            }
        }
    }

    // fields
    AString notes;
    notes += "Allow ";
    notes += d_owner.AppName();
    notes += " to automatically check for updates?\r\n";
    notes += "You may also check once and disable/enable at a later time.";

    ::SetDlgItemTextW(hWnd, IDC_PROMPT_TITLE, L"Check for updates automatically?");
    d_bold.SetDlgItemFont(hWnd, IDC_PROMPT_TITLE);

    ::SetDlgItemTextW(hWnd, IDC_PROMPT_NOTES, notes.c_str());
    d_norm.SetDlgItemFont(hWnd, IDC_PROMPT_NOTES);

    // buttons
    ::ShowWindow(::GetDlgItem(hWnd, IDC_CHECK_ONCE), SW_NORMAL);
    ::ShowWindow(::GetDlgItem(hWnd, IDC_CHECK_NO), SW_NORMAL);
    ::ShowWindow(::GetDlgItem(hWnd, IDC_CHECK_AUTO), SW_NORMAL);

    return TRUE; // return TRUE unless you set the focus to a control
}


//virtual
BOOL
CUpdatePromptDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam)) { 
    case IDC_CHECK_ONCE:
        OnBnClickedCheckOnce();
        return TRUE;
    case IDC_CHECK_AUTO:
        OnBnClickedCheckAuto();
        return TRUE;
    case IDC_CHECK_NO:
        OnBnClickedCheckNo();
        return TRUE;
    }
    return AutoDialog::OnCommand(wParam, lParam);
}


//virtual
void
CUpdatePromptDlg::OnSysCommand(WPARAM wParam, LPARAM lParam)
{
    if ((wParam & 0xFFF0) == IDM_ABOUTBOX) {
        UpdaterPrompt::AboutDlg about(d_owner, d_hWnd);
        about.DoModal();
    } else {
        AutoDialog::OnSysCommand(wParam, lParam);
    }
}


//virtual
LRESULT
CUpdatePromptDlg::OnCtlColorStatic(WPARAM wParam, LPARAM lParam)
{
    if (::GetDlgItem(d_hWnd, IDC_PROMPT_TITLE) == (HWND)lParam) {
        ::SetTextColor((HDC)wParam, RGB(0x56, 0x96, 0xBC));     // Steel Blue.
        return (LRESULT) ::GetSysColorBrush(COLOR_3DFACE);      // Dialog backgrond.
    }
    return 0;
}


void
CUpdatePromptDlg::OnBnClickedCheckOnce()
{
    EndDialog(IDC_CHECK_ONCE);
}


void
CUpdatePromptDlg::OnBnClickedCheckAuto()
{
    EndDialog(IDC_CHECK_AUTO);
}


void
CUpdatePromptDlg::OnBnClickedCheckNo()
{
    EndDialog(IDC_CHECK_NO);
}
