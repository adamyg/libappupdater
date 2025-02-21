//  $Id: TLibappupdater.cpp,v 1.7 2025/02/21 19:03:24 cvsuser Exp $
//
//  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2022 - 2025 Adam Young
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
#include <ios>
#include <iostream>
#include <iomanip>

#include "../src/VTColors.h"
#include "../src/VTSupport.h"
#include "../src/VTStream.h"

static void press(const char *msg);
static void stream();

int
main()
{
    stream();
    return 0;
}

static void
press(const char *msg = "Press ESC to continue ...")
{
    VTSupport::normal(std::cout);
    std::cout << '\n' << msg << std::endl;
    while (!VTSupport::ESCPressed());
}


static void
stream()
{
    static const char abc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ|abcdefghijklmnopqrstuvwxyz";
    std::ostream &out = std::cout;
    VTStream<std::ostream> strm(std::cout);

    // ---------------------------------------------------------------------------------
    // Erase sol/eol
    out << "\nerase_sol:\n" << '|' << abc << '|' << abc << '|';
    strm.cursor_back(_countof(abc));
    strm.erase_sol();
    out << "\nerase_eol:\n" << '|' << abc << '|' << abc << '|';
    strm.cursor_back(_countof(abc));
    strm.erase_eol();
    press();

    strm.clear();
    for (unsigned i = 0; i < 20; ++i)
        out << '|' << abc << '|' << abc << "|\n";
    out << "erase_up:\n";
    strm.cursor_prev(10);
    strm.erase_up();
    press();

    strm.clear();
    out << "erase_down:\n";
    for (unsigned i = 0; i < 20; ++i)
        out << '|' << abc << '|' << abc << "|\n";
    strm.cursor_prev(10);
    strm.erase_down();
    press();

    // ---------------------------------------------------------------------------------
    // VT colors
    out.width(16);
    out << "Foreground" << '|';
    out.width(16);
    out << "Background" << '|';
    out << '\n';

    for (unsigned idx = VTSupport::Black; idx <= VTSupport::BrightWhite; ++idx) {
        const VTSupport::Color color = (VTSupport::Color)(idx);
        const char *desc = VTSupport::to_ascii(color);
       
        strm.foreground(color);
        out.width(16);
        out << desc << '|';
        strm.normal();

        strm.background(color);
        out.width(16);
        out << desc << '|';
        strm.normal();
        out << '\n';
    }
    press();

    // ---------------------------------------------------------------------------------
    // Foreground, 0..255
    out.fill('0');
    for (unsigned idx = 0; idx <= 255; ++idx) {
        if (idx && (idx % 16) == 0) {
            strm.normal();
            out << '\n';
        }
        strm.foreground(idx);
        out.width(6);
        out << idx << " ";
    }
    strm.normal();
    out << '\n';

    // ---------------------------------------------------------------------------------
    // Background, 0..255
    out.fill(' ');
    for (unsigned idx = 0; idx <= 255; ++idx) {
        if (idx && (idx % 16) == 0) {
            strm.normal();
            out << '\n';
        }
        strm.background(idx);
        out.width(6);
        out << idx << " ";
    }
    press();

    // ---------------------------------------------------------------------------------
    // RGB colors
    const RGBColors::RGBName *names = RGBColors::names();
    out.fill(' ');
    for (unsigned idx = 0; names->name; ++names, ++idx) {
        strm.foreground(RGBColors::black);
        strm.background(names->rgb);
        out.width(8);
        out << idx << ": ";
        out.width(32);
        out << names->name;
        strm.normal() << '\n';
    }           
    press();
}

//end
