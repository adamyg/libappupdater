//  $Id: TProgressBar.cpp,v 1.17 2025/04/16 11:33:48 cvsuser Exp $
//
//  AutoUpdater: TProgressDialog.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
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

#include <string>
#include <cassert>
#include <vector>
#include <iostream>
#include <limits>
#include <algorithm>
#include <cmath>

#include "TProgressBar.h"
#include "VTSupport.h"

#undef min
#undef max

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


//static
int
TProgressBar::Progress(char *buffer, int buflen, unsigned complete, unsigned total) 
{
    const char *suffix[] = {"B", "KB", "MB", "GB", "TB"};

    if (total) {
        const unsigned percentage = (unsigned)(((double)complete / total) * 100.0);
        double unit = (double)(total);
        int s = 0;

        if (total > 1024) {
            while ((total / 1024) > 0 && s < static_cast<int>(_countof(suffix) - 1)) {
                unit = total / 1024.0;
                total /= 1024;
                ++s;
            }
        }
        return sprintf_s(buffer, buflen, " %2d%% / %.02lf%s ", percentage, unit, suffix[s]);
    }
    buffer[0] = 0;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// 

namespace {
    struct Coloriser {
        Coloriser(VTSupport &vt, ProgressStyle style, size_t width) :
            style_(style), vt_(vt)
        {
            if (ProgressStyleRainbow == style_) {
                colors_.reserve(width);
                for (size_t blk = 0; blk < width; ++blk) {
                    colors_.push_back(VTSupport::rainbox_color(width, blk));
                }
            }
        }

        template <typename Stream>
        void set(Stream &out, size_t idx)
        {
            switch (style_) {
            case ProgressStyleNative:
                break;
            case ProgressStyleAccent:
                if (0 == idx) {
                    vt_.foreground(out, vt_.hilite());
                    vt_.background_scaled(out, vt_.hilite());
                }
                break;
            case ProgressStyleRainbow:
                idx = std::min(idx, colors_.size() - 1);
                vt_.foreground(out, colors_[idx]);
                break;
            }
        }

        const ProgressStyle style_;
        std::vector<VTColor> colors_;
        VTSupport &vt_;
    };
};


///////////////////////////////////////////////////////////////////////////////
//  TProgressBar

TProgressBar::TProgressBar()
    : thread_(0), style_(ProgressStyleAccent), references_(1),
        complete_(0), total_(0), speed_(150),
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


void
TProgressBar::SetAnimationSpeed(DWORD speed)
{
    if (0 == speed) speed = 100; // default
    speed_ = speed;
}


void
TProgressBar::SetStyle(ProgressStyle style)
{
    style_ = style;
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
    if (false == user_cancelled_) user_cancelled_ = VTSupport::ESCPressed();
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


void
TProgressBar::Update()
{
    if (vt_.isvt()) {
        TProgressBar::UpdateVT();
        vt_.restore();

    } else {
        TProgressBar::UpdateDefault();
    }
}


void
TProgressBar::UpdateDefault()
{
    static const char animation[] = "|/-\\";

    std::ostream &out = std::cout;
    int console_width = ConsoleWidth();
    char progress_buffer[32];
    unsigned index = 0;

    for (;;) {
        stop_.Wait(speed_);

        // termination
        Updater::CriticalSection::Guard guard(lock_);
        if (! running_) {
            if (index) {
                for (int i = 0; i < console_width; ++i) {
                    out << ' ';
                }
                out << '\r';
                out.flush();
            }
            break;
        }

        // cancel message
        if (cancelable_ && HasUserCancelled()) {
            if (index != static_cast<unsigned>(-1)) {
                for (int i = 0; i < console_width; ++i) {
                    out << ' ';
                }
                out << '\r';
                index = static_cast<unsigned>(-1);
            }

            if (! text_msg_.empty())
                out << text_msg_ << " .. ";
            if (cancel_msg_.empty()) {
                out << "canceling";
            } else {
                out << cancel_msg_;
            }

            out << ' '<< animation[ index % (_countof(animation)-1) ];
            out << '\r';
            out.flush();
            continue;
        }

        // bar
        console_width = ConsoleWidth();

        const int progress_len =
                Progress(progress_buffer, sizeof(progress_buffer), complete_, total_);
        int display_width = console_width - (cancelable_ ? 10 : 4);
        int length = (cancelable_ ? 5 : 0);

        if (! text_msg_.empty()) {
            int text_width = (int)(text_msg_.length() + 1);

            length += text_width;
            if (text_width > (display_width - 10)) {
                text_width = display_width - 10;
                if (text_width > 10) {
                    const char *cursor = text_msg_.c_str();
                    for (int i = 2; i < text_width; ++i) {
                        out << *cursor++;
                    }
                    out << "..";
                } else {
                    text_width = 0;
                }
            } else {
                out << text_msg_ << ' ';
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
                const int start = (int)(index % display_width),
                    end = (int)((start + (display_width / 2)) % display_width);

                out << '[';
                if (end > start) {
                    for (int i = length; i < display_width; ++i) {
                        out << (i >= start && i <= end ? "#" : "-");
                    }
                } else {
                    for (int i = length; i < display_width; ++i) {
                        out << (i >= start || i <= end ? "#" : "-");
                    }
                }
                out << "] ";
            }

        } else {
            const double progress = static_cast<double>(complete_) / total_;
            const int block = static_cast<int>(std::floor(progress * display_width));

            if ((length + 10) < display_width) {
                out << '[';
                for (int i = length; i < display_width; ++i) {
                    out << (i <= block ? "#" : "-");
                }
                out << "] ";
            }
        }

        // trailing details
        out << animation[ index % (_countof(animation)-1) ];
        out << progress_buffer;
        if (cancelable_) {
            out << " - ESC";
        }
        out << '\r';
        out.flush();
        ++index;
    }
}


void
TProgressBar::UpdateVT()
{
    static const char* const fills[] = {
        "\xe2\x96\x91", // U+2591, Light Shade
        "\xe2\x96\x92", // U+2592, Medium Shade
        "\xe2\x96\x93", // U+2593, Dark Shade
        "\xe2\x96\x88", // U+2588, Full Block
        "\xe2\x96\x93", // U+2593, Dark Shade
        "\xe2\x96\x92", // U+2592, Medium Shade
        };

    static const char* const blocks[] = {
#define BLOCK_OFF   0
        " ",            // Blank
        "\xe2\x96\x8f", // U+258E, Left One Eighth Block
        "\xe2\x96\x8e", // U+258E, Left One Quarter Block
        "\xe2\x96\x8d", // U+258D, Left Three Eighths Block
        "\xe2\x96\x8c", // U+258C, Left Half Block
        "\xe2\x96\x8B", // U+258B, Left Five Eighths Block
        "\xe2\x96\x8a", // U+258A. Left Three Quarters Block
        "\xe2\x96\x89", // U+2589, Left Seven Eighths Block
        "\xe2\x96\x88"  // U+2588, Full Block
#define BLOCK_ON    8
        };

    static const char *animation[] = {
        "\xe2\x96\x81", // ▁
        "\xe2\x96\x82", // ▂
        "\xe2\x96\x83", // ▃
        "\xe2\x96\x84", // ▄
        "\xe2\x96\x85", // ▅
        "\xe2\x96\x86", // ▆
        "\xe2\x96\x87", // ▇
        "\xe2\x96\x88", // █
        "\xe2\x96\x87", // ▇
        "\xe2\x96\x86", // ▆
        "\xe2\x96\x85", // ▅
        "\xe2\x96\x84", // ▄
        "\xe2\x96\x83", // ▃
        "\xe2\x96\x81"  // ▁
        };

    std::ostream &out = std::cout;
    const int console_width = ConsoleWidth();
    const size_t width = std::max(console_width / 2, 40);
    Coloriser coloriser(vt_, style_, width);
    size_t index = 0;

    vt_.normal(out);
    if (! text_msg_.empty()) {
        out << text_msg_ << "\r\n";
    }

    for (;;) {
        stop_.Wait(speed_);

        // termination
        Updater::CriticalSection::Guard guard(lock_);
        if (! running_) {
            if (index) {
                vt_.normal(out);
                vt_.erase_eol(out);
                if (! text_msg_.empty()) {
                    vt_.cursor_prev(out);
                    vt_.erase_eol(out);
                }
                vt_.flush(out);
            }
            break;
        }

        // progress bar
        char progress_buffer[32];
        /*const int progress_len =*/
            Progress(progress_buffer, sizeof(progress_buffer), complete_, total_);

        if (marquee_ || 0 == total_) {
             const size_t block = (total_ ? 
                static_cast<size_t>(std::floor(((double)complete_ / total_) * width)) : (index / _countof(fills)));
               
            for (size_t blk = 0; blk < width; ++blk) {
                coloriser.set(out, blk);
                if (blk == block-2) {
                    out << fills[1];
                } else if (blk == block-1) {
                    out << fills[2];
                } else if (blk == block) {
                    out << fills[3];
                } else  if (blk == block+1) {
                    out << fills[4];
                } else  if (blk == block+2) {
                    out << fills[5];
                } else {
                    out << fills[0];
                }
            }

        } else {
            const double progress = static_cast<double>(complete_) / total_;
            const size_t block = static_cast<size_t>(std::floor(progress * width));
            const size_t part = static_cast<size_t>((progress * width - block) * 8);

            for (size_t blk = 0; blk < width; ++blk) {
                size_t state = BLOCK_OFF;
                if (blk < block) {
                    state = BLOCK_ON;
                } else if (blk == block) {
                    state = part;
                }
                coloriser.set(out, blk);
                out << blocks[state];
            }
        }

        // trailing details
        vt_.normal(out);
        if (cancelable_ && HasUserCancelled()) {
            if (cancel_msg_.empty()) {
                out << " Canceling";
            } else {
                out << ' ' << cancel_msg_;
            }
            vt_.erase_eol(out);
        } else {
            out << animation[ index % _countof(animation) ];
            out << progress_buffer;
            if (cancelable_) {
                out << " - ESC";
            }
        }
        out << '\r';
        vt_.flush(out);
        ++index;
    }
}

//end
