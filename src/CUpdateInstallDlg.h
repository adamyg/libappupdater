#pragma once
//  $Id: CUpdateInstallDlg.h,v 1.10 2021/08/14 05:23:48 cvsuser Exp $
//
//  Install dialog
//
//  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
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

#include <vector>

#include "AutoUpdater.h"
#include "AutoDialog.h"

#include "resource.h"

class CSimpleBrowser;

/////////////////////////////////////////////////////////////////////////////////////////
//  CUpdateInstallDlg
//

class CUpdateInstallDlg : public AutoDialog {
public:
    CUpdateInstallDlg(AutoUpdater &owner, bool prompt /*CWnd* pParent = NULL*/);
    virtual ~CUpdateInstallDlg();

    // Dialog Data
    enum { IDD = IDD_INSTALL_DIALOG };

    // Implementation
private:
    typedef void (__stdcall *Resizer)(const RECT &rect, void *param);

    struct SMove {
        HWND        c_hWnd;
        enum {Move = 1, Pull, Scale} c_action;
        enum What {X = 1, Y = 2, Xplus = 3, XY = 4 } c_what;
        double      c_dXMoveFrac;
        double      c_dYMoveFrac;
        double      c_dXSizeFrac;
        double      c_dYSizeFrac;
        ARect       c_rcInitial;
        Resizer     c_resizer;
        void       *c_param;
    };

    typedef std::vector<SMove> SMoving_t;

    void            ReleaseNotes();

    void            AutoPush(SMove &s, int iID = 0);
    void            AutoMove(int iID, SMove::What what = SMove::XY, HWND hWnd = 0);
    void            AutoPull(int iID, SMove::What what = SMove::XY, HWND hWnd = 0);
    void            AutoPull(int iID, Resizer resize, void *param, SMove::What what = SMove::XY, HWND hWnd = 0);
    void            AutoScale(int iID, double dXMovePct, double dYMovePct,
                            double dXSizePct, double dYSizePct, HWND hWnd = 0);

private:
    friend class CDialogUpdater;

    AutoUpdater    &d_owner;
    bool            d_prompt;
    HICON           d_hIcon;                    // application icon.
    AFont          *d_norm;                     // normal font.
    AFont          *d_bold;                     // bold font.
    CSimpleBrowser *d_browser;                  // browser instance.
    ASize           d_szMinimum;
    ASize           d_szInitial;
    HWND            d_hGripper;
    SMoving_t       d_autoMove;

protected:
    virtual BOOL    OnInitDialog();
    virtual BOOL    OnGetMinMaxInfo(MINMAXINFO *mmi);
    virtual void    OnSize(UINT nType, int cx, int cy);
    virtual BOOL    OnCommand(WPARAM wParam, LPARAM lParam);
    virtual void    OnSysCommand(UINT nID, LPARAM lParam);
    virtual LRESULT OnCtlColorStatic(WPARAM wParam, LPARAM lParam);
    virtual void    OnPaint();
    virtual HCURSOR OnQueryDragIcon();
    virtual void    OnBnClickedInstallNow();
    virtual void    OnBnClickedInstallLater();
    virtual void    OnBnClickedInstallSkip();
    virtual void    OnBnClickedInstallAuto();

protected:
  //CReadOnlyEdit   d_notes;
    AEdit           d_notes;
    AEdit           d_edit_action;
    AEdit           d_edit_result;
    AEdit           d_edit_summary;
    AEdit           d_edit_title;
    AButton         d_button_skip;
    AButton         d_button_auto;
    AButton         d_button_now;
    AButton         d_button_later;
    AButton         d_button_ok;
};
