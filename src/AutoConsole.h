#ifndef AUTOCONSOLE_H_INCLUDED
#define AUTOCONSOLE_H_INCLUDED
//  $Id: AutoConsole.h,v 1.5 2021/08/14 15:38:09 cvsuser Exp $
//
//  AutoUpdater: console interface.
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

#include "IAutoUpdaterUI.h"

class AutoConsoleUI : public IAutoUpdaterUI {
public:
    AutoConsoleUI();
    virtual ~AutoConsoleUI();

    virtual PromptResponse PromptDialog(AutoUpdater &owner);
    virtual int     InstallDialog(AutoUpdater &owner);
    virtual void    UptoDateDialog(AutoUpdater &owner);

    virtual void    WarningMessage(const char *message);
    virtual void    ErrorMessage(const char *message);

    virtual void    ProgressStart(AutoUpdater &owner, HWND parent, bool indeterminate = false, const char *msg = 0);
    virtual void    ProgressUpdate(int percentage, int total = 0);
    virtual bool    ProgressCancelled();
    virtual bool    ProgressStop();

private:
    void *          progress_;                  // progress implementation.
};

#endif //AUTOCONSOLE_H_INCLUDED

//end
