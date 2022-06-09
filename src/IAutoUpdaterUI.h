#ifndef IAUTOUPDATERUI_H_INCLUDED
#define IAUTOUPDATERUI_H_INCLUDED
//  $Id: IAutoUpdaterUI.h,v 1.7 2022/06/09 08:46:31 cvsuser Exp $
//
//  IAutoUpdaterUI: ui interface.
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

#include <string>

class AutoUpdater;

enum PromptResponse {
    PROMPT_CANCEL,
    PROMPT_ONCE,
    PROMPT_AUTO,
    PROMPT_NO
};

class IAutoUpdaterUI {
    IAutoUpdaterUI(const IAutoUpdaterUI &);
    IAutoUpdaterUI& operator=(const IAutoUpdaterUI &);

public:
    IAutoUpdaterUI() {}
    virtual ~IAutoUpdaterUI() {}

    virtual enum PromptResponse PromptDialog(AutoUpdater &owner) = 0;
    virtual int         InstallDialog(AutoUpdater &owner) = 0;
    virtual void        UptoDateDialog(AutoUpdater &owner) = 0;

    virtual void        WarningMessage(const char *message) = 0;
    virtual void        ErrorMessage(const char *message) = 0;

    virtual void        ProgressStart(AutoUpdater &owner, HWND parent, bool indeterminate = false, const char *msg = 0) = 0;
    virtual void        ProgressUpdate(int percentage, int total = 0) = 0;
    virtual bool        ProgressCancelled() = 0;
    virtual bool        ProgressStop() = 0;
};

#endif
