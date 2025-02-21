#pragma once
//  $Id: CUpdatePromptDlg.h,v 1.11 2025/02/21 19:03:23 cvsuser Exp $
//
//  AutoUpdater: Prompt dialog.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
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

#include "AutoDialog.h"

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////////////////
//  CUpdatePromptDlg dialog
//

class AutoUpdater;
class CUpdatePromptDlg : public AutoDialog {
public:
    CUpdatePromptDlg(AutoUpdater &owner);
    virtual ~CUpdatePromptDlg();

    // Dialog Data
    enum { IDD = IDD_PROMPT_DIALOG };

protected:
    virtual BOOL    OnInitDialog();
    virtual BOOL    OnCommand(WPARAM wParam, LPARAM lParam);
    virtual void    OnSysCommand(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnCtlColorStatic(WPARAM wParam, LPARAM lParam);
    void            OnBnClickedCheckOnce();
    void            OnBnClickedCheckNo();
    void            OnBnClickedCheckAuto();

protected:
    AFont           d_bold;
    AFont           d_norm;
};
