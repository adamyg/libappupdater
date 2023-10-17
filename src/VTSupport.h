#pragma once
//  $Id: VTSupport.h,v 1.2 2023/10/17 12:47:53 cvsuser Exp $
//
//  AutoUpdater: TProgressDialog.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2023, Adam Young
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

#include <shlwapi.h>
#pragma comment( lib, "shlwapi.lib" )

#include "common.h"

///////////////////////////////////////////////////////////////////////////////
//  VT support

class VTSupport {
    VTSupport(const VTSupport &); // delete
    VTSupport& operator=(const VTSupport &); // delete

public:
    struct ConsoleState {
        bool vt;
        DWORD mode;
        UINT cp;
        CONSOLE_CURSOR_INFO cursor;
    };

#define VT100_ESCAPE            "\x1b"
#define VT100_CSI               VT100_ESCAPE "["
#define VT100_TEXTFORMAT(_id_)  VT100_CSI #_id_ "m"

    enum VTMode {
        VTMODE_UNINIT = -1,
        VTMODE_UNAVAIL,
        VTMODE_INACTIVE,
        VTMODE_ACTIVE
    };

    struct VTColor {
        VTColor() : r(0), g(0), b(0) {}
        VTColor(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
        VTColor(DWORD color) {
            r = GetRValue(color), g = GetGValue(color), b = GetBValue(color);
        }
        VTColor& operator=(const DWORD color) {
            r = GetRValue(color), g = GetGValue(color), b = GetBValue(color);
            return *this;
        }
        uint8_t r, g, b;
    };

public:
    VTSupport() : mode_(VTMODE_UNINIT)
    {
        hilite_ = ::GetSysColor(COLOR_HIGHLIGHT);
    }

    ~VTSupport()
    {
        restore();
    }

    bool
    isvt()
    {
        if (VTMODE_UNINIT == mode_) {
            mode_ = (VTEnable(state_) ? VTMODE_ACTIVE : VTMODE_UNAVAIL);

        } else if (VTMODE_INACTIVE == mode_) {
            ConsoleState t_state;
            mode_ = (VTEnable(t_state) ? VTMODE_ACTIVE : VTMODE_UNAVAIL);
        }
        return (mode_ != VTMODE_UNAVAIL);
    }

    void
    restore()
    {
        if (VTMODE_ACTIVE != mode_) return;
        VTRestore(state_);
        mode_ = VTMODE_INACTIVE;
    }

    const VTColor hilite() const
    {
        return hilite_;
    }

    static VTColor rainbox_color(size_t max, size_t idx)
    {
        const double percentage = static_cast<double>(idx) / max;
        WORD hue = static_cast<WORD>(240.0 * percentage);
        return VTColor(::ColorHLSToRGB(hue, 120, 240));
    }

    template <typename Stream>
    Stream& reset(Stream &out)
    {
        return out << VT100_TEXTFORMAT(0);
    }

    /// Moves the cursor up n (default 1)
    template <typename Stream>
    static Stream& cursor_up(Stream &out, size_t n = 1)
    {
        return out << VT100_CSI << n << 'A';
    }

    /// Moves the cursor down n (default 1)
    template <typename Stream>
    static Stream& cursor_down(Stream &out, size_t n = 1)
    {
        return out << VT100_CSI << n << 'B';
    }

    // Moves the cursor forward n (default 1)
    template <typename Stream>
    static Stream& cursor_forward(Stream &out, size_t n = 1)
    {
        return out << VT100_CSI << n << 'C';
    }

    /// Moves the cursor back n (default 1)
    template <typename Stream>
    static Stream& cursor_back(Stream &out, size_t n = 1)
    {
        return out << VT100_CSI << n << 'D';
    }

    /// Moves cursor to beginning of the line n (default 1) lines down.
    template <typename Stream>
    static Stream& cursor_next(Stream &out, size_t n = 1)
    {
        return out << VT100_CSI << n << 'E';
    }

    /// Moves cursor to beginning of the line n (default 1) lines up.
    template <typename Stream>
    static Stream& cursor_prev(Stream &out, size_t n = 1)
    {
        return out << VT100_CSI << n << 'F';
    }

    /// Clear until the end-of-line.
    template <typename Stream>
    static Stream& erase_eol(Stream &out)
    {
        return out << VT100_CSI "K";
    }

    /// Clear to the start-of-line.
    template <typename Stream>
    static Stream& erase_sol(Stream &out)
    {
        return out << VT100_CSI "1K";
    }

    /// Set foreground color RGB
    template <typename Stream>
    Stream& foreground(Stream &out, const VTColor &color)
    {
        char buffer[32];
        return out << VTForeground(color, buffer, sizeof(buffer));
    }

    /// Set background color RGB
    template <typename Stream>
    Stream& background(Stream &out, const VTColor &color)
    {
        char buffer[32];
        return out << VTBackground(color, buffer, sizeof(buffer));
    }

    /// Set background color to the scaled RGB (default %33)
    template <typename Stream>
    Stream& background_scaled(Stream &out, const VTColor &color, unsigned scale = 3)
    {
        if (0 == scale) return out;

        VTColor scaled = color;
        scaled.r /= scale;
        scaled.g /= scale;
        scaled.b /= scale;

        char buffer[32];
        return out << VTBackground(scaled, buffer, sizeof(buffer));
    }

public:
    const char *
    VTForeground(const VTColor color, char *buffer, size_t buflen)
    {
        // CSI 38 ; 2 ; <r> ; <g> ; <b> -- RGB foreground color
        sprintf_s(buffer, buflen, VT100_CSI "38;2;%u;%u;%um", color.r, color.g, color.b);
        return buffer;
    }

    const char *
    VTBackground(const VTColor color, char *buffer, size_t buflen)
    {
        // CSI 48 ; 2 ; <r> ; <g> ; <b> m -- RGB background color
        sprintf_s(buffer, buflen, VT100_CSI "48;2;%u;%u;%um", color.r, color.g, color.b);
        return buffer;
    }

public:
    static bool
    VTEnable(ConsoleState &state)
    {
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#define VTMODE  (ENABLE_VIRTUAL_TERMINAL_PROCESSING|ENABLE_PROCESSED_OUTPUT)

        bool result = false;
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

        state.vt = FALSE;
        state.mode = 0;
        state.cp = (UINT)-1;

        if (hStdOut != INVALID_HANDLE_VALUE && hStdOut != NULL) {
            DWORD omode = 0;
            if (GetConsoleMode(hStdOut, &omode)) {
                if ((VTMODE & omode) == VTMODE) { // enable
                    result = true;
                } else {
                    DWORD nmode = omode|VTMODE;
                    if (SetConsoleMode(hStdOut, nmode)) {
                        nmode = 0;
                        if (GetConsoleMode(hStdOut, &nmode)) {
                            if ((VTMODE & nmode) == VTMODE) { // able tp enabled
                                state.mode = omode;
                                result = true;
                            }
                        }
                    }
                }
            }

            if (result) {
                state.cp = GetConsoleOutputCP();
                if (CP_UTF8 == state.cp || SetConsoleOutputCP(CP_UTF8)) { // UTF8 enabled
                    GetConsoleCursorInfo(hStdOut, &state.cursor);
                    if (state.cursor.bVisible) { // hide cursor
                        CONSOLE_CURSOR_INFO cci = state.cursor;
                        cci.bVisible = FALSE;
                        SetConsoleCursorInfo(hStdOut, &cci);
                    }
                    state.vt = true;

                } else {
                    if (state.mode) // restore mode
                        SetConsoleMode(hStdOut, state.mode);
                }
            }
        }

        return state.vt;
    }

    static void
    VTRestore(const ConsoleState &state)
    {
        if (state.vt) {
            HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (INVALID_HANDLE_VALUE != hStdOut && NULL != hStdOut) {
                if (state.mode) {
                    SetConsoleMode(hStdOut, state.mode);
                }

                if (state.cp != CP_UTF8) {
                    SetConsoleOutputCP(state.cp);
                }

                if (state.cursor.bVisible) {
                    SetConsoleCursorInfo(hStdOut, &state.cursor);
                }
            }
        }
    }

    static bool
    ESCPressed()
    {
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

        if (INVALID_HANDLE_VALUE != hStdIn && NULL != hStdIn) {
            INPUT_RECORD ir = {0};
            DWORD read = 0;

            while (PeekConsoleInputA(hStdIn, &ir, 1, &read) && read) {
                ReadConsoleInputA(hStdIn, &ir, 1, &read);
                if (KEY_EVENT == ir.EventType && ir.Event.KeyEvent.bKeyDown) {
                    if (VK_ESCAPE == ir.Event.KeyEvent.wVirtualKeyCode) {
                        FlushConsoleInputBuffer(hStdIn);
                        return true;
                    }
                }
                read = 0;
            }
        }
        return false;
    }

private:
    enum VTMode mode_;
    ConsoleState state_;
    VTColor hilite_;
};

//end
