#pragma once
//  $Id: CUptodateDlg.h,v 1.8 2021/08/14 05:23:48 cvsuser Exp $
//
//  CUpdateUptodateDlg
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

#include <string>

#include "AutoUpdater.h"
#include "AutoDialog.h"

#include "CUptodateDlg.h"
#include "resource.h"


///////////////////////////////////////////////////////////////////////////////
//  CUpdateUptodateDlg dialog
//

class CUpdateUptoDateDlg : public AutoDialog {
public:
    CUpdateUptoDateDlg(AutoUpdater &owner);
    virtual ~CUpdateUptoDateDlg();

    // Dialog Data
    enum { IDD = IDD_UPTODATE_DIALOG };

protected:
    virtual BOOL    OnInitDialog();
    virtual LRESULT OnCtlColorStatic(WPARAM wParam, LPARAM lParam);

private:
    AFont           d_norm;
    AFont           d_bold;
};
