#pragma once
//  $Id: VTSupport.h,v 1.8 2023/10/24 13:56:24 cvsuser Exp $
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

#include "common.h"

#include <shlwapi.h>
#if defined(__WATCOMC__)
#if (_WIN32_IE < 0x0500)
LWSTDAPI_(COLORREF) ColorHLSToRGB( WORD, WORD, WORD );
#endif
#endif
#if defined(PRAGMA_COMMENT_LIB)
#pragma comment( lib, "shlwapi.lib" )
#endif

///////////////////////////////////////////////////////////////////////////////
//  VTSupport

struct VTColor {
    VTColor() : r(0), g(0), b(0) {}
    VTColor(uint8_t r__, uint8_t g__, uint8_t b__) : r(r__), g(g__), b(b__) {}
    VTColor(DWORD color) {
        r = GetRValue(color), g = GetGValue(color), b = GetBValue(color);
    }
    VTColor& operator=(const DWORD color) {
        r = GetRValue(color), g = GetGValue(color), b = GetBValue(color);
        return *this;
    }
    VTColor scale(unsigned value = 3) const {
        return VTColor(r / value, g / value, b / value);
    }
    uint8_t r, g, b;
};

class VTSupport {
    VTSupport(const VTSupport &); // delete
    VTSupport& operator=(const VTSupport &); // delete

public:
    enum Color {
        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
        BrightBlack,
        BrightRed,
        BrightGreen,
        BrightYellow,
        BrightBlue,
        BrightMagenta,
        BrightCyan,
        BrightWhite
    };

    static const char *to_ascii(Color color) {
        const char *desc = "Unknown";
        switch (color) {
        case VTSupport::Black: desc = "Black"; break;
        case VTSupport::Red: desc = "Red"; break;
        case VTSupport::Green: desc = "Green"; break;
        case VTSupport::Yellow: desc = "Yellow"; break;
        case VTSupport::Blue: desc = "Blue"; break;
        case VTSupport::Magenta: desc = "Magenta"; break;
        case VTSupport::Cyan: desc = "Cyan"; break;
        case VTSupport::White: desc = "White"; break;
        case VTSupport::BrightBlack: desc = "BrightBlack"; break;
        case VTSupport::BrightRed: desc = "BrightRed"; break;
        case VTSupport::BrightGreen: desc = "BrightGreen"; break;
        case VTSupport::BrightYellow: desc = "BrightYellow"; break;
        case VTSupport::BrightBlue: desc = "BrightBlue"; break;
        case VTSupport::BrightMagenta: desc = "BrightMagenta"; break;
        case VTSupport::BrightCyan: desc = "BrightCyan"; break;
        case VTSupport::BrightWhite: desc = "BrightWhite"; break;
        default: break;
        }
        return desc;
    }

    friend std::ostream& operator<<(std::ostream &out, Color color) {
        return out << VTSupport::to_ascii(color);
    }

    struct ConsoleState {
        bool vt;
        DWORD mode;
        UINT cp;
        CONSOLE_CURSOR_INFO cursor;
    };

#define VT100_ESCAPE            "\x1b"
#define VT100_CSI               VT100_ESCAPE "["
#define VT100_SGR(id___)        VT100_CSI #id___ "m"

    enum VTMode {
        VTMODE_UNINIT = -1,
        VTMODE_UNAVAIL,
        VTMODE_INACTIVE,
        VTMODE_ACTIVE
    };

    enum CursorShape {
        UserShape,              // Default cursor shape configured by the user
        BlinkingBlock,          // Blinking block cursor shape
        SteadyBlock,            // Steady block cursor shape
        BlinkingUnderline,      // Blinking underline cursor shape
        SteadyUnderline,        // Steady underline cursor shape
        Blinking,               // Blinking bar cursor shape
        SteadyBar               // Steady bar cursor shape
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
        return VTColor(ColorHLSToRGB(hue, 120, 240));
    }

    template <typename Stream>
    static Stream& normal(Stream &out)
    {
        return out << VT100_SGR(0);
    }

    template <typename Stream>
    static Stream& bold(Stream &out, bool on = true)
    {
        return out << VT100_SGR(on ? 1 : 22);
    }

    template <typename Stream>
    static Stream& underline(Stream &out, bool on = true)
    {
        return out << VT100_SGR(on ? 4 : 24);
    }

    template <typename Stream>
    static Stream& inverse(Stream &out, bool on = true)
    {
        return out << VT100_SGR(on ? 7: 27);
    }

    template <typename Stream>
    static Stream& flush(Stream &out)
    {
        out.flush();
#if defined(__WATCOMC__)
        if (&out == &std::cout)
            std::fflush(stdout);
        else if (&out == &std::cerr)
            std::fflush(stderr);
#endif
        return out;
    }

    /// Screen selection (main otherwise alternative)
    template <typename Stream>
    static Stream& screen_buffer(Stream &out, bool alt = false)
    {
        if (alt)
            return out << VT100_CSI << "?1049h";    // Alternate Screen Buffer.
        return out << VT100_CSI << "?1049l";        // Main Screen Buffer.
    }

    /// Cursor moves to <x>; <y> coordinate within the viewport,
    template <typename Stream>
    static Stream& cursor_pos(Stream &out, unsigned y, unsigned x)
    {
        return out << VT100_CSI << y << ';' << x << 'H';
    }

    /// Customization of the cursor shape.
    template <typename Stream>
    static Stream& cursor_shape(Stream &out, CursorShape shape)
    {
        return out << VT100_CSI << reinterpret_cast<unsigned>(shape) << 'q';
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

    /// Saves the cursor position/state in SCO console mode.
    template <typename Stream>
    static Stream& cursor_save(Stream &out)
    {
        return out << VT100_CSI << 's';
    }

    /// Restores the cursor position/state in SCO console mode.
    template <typename Stream>
    static Stream& cursor_restore(Stream &out)
    {
        return out << VT100_CSI << 'u';
    }

    /// Erases from the current cursor position (inclusive) to the end of the line
    template <typename Stream>
    static Stream& erase_eol(Stream &out)
    {
        return out << VT100_CSI "0K";
    }

    /// Erases from the beginning of the line up to and including the current cursor position.
    template <typename Stream>
    static Stream& erase_sol(Stream &out)
    {
        return out << VT100_CSI "1K";
    }

    /// Erases the entire line.
    template <typename Stream>
    static Stream& erase(Stream &out)
    {
        return out << VT100_CSI "2K";
    }

    /// Erases from the current cursor position (inclusive) to the end of the display
    template <typename Stream>
    static Stream& erase_down(Stream &out)
    {
        return out << VT100_CSI "0J";
    }

    /// Erases from the beginning of the display up to and including the current cursor position.
    template <typename Stream>
    static Stream& erase_up(Stream &out)
    {
        return out << VT100_CSI "1J";
    }

    /// Erases the entire display.
    template <typename Stream>
    static Stream& clear(Stream &out)
    {
        return out << VT100_CSI "2J";
    }

    /// Set foreground color)
    template <typename Stream>
    static Stream& foreground(Stream &out, Color color)
    {
        if (color >= Black && color <= White) // 30 .. 37
            return out << VT100_CSI << ((30 - Black) + color) << 'm';

        else if (color >= BrightBlack && color <= BrightWhite) // 90 .. 97
            return out << VT100_CSI << ((90 - BrightBlack) + color) << 'm';

        return out;
    }

    /// Set foreground color to index (0..255)
    template <typename Stream>
    static Stream& foreground(Stream &out, unsigned idx)
    {
        char buffer[32];
        return out << VTForeground(idx, buffer, sizeof(buffer));
    }

    /// Set foreground color RGB
    template <typename Stream>
    static Stream& foreground(Stream &out, const VTColor color)
    {
        char buffer[32];
        return out << VTForeground(color, buffer, sizeof(buffer));
    }

    template <typename Stream>
    static Stream& foreground_scaled(Stream &out, const VTColor color, unsigned value = 3)
    {
        if (value) out << foreground(out,  color.scale(value));
    }

    /// Set foreground color)
    template <typename Stream>
    static Stream& background(Stream &out, Color color)
    {
        if (color >= Black && color <= White) // 40 .. 47
            return out << VT100_CSI << ((40 - Black) + color) << 'm';

        else if (color >= BrightBlack && color <= BrightWhite) // 100 .. 107
            return out << VT100_CSI << ((100 - BrightBlack) + color) << 'm';

        return out;
    }

    /// Set background color to index (0..255)
    template <typename Stream>
    static Stream& background(Stream &out, unsigned idx)
    {
        char buffer[32];
        return out << VTBackground(idx, buffer, sizeof(buffer));
    }

    /// Set background color RGB
    template <typename Stream>
    static Stream& background(Stream &out, const VTColor color)
    {
        char buffer[32];
        return out << VTBackground(color, buffer, sizeof(buffer));
    }

    /// Set background color to the scaled RGB (default %33)
    template <typename Stream>
    static Stream& background_scaled(Stream &out, const VTColor color, unsigned value = 3)
    {
        if (value) background(out, color.scale(value));
        return out;
    }

public:
    static const char *
    VTForeground(const VTColor color, char *buffer, size_t buflen)
    {
        // CSI 38 ; 2 ; <r> ; <g> ; <b> m -- RGB foreground color
        sprintf_s(buffer, buflen, VT100_CSI "38;2;%u;%u;%um", color.r, color.g, color.b);
        return buffer;
    }

    static const char *
    VTForeground(unsigned idx, char *buffer, size_t buflen)
    {
        // CSI 38 ; 5 ; <idx> m -- Foreground color
        sprintf_s(buffer, buflen, VT100_CSI "38;5;%um", idx);
        return buffer;
    }

    static const char *
    VTBackground(const VTColor color, char *buffer, size_t buflen)
    {
        // CSI 48 ; 2 ; <r> ; <g> ; <b> m -- RGB background color
        sprintf_s(buffer, buflen, VT100_CSI "48;2;%u;%u;%um", color.r, color.g, color.b);
        return buffer;
    }

    static const char *
    VTBackground(unsigned idx, char *buffer, size_t buflen)
    {
        // CSI 48 ; 5 ; <idx> m -- Background color
        sprintf_s(buffer, buflen, VT100_CSI "48;5;%um", idx);
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
