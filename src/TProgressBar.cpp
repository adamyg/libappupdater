//  $Id: TProgressBar.cpp,v 1.8 2021/08/18 13:01:03 cvsuser Exp $
//
//  AutoUpdater: TProgressDialog.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
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

#include "common.h"

#include <string>
#include <cassert>
#include <iostream>
#include <limits>

#include "TProgressBar.h"


///////////////////////////////////////////////////////////////////////////////
//  Support functions.

static DWORD WINAPI
progress_thread(LPVOID lpParameter)
{
    TProgressBar *self = static_cast<TProgressBar *>(lpParameter);
    self->Update();
    self->Release();
    return 0;
}


//static
int
TProgressBar::ConsoleWidth()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi = {0};
    ::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return (int)(csbi.srWindow.Right - csbi.srWindow.Left);
}



///////////////////////////////////////////////////////////////////////////////
//  TProgressBar

TProgressBar::TProgressBar()
    : references_(1), thread_(0), complete_(0), total_(0),
        marquee_(false), cancelable_(false),
        user_cancelled_(false), running_(false)
{
    stop_.Create();
}


TProgressBar::~TProgressBar()
{
    assert(0 == references_);
}


void
TProgressBar::Release()
{
    unsigned references;
    {   Updater::CriticalSection::Guard guard(lock_);
        assert(references_);
        references = --references_;
    }
    if (0 == references) {
        delete this;
    }
}


void 
TProgressBar::SetTextMsg(const char *text)
{
    Updater::CriticalSection::Guard guard(lock_);
    text_msg_.assign(text ? text : "");
}


void 
TProgressBar::SetCancelMsg(const char *text)
{
    Updater::CriticalSection::Guard guard(lock_);
    cancel_msg_.assign(text ? text : "");
}


bool
TProgressBar::Start(bool marquee, bool cancelable)
{
    assert(references_);

    if (! stop_.IsOpen()) return false;

    Updater::CriticalSection::Guard guard(lock_);
    if (thread_) return true;

    marquee_ = marquee;
    cancelable_ = cancelable;
    running_ = true;
    ++references_;
    if (0 != (thread_= ::CreateThread(NULL, 0, progress_thread, this, 0, NULL))) {
        return true;
    }
    --references_;
    running_ = false;
    return false;
}


bool
TProgressBar::HasUserCancelled()
{
    assert(references_);
    return user_cancelled_;
}


void
TProgressBar::SetProgress(unsigned complete, unsigned total)
{
    assert(references_);

    Updater::CriticalSection::Guard guard(lock_);
    if ((complete_ = complete) > total) {
        complete_ = total;
    }
    total_ = total;
}


void
TProgressBar::Stop()
{
    assert(references_);

    HANDLE thread;
    {   Updater::CriticalSection::Guard guard(lock_);
        if (0 != (thread = thread_)) {
            stop_.Trigger();
            running_ = false;
        }
        thread_ = 0;
    }
    if (thread) {
        ::WaitForMultipleObjects(1, &thread, FALSE, INFINITE);
        ::CloseHandle(thread);
    }
}


static int
progress(char *buffer, int buflen, int complete, int total) 
{
    const char *suffix[] = {"B", "KB", "MB", "GB", "TB"};

    if (total > 0 && complete >= 0) {
        const int percentage = 
                (int)(((double)complete / total) * 100.0);
        double unit = (double)(total);
        int s = 0;

        if (total > 1024) {
            while ((total / 1024) > 0 && s < (_countof(suffix) - 1)) {
                unit = total / 1024.0;
                total /= 1024;
                ++s;
            }
        }
        return sprintf_s(buffer, buflen, " %d%% / %.02lf%s ", percentage, unit, suffix[s]);
    }

    buffer[0] = 0;
    return 0;
}


void
TProgressBar::Update()
{
    static const char animation[] = "|/-\\";
    int console_width = ConsoleWidth();
    char progress_buffer[32];
    unsigned index = 0;
    unsigned pos = 0;

    for (;;) {
        stop_.Wait(125);

        // termination?
        Updater::CriticalSection::Guard guard(lock_);
        if (! running_) {
            for (int i = 0; i < console_width; ++i) {
                std::cout << " ";
            }
            std::cout << "\r";
            std::cout.flush();
            break;
        }

        // optional status text
        int length = (cancelable_ ? 5 : 0);
        if (user_cancelled_) {
            if (cancel_msg_.empty()) {
                std::cout << cancel_msg_ << " ";
                length += (int)(cancel_msg_.length() + 1);
            }
        } else {
            if (text_msg_.empty()) {
                std::cout << text_msg_ << " ";
                length += (int)(text_msg_.length() + 1);
            }
        }

        // bar
        console_width = ConsoleWidth();

        const int progress_len =
                progress(progress_buffer, sizeof(progress_buffer), complete_, total_);
        int display_width = console_width - (cancelable_ ? 10 : 4);

        if (! text_msg_.empty()) { //XXX
            int text_width = (int)(text_msg_.length() + 1);

            if (text_width > (display_width - 10)) {
                text_width = display_width - 10;
                if (text_width > 10) {
                    const char *cursor = text_msg_.c_str();
                    for (int i = 2; i < text_width; ++i) {
                        std::cout << *cursor++;
                    }
                    std::cout << "..";
                } else {
                    text_width = 0;
                }
            } else {
                std::cout << text_msg_ << ' ';
            }
            display_width -= text_width;
        }

        if (display_width > progress_len) {
            display_width -= progress_len;
        } else {
            progress_buffer[0] = 0;
        }

        if (marquee_ || 0 == total_) {
            if ((length + 10) < display_width) {
                const int start = (int)(pos % display_width),
                    end = (int)((start + (display_width / 3)) % display_width);

                std::cout << "[";
                if (end > start) {
                    for (int i = length; i < display_width; ++i) {
                        std::cout << (i >= start && i <= end ? "#" : "-");
                    }
                } else {
                    for (int i = length; i < display_width; ++i) {
                        std::cout << (i >= start || i <= end ? "#" : "-");
                    }
                }
                std::cout << "] ";
            }
            ++pos;

        } else {
            const float progress = (float)complete_ / total_;
            int bar_progress = (int)(progress * display_width);

            if ((length + 10) < display_width) {
                std::cout << "[";
                for (int i = length; i < display_width; ++i) {
                    std::cout << (i <= bar_progress ? "#" : "-");
                }
                std::cout << "] ";
            }
        }

        std::cout << progress_buffer;

        std::cout << animation[ ++index % (sizeof(animation)-1) ];

        if (cancelable_) {
            std::cout << " - ESC";
        }

        std::cout << "\r";
        std::cout.flush();
    }
}
