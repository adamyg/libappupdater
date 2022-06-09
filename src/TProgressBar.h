#pragma once
//  $Id: TProgressBar.h,v 1.4 2021/08/17 15:27:10 cvsuser Exp $
//
//  TProgressBar
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

#include "AutoThread.h"

class TProgressBar {
    virtual ~TProgressBar();

public:
    TProgressBar();

    void SetTextMsg(const char *text);
    void SetCancelMsg(const char *text);

    bool Start(bool marquee = true, bool cancelable = true);
    void SetProgress(unsigned complete, unsigned total);
    bool HasUserCancelled();
    void Stop();

public:
    static int ConsoleWidth();

public:
    void Release();
    void Update();

private:
    Updater::CriticalSection lock_;
    Updater::WaitableEvent stop_;
    std::string text_msg_;
    std::string cancel_msg_;
    HANDLE thread_;
    unsigned references_;
    unsigned complete_;
    unsigned total_;
    bool marquee_;
    bool cancelable_;
    bool user_cancelled_;
    bool running_;
};
