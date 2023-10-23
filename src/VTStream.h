#pragma once
//  $Id: VTStream.h,v 1.3 2023/10/23 12:45:14 cvsuser Exp $
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2022 - 2023, Adam Young
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

#include "VTSupport.h"

/////////////////////////////////////////////////////////////////////////////////////////
//  VTStream

template <typename Stream = std::ostream>
class VTStream {
public:
    VTStream(Stream &out) : out_(out)
    {
        VTSupport::VTEnable(state_);
    }

    ~VTStream()
    {
        VTSupport::VTRestore(state_);
    }

    Stream& normal()
    {
        return VTSupport::normal(out_);
    }

    Stream& bold(bool on = true)
    {
        return VTSupport::bold(on);
    }

    Stream& underline(bool on = true)
    {
        return VTSupport::underline(on);
    }

    Stream& inverse(bool on = true)
    {
        return VTSupport::inverse(on);
    }

    Stream& flush()
    {
        return VTSupport::flush(out_);
    }

    Stream& cursor_pos(unsigned y, unsigned x)
    {
        return VTSupport::cursor_pos(out_, y, x);
    }

    Stream& cursor_shape(VTSupport::CursorShape shape)
    {
        return VTSupport::cursor_shape(out_, shape);
    }

    Stream& cursor_up(size_t n = 1)
    {
        return VTSupport::cursor_up(out_, n);
    }

    Stream& cursor_down(size_t n = 1)
    {
        return VTSupport::cursor_down(out_, n);
    }

    Stream& cursor_forward(size_t n = 1)
    {
        return VTSupport::cursor_forward(out_, n);
    }

    Stream& cursor_back(size_t n = 1)
    {
        return VTSupport::cursor_back(out_, n);
    }

    Stream& cursor_next(size_t n = 1)
    {
        return VTSupport::cursor_next(out_, n);
    }

    Stream& cursor_prev(size_t n = 1)
    {
        return VTSupport::cursor_prev(out_, n);
    }

    Stream& cursor_save()
    {
        return VTSupport::cursor_save(out_);
    }

    Stream& cursor_restore()
    {
        return VTSupport::cursor_restore(out_);
    }

    Stream& erase_sol()
    {
        return VTSupport::erase_sol(out_);
    }

    Stream& erase_eol()
    {
        return VTSupport::erase_eol(out_);
    }

    Stream& erase_up()
    {
        return VTSupport::erase_up(out_);
    }

    Stream& erase_down()
    {
        return VTSupport::erase_down(out_);
    }

    Stream& erase()
    {
        return VTSupport::erase(out_);
    }

    Stream& clear()
    {
        return VTSupport::clear(out_);
    }

    Stream& foreground(VTSupport::Color color)
    {
        return VTSupport::foreground(out_, color);
    }

    Stream& foreground(unsigned idx)
    {
        return VTSupport::foreground(out_, idx);
    }

    Stream& foreground(const VTColor color)
    {
        return VTSupport::foreground(out_, color);
    }

    Stream& foreground_scaled(const VTColor color, unsigned value = 3)
    {
        return VTSupport::foreground_scaled(out_, color, value);
    }

    Stream& background(VTSupport::Color color)
    {
        return VTSupport::background(out_, color);
    }

    Stream& background(unsigned idx)
    {
        return VTSupport::background(out_, idx);
    }

    Stream& background(const VTColor color)
    {
        return VTSupport::background(out_, color);
    }

    Stream& background_scaled(const VTColor color, unsigned value = 3)
    {
        return VTSupport::background_scaled(out_, color, value)
    }

private:
    Stream &out_;
    VTSupport::ConsoleState state_;
};

//end