//  $Id: CUptodateDlg.cpp,v 1.12 2022/06/09 08:46:31 cvsuser Exp $
//
//  AutpUpdater: Up-to-date dialog.
//
//  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
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

#include "CUptodateDlg.h"
#include "resource.h"


CUpdateUptoDateDlg::CUpdateUptoDateDlg(AutoUpdater &owner)
    : AutoDialog(IDD, owner)
{
    d_bold.CreatePointFont(UPDATE_BOLDSIZE, UPDATE_BOLDFONT);
    d_norm.CreatePointFont(UPDATE_NORMSIZE, UPDATE_NORMFONT);
}


CUpdateUptoDateDlg::~CUpdateUptoDateDlg()
{
}


//virtual
BOOL
CUpdateUptoDateDlg::OnInitDialog()
{
    AutoDialog::OnInitDialog();
    HWND hWnd = GetSafeHwnd();

    AString notes;
    notes += d_owner.AppName();
    notes += L" is up-to-date, you are running version ";
    notes += d_owner.AppVersion();

#if (TODO)
    if (! d_owner.Manifest().channel.empty() &&
          d_owner.Manifest().channel.empty() != "release") {
        notes += _T("\r\n");
        notes += _T("You are on the \"");
        Updater::Append(notes, d_owner.Manifest().channel);
        notes += _T("\" channel");
    }
#endif

    ::SetDlgItemTextW(hWnd, IDC_UPTODATE_TITLE, L"Application is up to date");
    d_bold.SetDlgItemFont(hWnd, IDC_UPTODATE_TITLE);

    ::SetDlgItemTextW(hWnd, IDC_UPTODATE_NOTES, notes.c_str());
    d_norm.SetDlgItemFont(hWnd, IDC_UPTODATE_NOTES);

    return TRUE; // return TRUE unless you set the focus to a control
}


//virtual
LRESULT
CUpdateUptoDateDlg::OnCtlColorStatic(WPARAM wParam, LPARAM lParam)
{
    if (::GetDlgItem(d_hWnd, IDC_UPTODATE_TITLE) == (HWND)lParam) {
        ::SetTextColor((HDC)wParam, RGB(0x56, 0x96, 0xBC));     // Steel Blue.
        return (LRESULT) ::GetSysColorBrush(COLOR_3DFACE);      // Dialog backgrond.
    }
    return 0;
}
