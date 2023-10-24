//  $Id: CUpdateInstallDlg.cpp,v 1.21 2023/10/24 13:56:23 cvsuser Exp $
//
//  AutoUpdater: Install dialog.
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

#include <time.h>
#include <assert.h>
#include <algorithm>

#include "AutoLogger.h"
#include "AutoThread.h"
#include "AutoDialog.h"

#include "CUpdateInstallDlg.h"
#include "CSimpleBrowser.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////////////////
//  AboutDlg

namespace UpdaterInstall {

class AboutDlg : public AutoDialog {
public:
    AboutDlg(AutoUpdater &owner, HWND hParent) :
        AutoDialog(IDD, owner, hParent) { }
    enum { IDD = IDD_UPDATE_ABOUT };
};

}   //anon namespace


/////////////////////////////////////////////////////////////////////////////////////////
//  CDialogUpdater
//

using namespace Updater;

class CDialogUpdater : public IInstallNow {
public:
    CDialogUpdater(CUpdateInstallDlg &owner) : owner_(owner) {
    }

    virtual void operator()(const char *message) {
        owner_.d_edit_result.SetText(message);
        owner_.d_edit_result.SetFont(owner_.d_bold);
        LOG<LOG_TRACE>() << "MSG: " << message << LOG_ENDL;
    }

    virtual HWND GetParent() {
        return owner_.GetSafeHwnd();
    }

    CUpdateInstallDlg &owner_;
};


#if defined(_MSC_VER)
#pragma warning(disable:4355) //'this' : used in base member initializer list.
#endif

/////////////////////////////////////////////////////////////////////////////////////////
//  CUpdateInstallDlg dialog
//
CUpdateInstallDlg::CUpdateInstallDlg(AutoUpdater &owner, bool prompt)
    : AutoDialog(CUpdateInstallDlg::IDD, owner),
        d_owner(owner), d_prompt(prompt),
        d_norm(0), d_bold(0),
        d_browser(0),
        d_szMinimum(0, 0),
        d_szInitial(0, 0),
        d_hGripper(0),
        d_notes(IDC_INSTALL_NOTES, *this),
        d_edit_action(IDC_INSTALL_ACTION, *this),
        d_edit_result(IDC_INSTALL_RESULT, *this),
        d_edit_summary(IDC_INSTALL_SUMMARY, *this),
        d_edit_title(IDC_INSTALL_RNTITLE, *this),
        d_button_skip(IDC_INSTALL_SKIP, *this),
        d_button_auto(IDC_INSTALL_AUTO, *this),
        d_button_now(IDC_INSTALL_NOW, *this),
        d_button_later(IDC_INSTALL_LATER, *this),
        d_button_ok(IDOK, *this)
{
    d_norm = new AFont;
    d_norm->CreatePointFont(UPDATE_NORMSIZE, UPDATE_NORMFONT);
    d_bold = new AFont;
    d_bold->CreatePointFont(UPDATE_BOLDSIZE, UPDATE_BOLDFONT);
}


CUpdateInstallDlg::~CUpdateInstallDlg()
{
    /*::if (CloseHandle(d_hGripper), we are not the owner*/
    delete d_browser;
    delete d_norm;
    delete d_bold;
}


BOOL
CUpdateInstallDlg::OnInitDialog()
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

    SetIcon(d_hIcon, TRUE);                     // 32x32 icon

    // Resize support
    // .. keep the initial size of the client area as a baseline for moving/sizing controls
    // .. use the initial dialog size as the default minimum
    ARect rcClient;
    GetClientRect(rcClient);
    d_szInitial = rcClient.Size();
    if (0 == d_szMinimum.cx) d_szMinimum = d_szInitial;

    // .. create a gripper in the bottom-right corner, THICKFRAME implied
    // Note, WS_THICKFRAME maybe/is lost if the resource was edited via VisualStudio
    if (d_prompt) {
        assert(GetStyle() & (WS_SIZEBOX|WS_THICKFRAME));
        if (0 != ((GetStyle() & (WS_SIZEBOX|WS_THICKFRAME)))) {
            ARect sz;

            sz.SetRect(-::GetSystemMetrics(SM_CXVSCROLL), -::GetSystemMetrics(SM_CYHSCROLL), 0, 0);
            sz.OffsetRect(rcClient.BottomRight());
            d_hGripper = CreateWindow(_T("Scrollbar"), _T("size"),  WS_CHILD | WS_VISIBLE | SBS_SIZEGRIP,
                                sz.left, sz.top, sz.Width(), sz.Height(), hWnd, NULL, d_owner.ModuleHandle(), NULL);
            if (d_hGripper) {
                // put the gripper first in the z-order so it paints first and doesn't obscure other controls
                ::SetWindowPos(d_hGripper, HWND_TOP, 0, 0, 0, 0,
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
                AutoMove(-1, SMove::XY, d_hGripper);
            }
        }
    }

    // Initialization
    d_edit_action.SetFont(d_bold);
    d_button_auto.SetCheck(d_owner.GetAuto() ? BST_CHECKED : BST_UNCHECKED);
    if (d_prompt) {
        //
        //  Install prompt ....

        // <appname <channel> version <version> is available
        AString action;
        action += d_owner.AppName();
        if (! d_owner.Manifest().BuildLabel.empty()) {
            action += _T(" ");
            action += d_owner.Manifest().BuildLabel;
        }
        action += _T(" version ");
        action += d_owner.Manifest().attributeVersion;
        action += _T(" is available");
        d_edit_action.SetText(action);

        // summary information
        AString summary;
        summary += d_owner.Manifest().title;
        summary += _T(" is now available (you have ");
        summary += d_owner.AppVersion();
        summary += _T("). Would you like to install it now?");
        if (d_owner.Manifest().pubDate.length()) {
            summary += _T("\r\nPublished ");
            summary += d_owner.Manifest().pubDate;
        }
        d_edit_summary.SetText(summary);
        d_edit_summary.SetFont(d_norm);

        // Release Notes:
        //  <notes|description>
        ReleaseNotes();

        // .. actions
        d_button_ok.ShowWindow(SW_HIDE);
        d_button_skip.ShowWindow(d_owner.Manifest().IsCriticalUpdate(d_owner.AppVersion()) ? SW_HIDE : SW_SHOW);
        d_button_now.ShowWindow(SW_SHOW);
        d_button_later.ShowWindow(SW_SHOW);

        // .. screen resources to reposition
        AutoPull(IDC_INSTALL_SUMMARY, SMove::X);
        AutoPull(IDC_INSTALL_NOTES);
        AutoMove(IDC_INSTALL_RESULT, SMove::Xplus);
        AutoMove(IDC_INSTALL_AUTO, SMove::Y);
        AutoMove(IDC_INSTALL_SKIP, SMove::Y);
        AutoMove(IDC_INSTALL_LATER);
        AutoMove(IDC_INSTALL_NOW);
        AutoMove(IDOK);

    } else {
        //
        //  Up-to-date, verbose
        AString action;
        action += d_owner.AppName();
        action += _T(" version ");
        action += d_owner.Manifest().attributeVersion;
        action += _T(" is up-to-date");
        d_edit_action.SetText(action);
        d_edit_title.ShowWindow(SW_HIDE);

        // Release Notes:
        //  <notes|description>
        ReleaseNotes();

        // .. actions
        d_button_ok.ShowWindow(SW_SHOW);
        d_button_skip.ShowWindow(SW_HIDE);
        d_button_now.ShowWindow(SW_HIDE);
        d_button_later.ShowWindow(SW_HIDE);

        // .. screen resources to reposition
        AutoPull(IDC_INSTALL_NOTES);
        AutoMove(IDOK);
    }

    return TRUE; // return TRUE unless you set the focus to a control
}


static void __stdcall
BrowserResizer(const RECT &rect, void *param)
{
    RECT item_rect; // relative to 0,0
    ::SetRect(&item_rect, 0, 0, (rect.right - rect.left), (rect.bottom - rect.top));

    if (CSimpleBrowser *browser = static_cast<CSimpleBrowser *>(param)) {
        browser->SetRect(item_rect);
    }
}


//private
void
CUpdateInstallDlg::ReleaseNotes()
{
    const Updater::AutoManifest& manifest = d_owner.Manifest();

    d_edit_title.SetWindowText(_T("Release Notes:"));
    d_edit_title.SetFont(d_bold);

#if (0)
    if (useweb_) {
#endif

        d_browser = new CSimpleBrowser();
        d_browser->CreateFromControl(GetSafeHwnd(), IDC_INSTALL_NOTES);
        if (manifest.releaseNotesLink.empty()) {
            d_browser->Content(Updater::to_wstring(manifest.description));
        } else if (! manifest.releaseNotesContent.empty()) {
            d_browser->Content(Updater::to_wstring(manifest.releaseNotesContent));
        } else {
            d_browser->Navigate(Updater::to_wstring(manifest.releaseNotesLink));
        }
        AutoPull(IDC_INSTALL_NOTES, &BrowserResizer, (void *)d_browser);

#if (0)
    } else {
#if defined(UNICODE)
        std::wstring text;
        text = Updater::to_wstring(manifest.description);
        for (size_t pos = 0; (pos = text.find(L"\n", pos)) != std::string::npos; pos += 2) {
            text.replace(pos, 1, L"\r\n");
        }
#else
        std::string text;
        text = manifest.description;
        for (size_t pos = 0; (pos = text.find("\n", pos)) != std::string::npos; pos += 2) {
            text.replace(pos, 1, "\r\n");
        }
#endif
        d_notes.SetWindowText(text.c_str());
    }
#endif
}


LRESULT
CUpdateInstallDlg::OnCtlColorStatic(WPARAM wParam, LPARAM lParam)
{
    if (::GetDlgItem(d_hWnd, IDC_INSTALL_ACTION) == (HWND)lParam) {
        ::SetTextColor((HDC)wParam, RGB(0x56, 0x96, 0xBC));     // Steel Blue.
        return (LRESULT) ::GetSysColorBrush(COLOR_3DFACE);      // Dialog backgrond.

    } else if (::GetDlgItem(d_hWnd, IDC_INSTALL_RESULT) == (HWND)lParam) {
        ::SetTextColor((HDC)wParam, RGB(0xE0, 0x48, 0x36));     // Cinnabar.
        return (LRESULT) ::GetSysColorBrush(COLOR_3DFACE);      // Dialog backgrond.
    }
    return 0;
}


void
CUpdateInstallDlg::AutoPush(SMove &s, int iID)
{
    if (0 == s.c_hWnd && iID > 0) GetDlgItem(iID, &s.c_hWnd);
    assert(s.c_hWnd); assert(s.c_action);
    ::GetWindowRect(s.c_hWnd, &s.c_rcInitial);
    ScreenToClient(s.c_rcInitial);
    d_autoMove.push_back(s);
}


void
CUpdateInstallDlg::AutoMove(int iID, SMove::What what, HWND hWnd)
{
    SMove s = {0};
    s.c_hWnd = hWnd;
    s.c_action = SMove::Move;
    s.c_what = what;
    AutoPush(s, iID);
}


void
CUpdateInstallDlg::AutoPull(int iID, SMove::What what, HWND hWnd)
{
    SMove s = {0};
    s.c_hWnd = hWnd;
    s.c_action = SMove::Pull;
    s.c_what = what;
    AutoPush(s, iID);
}


void
CUpdateInstallDlg::AutoPull(int iID, CUpdateInstallDlg::Resizer resizer, void *param, SMove::What what, HWND hWnd)
{
    SMove s = {0};
    s.c_hWnd = hWnd;
    s.c_action = SMove::Pull;
    s.c_what = what;
    s.c_resizer = resizer;
    s.c_param = param;
    AutoPush(s, iID);
}


void
CUpdateInstallDlg::AutoScale(int iID, double dXMovePct, double dYMovePct,
            double dXSizePct, double dYSizePct, HWND hWnd)
{
    assert((dXMovePct + dXSizePct) <= 100.0);
    assert((dYMovePct + dYSizePct) <= 100.0);
    SMove s = {0};
    s.c_hWnd = hWnd;
    s.c_action = SMove::Scale;
    s.c_dXMoveFrac = dXMovePct / 100.0; s.c_dYMoveFrac = dYMovePct / 100.0;
    s.c_dXSizeFrac = dXSizePct / 100.0; s.c_dYSizeFrac = dYSizePct / 100.0;
    AutoPush(s, iID);
}


BOOL
CUpdateInstallDlg::OnGetMinMaxInfo(MINMAXINFO *lpMMI)
{
    AutoDialog::OnGetMinMaxInfo(lpMMI);

    if (lpMMI->ptMinTrackSize.x < d_szMinimum.cx) lpMMI->ptMinTrackSize.x = d_szMinimum.cx;
    if (lpMMI->ptMinTrackSize.y < d_szMinimum.cy) lpMMI->ptMinTrackSize.y = d_szMinimum.cy;
    return TRUE;
}


void
CUpdateInstallDlg::OnSize(UINT nType, int cx, int cy)
{
    AutoDialog::OnSize(nType, cx, cy);

    if (d_autoMove.size()) {
        const int iXDelta = cx - d_szInitial.cx;
        const int iYDelta = cy - d_szInitial.cy;
        HDWP hDefer = BeginDeferWindowPos((int)d_autoMove.size());

        for (SMoving_t::const_iterator p = d_autoMove.begin();  p != d_autoMove.end();  ++p) {
            if (p->c_hWnd) {
                ARect nrect(p->c_rcInitial);

                switch (p->c_action) {
                case SMove::Move:       // move
                    switch (p->c_what) {
                    case SMove::XY:     // retaining current size.
                        nrect.OffsetRect(iXDelta, iYDelta);
                        break;
                    case SMove::Y:      // retaining current size.
                        nrect.top += iYDelta;
                        nrect.bottom += iYDelta;
                        break;
                    case SMove::Xplus:  // pin column, repos Y, resize X.
                        nrect.top    += iYDelta;
                        nrect.bottom += iYDelta;
                        nrect.right  += iXDelta;
                        break;
                    case SMove::X:      // retaining current size.
                        nrect.left += iXDelta;
                        nrect.right += iXDelta;
                        break;
                    }
                    break;
                case SMove::Pull:       // pull bottom/right corner
                    switch (p->c_what) {
                    case SMove::Y:      // bottom
                        nrect.bottom += iYDelta;
                        break;
                    case SMove::X:      // right
                        nrect.right  += iXDelta;
                        break;
                    default:            // bottom/right corner
                        nrect.right  += iXDelta;
                        nrect.bottom += iYDelta;
                        break;
                    }
                    break;
                case SMove::Scale:      // reposition/scale
                    nrect.OffsetRect(int(iXDelta * p->c_dXMoveFrac), int(iYDelta * p->c_dYMoveFrac));
                    nrect.right  += int(iXDelta * p->c_dXSizeFrac);
                    nrect.bottom += int(iYDelta * p->c_dYSizeFrac);
                    break;
                }

                if (p->c_resizer) {
                    (p->c_resizer)(nrect, p->c_param);

                } else {
                    UINT uFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER;
                    if (SMove::Scale == p->c_action &&
                            ((p->c_dXSizeFrac != 0.0) || (p->c_dYSizeFrac != 0.0))) {
                        uFlags |= SWP_NOCOPYBITS;
                    }
                    DeferWindowPos(hDefer, p->c_hWnd, NULL, nrect.left, nrect.top, nrect.Width(), nrect.Height(), uFlags);
                }
            }
        }

        EndDeferWindowPos(hDefer);
    }

    if (d_hGripper) {
        ::ShowWindow(d_hGripper, (nType == SIZE_MAXIMIZED) ? SW_HIDE : SW_SHOW);
    }
}


BOOL
CUpdateInstallDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam)) {
    case IDC_INSTALL_AUTO:
        OnBnClickedInstallAuto();
        return FALSE;
    case IDC_INSTALL_LATER:
        OnBnClickedInstallLater();
        return TRUE;
    case IDC_INSTALL_SKIP:
        OnBnClickedInstallSkip();
        return TRUE;
    case IDC_INSTALL_NOW:
        OnBnClickedInstallNow();
        return TRUE;
    case IDCANCEL:
        OnCANCEL();
        return TRUE;
    case IDOK:
        OnOK();
        return TRUE;
    }
    return AutoDialog::OnCommand(wParam, lParam);
}


void
CUpdateInstallDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        UpdaterInstall::AboutDlg about(d_owner, GetSafeHwnd());
        about.DoModal();
    } else {
        AutoDialog::OnSysCommand(nID, lParam);
    }
}


void
CUpdateInstallDlg::OnPaint()
{
#if (TODO)
    if (IsIconic()) {
        // If you add a minimize button to your dialog, you will need the code below
        //  to draw the icon.  For MFC applications using the document/view model,
        //  this is automatically done for you by the framework.
        CPaintDC dc(this);                      // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        // Center icon in client rectangle
        const int cxIcon = GetSystemMetrics(SM_CXICON);
        const int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        const int x = (rect.Width() - cxIcon + 1) / 2;
        const int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, d_hIcon);             // draw the icon
    } else {
        AutoDialog::OnPaint();
    }
#endif
}


HCURSOR
CUpdateInstallDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(d_hIcon);
}


// <Install Now
void
CUpdateInstallDlg::OnBnClickedInstallNow()
{
    if (! d_button_now.IsWindowEnabled()) {
        return;
    }

    CDialogUpdater updater(*this);
    d_edit_result.SetText(AString());

    d_button_skip.EnableWindow(false);
    d_button_now.EnableWindow(false);
    d_button_later.EnableWindow(false);

    if (d_owner.InstallNow(updater, true)) {
        EndDialog(IDC_INSTALL_NOW);

    } else {
        d_button_later.EnableWindow(true);
        d_button_now.EnableWindow(true);
        if (! d_owner.Manifest().IsCriticalUpdate(d_owner.AppVersion())) {
            d_button_skip.EnableWindow(true);
        }
    }
}


// <Remind Me Later>
void
CUpdateInstallDlg::OnBnClickedInstallLater()
{
    d_owner.InstallLater();
    EndDialog(IDC_INSTALL_LATER);
}


// <Skip Version>
void
CUpdateInstallDlg::OnBnClickedInstallSkip()
{
    d_owner.InstallSkip();
    EndDialog(IDC_INSTALL_SKIP);
}


// [x] Automatically check for updates
void
CUpdateInstallDlg::OnBnClickedInstallAuto()
{
    d_owner.SetAuto(BST_CHECKED == d_button_auto.GetCheck() ? true : false);
}

//end
