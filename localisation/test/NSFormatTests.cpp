/*  $Id: NSFormatTests.cpp,v 1.3 2022/06/09 08:46:30 cvsuser Exp $
 *
 *  NSLocalization - String tests.
 *
 *  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
 *
 *  Copyright (c) 2021 Adam Young
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

/*
 * Copyright (c) 2011 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "..\NSFormat.h"
#if defined(__WATCOMC__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#else
#include <cassert>
#include <cstring>
#endif

#if defined(_MSC_VER)
#pragma warning(disable:4146)
#endif
#if defined(__WATCOMC__)
#pragma warning 13 9    // unreachable code
#pragma warning 14 9    // no reference to symbol 'xxx' 
#pragma warning 368 9   // conditional expression in if statement is always false
#pragma warning 887 9   // unary '-' of unsigned operand produces unsigned result
#endif


static void
TEST(const char *result, int /*ret*/, const char *fmt, ...)
{
    char buffer[1024];
    va_list ap;
    va_start(ap, fmt);
    NSFormat::vformat(buffer, sizeof(buffer), fmt, ap);
    assert(0 == strcmp(result, buffer));
    va_end(ap);
}


static void
test_1(void)
{
    /* Basic */
    const char *hello_world = "hello, world";

    TEST(":hello, world:",          -1, ":%s:",         hello_world);
    TEST(":hello, world:",          -1, ":%10s:",       hello_world);
    TEST(":hello, wor:",            -1, ":%.10s:",      hello_world);
    TEST(":hello, world:",          -1, ":%-10s:",      hello_world);
    TEST(":hello, world:",          -1, ":%.15s:",      hello_world);
    TEST(":hello, world   :",       -1, ":%-15s:",      hello_world);
    TEST(":     hello, wor:",       -1, ":%15.10s:",    hello_world);
    TEST(":     hello, wor:",       -1, ":%15.10s:",    hello_world);
    TEST(":hello, wor     :",       -1, ":%-15.10s:",   hello_world);
    TEST(":hello, wor:",            -1, ":%5.10s:",     hello_world);

    TEST("  0",                     -1, "%3d",          0);
    TEST("123456789",               -1, "%3d",          123456789);
    TEST("-10",                     -1, "%3d",          -10);
    TEST("-123456789",              -1, "%3d",          -123456789);

    TEST("0  ",                     -1, "%-3d",         0);
    TEST("123456789",               -1, "%-3d",         123456789);
    TEST("-10",                     -1, "%-3d",         -10);
    TEST("-123456789",              -1, "%-3d",         -123456789);

    TEST("000",                     -1, "%03d",         0);
    TEST("001",                     -1, "%03d",         1);
    TEST("123456789",               -1, "%03d",         123456789);
    TEST("-10",                     -1, "%03d",         -10);
    TEST("-123456789",              -1, "%03d",         -123456789);

    TEST("'   10'",                 -1, "'%5d'",        10);
    TEST("'10   '",                 -1, "'%-5d'",       10);
    TEST("'00010'",                 -1, "'%05d'",       10);
    TEST("'  +10'",                 -1, "'%+5d'",       10);
    TEST("'+10  '",                 -1, "'%-+5d'",      10);

    TEST("'10.3'",                  -1, "'%.1f'",       10.3456);
    TEST("'10.35'",                 -1, "'%.2f'",       10.3456);
    TEST("'   10.35'",              -1, "'%8.2f'",      10.3456);
    TEST("' 10.3456'",              -1, "'%8.4f'",      10.3456);
    TEST("'00010.35'",              -1, "'%08.2f'",     10.3456);
    TEST("'10.35   '",              -1, "'%-8.2f'",     10.3456);
    TEST("'101234567.35'",          -1, "'%-8.2f'",     101234567.3456);

    TEST("AbC",                     -1, "%c%c%c",       'A', 'b', 'C');

    /* Ein String ohne alles */
    TEST("Hallo heimur",            12, "Hallo heimur");

    /* Einfache Konvertierungen */
    TEST("Hallo heimur",            12, "%s",           "Hallo heimur");
    TEST("1024",                     4, "%d",           1024);
    TEST("-1024",                    5, "%d",           -1024);
    TEST("1024",                     4, "%i",           1024);
    TEST("-1024",                    5, "%i",           -1024);
    TEST("1024",                     4, "%u",           1024u);
    TEST("4294966272",              10, "%u",           -1024u);
    TEST("777",                      3, "%o",           0777u);
    TEST("37777777001",             11, "%o",           -0777u);
    TEST("1234abcd",                 8, "%x",           0x1234abcdu);
    TEST("edcb5433",                 8, "%x",           -0x1234abcdu);
    TEST("1234ABCD",                 8, "%X",           0x1234abcdu);
    TEST("EDCB5433",                 8, "%X",           -0x1234abcdu);
    TEST("x",                        1, "%c",           'x');
    TEST("%",                        1, "%%");

    /* Mit %c kann man auch Nullbytes ausgeben */
//  TEST("\0",                       1, "%c",           '\0');

    /* Vorzeichen erzwingen (Flag +) */
    TEST("Hallo heimur",            12, "%+s",          "Hallo heimur");
    TEST("+1024",                    5, "%+d",          1024);
    TEST("-1024",                    5, "%+d",          -1024);
    TEST("+1024",                    5, "%+i",          1024);
    TEST("-1024",                    5, "%+i",          -1024);
    TEST("1024",                     4, "%+u",          1024u);
    TEST("4294966272",              10, "%+u",          -1024u);
    TEST("777",                      3, "%+o",          0777u);
    TEST("37777777001",             11, "%+o",          -0777u);
    TEST("1234abcd",                 8, "%+x",          0x1234abcdu);
    TEST("edcb5433",                 8, "%+x",          -0x1234abcdu);
    TEST("1234ABCD",                 8, "%+X",          0x1234abcdu);
    TEST("EDCB5433",                 8, "%+X",          -0x1234abcdu);
    TEST("x",                        1, "%+c",          'x');

    /* Vorzeichenplatzhalter erzwingen (Flag <space>) */
    TEST("Hallo heimur",            12, "% s",          "Hallo heimur");
#if (0)
    TEST(" 1024",                    5, "% d",          1024);
    TEST("-1024",                    5, "% d",          -1024);
    TEST(" 1024",                    5, "% i",          1024);
    TEST("-1024",                    5, "% i",          -1024);
#endif
    TEST("1024",                     4, "% u",          1024u);
    TEST("4294966272",              10, "% u",          -1024u);
    TEST("777",                      3, "% o",          0777u);
    TEST("37777777001",             11, "% o",          -0777u);
    TEST("1234abcd",                 8, "% x",          0x1234abcdu);
    TEST("edcb5433",                 8, "% x",          -0x1234abcdu);
    TEST("1234ABCD",                 8, "% X",          0x1234abcdu);
    TEST("EDCB5433",                 8, "% X",          -0x1234abcdu);
    TEST("x",                        1, "% c",          'x');

    /* Flag + hat Vorrang über <space> */
    TEST("Hallo heimur",            12, "%+ s",         "Hallo heimur");
    TEST("+1024",                    5, "%+ d",         1024);
    TEST("-1024",                    5, "%+ d",         -1024);
    TEST("+1024",                    5, "%+ i",         1024);
    TEST("-1024",                    5, "%+ i",         -1024);
    TEST("1024",                     4, "%+ u",         1024u);
    TEST("4294966272",              10, "%+ u",         -1024u);
    TEST("777",                      3, "%+ o",         0777u);
    TEST("37777777001",             11, "%+ o",         -0777u);
    TEST("1234abcd",                 8, "%+ x",         0x1234abcdu);
    TEST("edcb5433",                 8, "%+ x",         -0x1234abcdu);
    TEST("1234ABCD",                 8, "%+ X",         0x1234abcdu);
    TEST("EDCB5433",                 8, "%+ X",         -0x1234abcdu);
    TEST("x",                        1, "%+ c",         'x');

    /* Alternative Form */
    TEST("0777",                     4, "%#o",          0777u);
    TEST("037777777001",            12, "%#o",          -0777u);
    TEST("0x1234abcd",              10, "%#x",          0x1234abcdu);
    TEST("0xedcb5433",              10, "%#x",          -0x1234abcdu);
    TEST("0X1234ABCD",              10, "%#X",          0x1234abcdu);
    TEST("0XEDCB5433",              10, "%#X",          -0x1234abcdu);
    TEST("0",                        1, "%#o",          0u);
    TEST("0",                        1, "%#x",          0u);
    TEST("0",                        1, "%#X",          0u);

    /* Feldbreite: Kleiner als Ausgabe */
    TEST("Hallo heimur",            12, "%1s",          "Hallo heimur");
    TEST("1024",                     4, "%1d",          1024);
    TEST("-1024",                    5, "%1d",          -1024);
    TEST("1024",                     4, "%1i",          1024);
    TEST("-1024",                    5, "%1i",          -1024);
    TEST("1024",                     4, "%1u",          1024u);
    TEST("4294966272",              10, "%1u",          -1024u);
    TEST("777",                      3, "%1o",          0777u);
    TEST("37777777001",             11, "%1o",          -0777u);
    TEST("1234abcd",                 8, "%1x",          0x1234abcdu);
    TEST("edcb5433",                 8, "%1x",          -0x1234abcdu);
    TEST("1234ABCD",                 8, "%1X",          0x1234abcdu);
    TEST("EDCB5433",                 8, "%1X",          -0x1234abcdu);
    TEST("x",                        1, "%1c",          'x');

    /* Feldbreite: Größer als Ausgabe */
    TEST("               Hallo",    20, "%20s",         "Hallo");
    TEST("                1024",    20, "%20d",         1024);
    TEST("               -1024",    20, "%20d",         -1024);
    TEST("                1024",    20, "%20i",         1024);
    TEST("               -1024",    20, "%20i",         -1024);
    TEST("                1024",    20, "%20u",         1024u);
    TEST("          4294966272",    20, "%20u",         -1024u);
    TEST("                 777",    20, "%20o",         0777u);
    TEST("         37777777001",    20, "%20o",         -0777u);
    TEST("            1234abcd",    20, "%20x",         0x1234abcdu);
    TEST("            edcb5433",    20, "%20x",         -0x1234abcdu);
    TEST("            1234ABCD",    20, "%20X",         0x1234abcdu);
    TEST("            EDCB5433",    20, "%20X",         -0x1234abcdu);
    TEST("                   x",    20, "%20c",         'x');

    /* Feldbreite: Linksbündig */
    TEST("Hallo               ",    20, "%-20s",        "Hallo");
    TEST("1024                ",    20, "%-20d",        1024);
    TEST("-1024               ",    20, "%-20d",        -1024);
    TEST("1024                ",    20, "%-20i",        1024);
    TEST("-1024               ",    20, "%-20i",        -1024);
    TEST("1024                ",    20, "%-20u",        1024u);
    TEST("4294966272          ",    20, "%-20u",        -1024u);
    TEST("777                 ",    20, "%-20o",        0777u);
    TEST("37777777001         ",    20, "%-20o",        -0777u);
    TEST("1234abcd            ",    20, "%-20x",        0x1234abcdu);
    TEST("edcb5433            ",    20, "%-20x",        -0x1234abcdu);
    TEST("1234ABCD            ",    20, "%-20X",        0x1234abcdu);
    TEST("EDCB5433            ",    20, "%-20X",        -0x1234abcdu);
    TEST("x                   ",    20, "%-20c",        'x');

    /* Feldbreite: Padding mit 0 */
    TEST("00000000000000001024",    20, "%020d",        1024);
    TEST("-0000000000000001024",    20, "%020d",        -1024);
    TEST("00000000000000001024",    20, "%020i",        1024);
    TEST("-0000000000000001024",    20, "%020i",        -1024);
    TEST("00000000000000001024",    20, "%020u",        1024u);
    TEST("00000000004294966272",    20, "%020u",        -1024u);
    TEST("00000000000000000777",    20, "%020o",        0777u);
    TEST("00000000037777777001",    20, "%020o",        -0777u);
    TEST("0000000000001234abcd",    20, "%020x",        0x1234abcdu);
    TEST("000000000000edcb5433",    20, "%020x",        -0x1234abcdu);
    TEST("0000000000001234ABCD",    20, "%020X",        0x1234abcdu);
    TEST("000000000000EDCB5433",    20, "%020X",        -0x1234abcdu);

    /* Feldbreite: Padding und alternative Form */
    TEST("                0777",    20, "%#20o",        0777u);
    TEST("        037777777001",    20, "%#20o",        -0777u);
    TEST("          0x1234abcd",    20, "%#20x",        0x1234abcdu);
    TEST("          0xedcb5433",    20, "%#20x",        -0x1234abcdu);
    TEST("          0X1234ABCD",    20, "%#20X",        0x1234abcdu);
    TEST("          0XEDCB5433",    20, "%#20X",        -0x1234abcdu);

    TEST("00000000000000000777",    20, "%#020o",       0777u);
    TEST("00000000037777777001",    20, "%#020o",       -0777u);
    TEST("0x00000000001234abcd",    20, "%#020x",       0x1234abcdu);
    TEST("0x0000000000edcb5433",    20, "%#020x",       -0x1234abcdu);
    TEST("0X00000000001234ABCD",    20, "%#020X",       0x1234abcdu);
    TEST("0X0000000000EDCB5433",    20, "%#020X",       -0x1234abcdu);

    /* Feldbreite: - hat Vorrang vor 0 */
    TEST("Hallo               ",    20, "%0-20s",       "Hallo");
    TEST("1024                ",    20, "%0-20d",       1024);
    TEST("-1024               ",    20, "%0-20d",       -1024);
    TEST("1024                ",    20, "%0-20i",       1024);
    TEST("-1024               ",    20, "%0-20i",       -1024);
    TEST("1024                ",    20, "%0-20u",       1024u);
    TEST("4294966272          ",    20, "%0-20u",       -1024u);
    TEST("777                 ",    20, "%-020o",       0777u);
    TEST("37777777001         ",    20, "%-020o",       -0777u);
    TEST("1234abcd            ",    20, "%-020x",       0x1234abcdu);
    TEST("edcb5433            ",    20, "%-020x",       -0x1234abcdu);
    TEST("1234ABCD            ",    20, "%-020X",       0x1234abcdu);
    TEST("EDCB5433            ",    20, "%-020X",       -0x1234abcdu);
    TEST("x                   ",    20, "%-020c",       'x');

    /* Feldbreite: Aus Parameter */
    TEST("               Hallo",    20, "%*s",          20, "Hallo");
    TEST("                1024",    20, "%*d",          20, 1024);
    TEST("               -1024",    20, "%*d",          20, -1024);
    TEST("                1024",    20, "%*i",          20, 1024);
    TEST("               -1024",    20, "%*i",          20, -1024);
    TEST("                1024",    20, "%*u",          20, 1024u);
    TEST("          4294966272",    20, "%*u",          20, -1024u);
    TEST("                 777",    20, "%*o",          20, 0777u);
    TEST("         37777777001",    20, "%*o",          20, -0777u);
    TEST("            1234abcd",    20, "%*x",          20, 0x1234abcdu);
    TEST("            edcb5433",    20, "%*x",          20, -0x1234abcdu);
    TEST("            1234ABCD",    20, "%*X",          20, 0x1234abcdu);
    TEST("            EDCB5433",    20, "%*X",          20, -0x1234abcdu);
    TEST("                   x",    20, "%*c",          20, 'x');

    /* Präzision / Mindestanzahl von Ziffern */
    TEST("Hallo heimur",            12, "%.20s",        "Hallo heimur");
    TEST("00000000000000001024",    20, "%.20d",        1024);
    TEST("-00000000000000001024",   21, "%.20d",        -1024);
    TEST("00000000000000001024",    20, "%.20i",        1024);
    TEST("-00000000000000001024",   21, "%.20i",        -1024);
    TEST("00000000000000001024",    20, "%.20u",        1024u);
    TEST("00000000004294966272",    20, "%.20u",        -1024u);
    TEST("00000000000000000777",    20, "%.20o",        0777u);
    TEST("00000000037777777001",    20, "%.20o",        -0777u);
    TEST("0000000000001234abcd",    20, "%.20x",        0x1234abcdu);
    TEST("000000000000edcb5433",    20, "%.20x",        -0x1234abcdu);
    TEST("0000000000001234ABCD",    20, "%.20X",        0x1234abcdu);
    TEST("000000000000EDCB5433",    20, "%.20X",        -0x1234abcdu);

    /* Feldbreite und Präzision */
    TEST("               Hallo",    20, "%20.5s",       "Hallo heimur");
    TEST("               01024",    20, "%20.5d",       1024);
    TEST("              -01024",    20, "%20.5d",       -1024);
    TEST("               01024",    20, "%20.5i",       1024);
    TEST("              -01024",    20, "%20.5i",       -1024);
    TEST("               01024",    20, "%20.5u",       1024u);
    TEST("          4294966272",    20, "%20.5u",       -1024u);
    TEST("               00777",    20, "%20.5o",       0777u);
    TEST("         37777777001",    20, "%20.5o",       -0777u);
    TEST("            1234abcd",    20, "%20.5x",       0x1234abcdu);
    TEST("          00edcb5433",    20, "%20.10x",      -0x1234abcdu);
    TEST("            1234ABCD",    20, "%20.5X",       0x1234abcdu);
    TEST("          00EDCB5433",    20, "%20.10X",      -0x1234abcdu);

    /* Präzision: 0 wird ignoriert */
    TEST("               Hallo",    20, "%020.5s",      "Hallo heimur");
    TEST("               01024",    20, "%020.5d",      1024);
    TEST("              -01024",    20, "%020.5d",      -1024);
    TEST("               01024",    20, "%020.5i",      1024);
    TEST("              -01024",    20, "%020.5i",      -1024);
    TEST("               01024",    20, "%020.5u",      1024u);
    TEST("          4294966272",    20, "%020.5u",      -1024u);
    TEST("               00777",    20, "%020.5o",      0777u);
    TEST("         37777777001",    20, "%020.5o",      -0777u);
    TEST("            1234abcd",    20, "%020.5x",      0x1234abcdu);
    TEST("          00edcb5433",    20, "%020.10x",     -0x1234abcdu);
    TEST("            1234ABCD",    20, "%020.5X",      0x1234abcdu);
    TEST("          00EDCB5433",    20, "%020.10X",     -0x1234abcdu);

    /* Präzision 0 */
    TEST("",                         0, "%.0s",         "Hallo heimur");
    TEST("                    ",    20, "%20.0s",       "Hallo heimur");
    TEST("",                         0, "%.s",          "Hallo heimur");
    TEST("                    ",    20, "%20.s",        "Hallo heimur");
    TEST("                1024",    20, "%20.0d",       1024);
    TEST("               -1024",    20, "%20.d",        -1024);
    TEST("                    ",    20, "%20.d",        0);
    TEST("                1024",    20, "%20.0i",       1024);
    TEST("               -1024",    20, "%20.i",        -1024);
    TEST("                    ",    20, "%20.i",        0);
    TEST("                1024",    20, "%20.u",        1024u);
    TEST("          4294966272",    20, "%20.0u",       -1024u);
    TEST("                    ",    20, "%20.u",        0u);
    TEST("                 777",    20, "%20.o",        0777u);
    TEST("         37777777001",    20, "%20.0o",       -0777u);
    TEST("                    ",    20, "%20.o",        0u);
    TEST("            1234abcd",    20, "%20.x",        0x1234abcdu);
    TEST("            edcb5433",    20, "%20.0x",       -0x1234abcdu);
    TEST("                    ",    20, "%20.x",        0u);
    TEST("            1234ABCD",    20, "%20.X",        0x1234abcdu);
    TEST("            EDCB5433",    20, "%20.0X",       -0x1234abcdu);
    TEST("                    ",    20, "%20.X",        0u);

    /* Negative Präzision wird ignoriert */
    /* XXX glibc tut nicht, was ich erwartet habe, vorerst deaktiviert... */
#if 0
    TEST("Hallo heimur",            12, "%.-42s",       "Hallo heimur");
    TEST("1024",                     4, "%.-42d",       1024);
    TEST("-1024",                    5, "%.-42d",       -1024);
    TEST("1024",                     4, "%.-42i",       1024);
    TEST("-1024",                    5, "%.-42i",       -1024);
    TEST("1024",                     4, "%.-42u",       1024u);
    TEST("4294966272",              10, "%.-42u",       -1024u);
    TEST("777",                      3, "%.-42o",       0777u);
    TEST("37777777001",             11, "%.-42o",       -0777u);
    TEST("1234abcd",                 8, "%.-42x",       0x1234abcdu);
    TEST("edcb5433",                 8, "%.-42x",       -0x1234abcdu);
    TEST("1234ABCD",                 8, "%.-42X",       0x1234abcdu);
    TEST("EDCB5433",                 8, "%.-42X",       -0x1234abcdu);
#endif

    /*
     * Precision and field width from parameters.
     *
     *  + has priority over <space>,
     *  - has priority over 0 (which is ignored anyway, because a precision is given);
     */
    TEST("Hallo               ",    20, "% -0+*.*s",    20,  5, "Hallo heimur");
    TEST("+01024              ",    20, "% -0+*.*d",    20,  5,  1024);
    TEST("-01024              ",    20, "% -0+*.*d",    20,  5,  -1024);
    TEST("+01024              ",    20, "% -0+*.*i",    20,  5,  1024);
    TEST("-01024              ",    20, "% 0-+*.*i",    20,  5,  -1024);
    TEST("01024               ",    20, "% 0-+*.*u",    20,  5,  1024u);
    TEST("4294966272          ",    20, "% 0-+*.*u",    20,  5,  -1024u);
    TEST("00777               ",    20, "%+ -0*.*o",    20,  5,  0777u);
    TEST("37777777001         ",    20, "%+ -0*.*o",    20,  5,  -0777u);
    TEST("1234abcd            ",    20, "%+ -0*.*x",    20,  5,  0x1234abcdu);
    TEST("00edcb5433          ",    20, "%+ -0*.*x",    20, 10,  -0x1234abcdu);
    TEST("1234ABCD            ",    20, "% -+0*.*X",    20,  5,  0x1234abcdu);
    TEST("00EDCB5433          ",    20, "% -+0*.*X",    20, 10,  -0x1234abcdu);
}


#ifndef ULONGLONG
#define ULONGLONG unsigned long long
#endif
#ifndef LONGLONG
#define LONGLONG long long
#endif

static void
OK(int ok, const char *fmt, ...)
{
    if (ok) return;
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    abort();
}


static void
test_2(void)
{
    char buffer[100];
    const char *format;
    const double pnumber = 789456123.0;
    int x, r;

    format = "%+#23.15e";
    r = NSFormat::format(buffer,sizeof(buffer),format,pnumber);
        // https://stackoverflow.com/questions/9226400/portable-printing-of-exponent-of-a-double-to-c-iostreams/35738440#35738440
    OK( r==23, "return count wrong\n");
    if (buffer[0] == ' ') {
        OK( 0==strcmp(buffer," +7.894561230000000e+08"),"+#23.15e failed: '%s'\n", buffer);
    } else{
        OK( 0==strcmp(buffer,"+7.894561230000000e+008"),"+#23.15e failed: '%s'\n", buffer);
    }

    format = "%-#23.15e";
    r = NSFormat::format(buffer,sizeof(buffer),format,pnumber);
    OK( r==23, "return count wrong\n");
    if (buffer[21] == ' ') {
        OK( 0==strcmp(buffer,"7.894561230000000e+08  "),"-#23.15e failed: '%s'\n", buffer);
    } else {
        OK( 0==strcmp(buffer,"7.894561230000000e+008 "),"-#23.15e failed: '%s'\n", buffer);
    }

    format = "%#23.15e";
    r = NSFormat::format(buffer,sizeof(buffer),format,pnumber);
    OK( r==23, "return count wrong\n");
    if (buffer[1] == ' ') {
        OK( 0==strcmp(buffer,"  7.894561230000000e+08"),"#23.15e failed: '%s'\n", buffer);
    } else {
        OK( 0==strcmp(buffer," 7.894561230000000e+008"),"#23.15e failed: '%s'\n", buffer);
    }

    format = "%#1.1g";
    r = NSFormat::format(buffer,sizeof(buffer),format,pnumber);
    if (r == 7) {
        OK( 0==strcmp(buffer,"8.e+008"),"#1.1g failed: '%s'\n", buffer);
    } else {
        OK( r==6, "return count wrong\n");
        OK( 0==strcmp(buffer,"8.e+08"),"#1.1g failed: '%s'\n", buffer);
    }

    format = "%I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,((ULONGLONG)0xffffffff)*0xffffffff);
    OK( r==11, "return count wrong\n");
    OK( 0==strcmp(buffer,"-8589934591"),"Problem with long long\n");

    format = "%+8I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"    +100") && r==8,"+8I64d failed: '%s'\n", buffer);

    format = "%+.8I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"+00000100") && r==9,"+.8I64d failed: '%s'\n", buffer);

    format = "%+10.8I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer," +00000100") && r==10,"+10.8I64d failed: '%s'\n", buffer);

    format = "%_1I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"_1I64d") && r==6,"_1I64d failed\n");

    format = "%-1.5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer,"-00100") && r==6,"-1.5I64d failed: '%s'\n", buffer);

    format = "%5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"  100") && r==5,"5I64d failed: '%s'\n", buffer);

    format = "%5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer," -100") && r==5,"5I64d failed: '%s'\n", buffer);

    format = "%-5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"100  ") && r==5,"-5I64d failed: '%s'\n", buffer);

    format = "%-5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer,"-100 ") && r==5,"-5I64d failed: '%s'\n", buffer);

    format = "%-.5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"00100") && r==5,"-.5I64d failed: '%s'\n", buffer);

    format = "%-.5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer,"-00100") && r==6,"-.5I64d failed: '%s'\n", buffer);

    format = "%-8.5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"00100   ") && r==8,"-8.5I64d failed: '%s'\n", buffer);

    format = "%-8.5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer,"-00100  ") && r==8,"-8.5I64d failed: '%s'\n", buffer);

    format = "%05I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"00100") && r==5,"05I64d failed: '%s'\n", buffer);

    format = "%05I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer,"-0100") && r==5,"05I64d failed: '%s'\n", buffer);

    format = "% I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer," 100") && r==4,"' I64d' failed: '%s'\n", buffer);

    format = "% I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer,"-100") && r==4,"' I64d' failed: '%s'\n", buffer);

    format = "% 5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"  100") && r==5,"' 5I64d' failed: '%s'\n", buffer);

    format = "% 5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer," -100") && r==5,"' 5I64d' failed: '%s'\n", buffer);

    format = "% .5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer," 00100") && r==6,"' .5I64d' failed: '%s'\n", buffer);

    format = "% .5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer,"-00100") && r==6,"' .5I64d' failed: '%s'\n", buffer);

    format = "% 8.5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"   00100") && r==8,"' 8.5I64d' failed: '%s'\n", buffer);

    format = "% 8.5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer,"  -00100") && r==8,"' 8.5I64d' failed: '%s'\n", buffer);

    format = "%.0I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)0);
    OK(r==0,".0I64d failed: '%s'\n", buffer);

    format = "%#+21.18I64x";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer," 0x00ffffffffffffff9c") && r==21,"#+21.18I64x failed: '%s'\n", buffer);

    format = "%#.25I64o";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer,"0001777777777777777777634") && r==25,"#.25I64o failed: '%s'\n", buffer);

    format = "%#+24.20I64o";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer," 01777777777777777777634") && r==24,"#+24.20I64o failed: '%s'\n", buffer);

    format = "%#+18.21I64X";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer,"0X00000FFFFFFFFFFFFFF9C") && r==23,"#+18.21I64X failed: '%s '\n", buffer);

    format = "%#+20.24I64o";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-100);
    OK( 0==strcmp(buffer,"001777777777777777777634") && r==24,"#+20.24I64o failed: '%s'\n", buffer);

    format = "%#+25.22I64u";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-1);
    OK( 0==strcmp(buffer,"   0018446744073709551615") && r==25,"#+25.22I64u conversion failed: '%s'\n", buffer);

    format = "%#+25.22I64u";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-1);
    OK( 0==strcmp(buffer,"   0018446744073709551615") && r==25,"#+25.22I64u failed: '%s'\n", buffer);

    format = "%#+30.25I64u";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-1);
    OK( 0==strcmp(buffer,"     0000018446744073709551615") && r==30,"#+30.25I64u failed: '%s'\n", buffer);

    format = "%+#25.22I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-1);
    OK( 0==strcmp(buffer,"  -0000000000000000000001") && r==25,"+#25.22I64d failed: '%s'\n", buffer);

    format = "%#-8.5I64o";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"00144   ") && r==8,"-8.5I64o failed: '%s'\n", buffer);

    format = "%#-+ 08.5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"+00100  ") && r==8,"'#-+ 08.5I64d failed: '%s'\n", buffer);

    format = "%#-+ 08.5I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)100);
    OK( 0==strcmp(buffer,"+00100  ") && r==8,"#-+ 08.5I64d failed: '%s'\n", buffer);

    format = "%.80I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)1);
    OK(r==80,"%s format failed\n", format);

    format = "% .80I64d";
    r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)1);
    OK(r==81,"%s format failed\n", format);

    format = "% .80d";
    r = NSFormat::format(buffer,sizeof(buffer),format,1);
    OK(r==81,"%s format failed\n", format);

    format = "%lld";
    r = NSFormat::format(buffer,sizeof(buffer),format,((ULONGLONG)0xffffffff)*0xffffffff);
    OK( r == 1 || r == 11, "return count wrong %d\n", r);
    if (r == 11)  /* %ll works on Vista */
    {
        OK( 0==strcmp(buffer, "-8589934591"), "Problem with \"ll\" interpretation '%s'\n", buffer);
    }
    else
    {
        OK( 0==strcmp(buffer, "1"), "Problem with \"ll\" interpretation '%s'\n", buffer);
    }

    format = "%I";
    r = NSFormat::format(buffer,sizeof(buffer),format,1);
    OK( 0==strcmp(buffer, "I"), "Problem with \"I\" interpretation\n");
    OK( r==1, "return count wrong\n");

    format = "%I0d";
    r = NSFormat::format(buffer,sizeof(buffer),format,1);
    OK( 0==strcmp(buffer,"I0d"),"I0d failed\n");
    OK( r==3, "return count wrong\n");

    format = "%I32d";
    r = NSFormat::format(buffer,sizeof(buffer),format,1);
    if (r == 1) {
        OK( 0==strcmp(buffer,"1"),"I32d failed, got '%s'\n",buffer);
    } else {
        /* Older versions don't grOK I32 format */
        OK(r == 4 &&  0==strcmp(buffer,"I32d"),"I32d failed, got '%s',%d\n",buffer,r);
    }

//  format = "%I64D";
//  r = NSFormat::format(buffer,sizeof(buffer),format,(LONGLONG)-1);
//  OK( 0==strcmp(buffer,"D"),"I64D failed: %s\n",buffer);
//  OK( r==1, "return count wrong\n");

    format = "% d";
    r = NSFormat::format(buffer,sizeof(buffer),format,1);
    OK( 0==strcmp(buffer, " 1"),"Problem with sign place-holder: '%s'\n",buffer);
    OK( r==2, "return count wrong\n");

    format = "%+ d";
    r = NSFormat::format(buffer,sizeof(buffer),format,1);
    OK( 0==strcmp(buffer, "+1"),"Problem with sign flags: '%s'\n",buffer);
    OK( r==2, "return count wrong\n");

//  format = "%S";
//  r = NSFormat::format(buffer,sizeof(buffer),format,wide);
//  OK( 0==strcmp(buffer,"wide"),"Problem with wide string format\n");
//  OK( r==4, "return count wrong\n");

    format = "%04c";
    r = NSFormat::format(buffer,sizeof(buffer),format,'1');
    OK( 0==strcmp(buffer,"0001"),"Character not zero-prefixed \"%s\"\n",buffer);
    OK( r==4, "return count wrong\n");

    format = "%-04c";
    r = NSFormat::format(buffer,sizeof(buffer),format,'1');
    OK( 0==strcmp(buffer,"1   "),"Character zero-padded and/or not left-adjusted \"%s\"\n",buffer);
    OK( r==4, "return count wrong\n");

    format = "%#012x";
    r = NSFormat::format(buffer,sizeof(buffer),format,1);
    OK( 0==strcmp(buffer,"0x0000000001"),"Hexadecimal zero-padded \"%s\"\n",buffer);

    format = "%#04.8x";
    r = NSFormat::format(buffer,sizeof(buffer),format,1);
    OK( 0==strcmp(buffer,"0x00000001"), "Hexadecimal zero-padded precision \"%s\"\n",buffer);

    format = "%#-08.2x";
    r = NSFormat::format(buffer,sizeof(buffer),format,1);
    OK( 0==strcmp(buffer,"0x01    "), "Hexadecimal zero-padded not left-adjusted \"%s\"\n",buffer);

    format = "%#08o";
    r = NSFormat::format(buffer,sizeof(buffer),format,1);
    OK( 0==strcmp(buffer,"00000001"), "Octal zero-padded \"%s\"\n",buffer);

    if (sizeof(void *) == 8)
    {
        format = "%p";
        r = NSFormat::format(buffer,sizeof(buffer),format,(void *)57);
        OK( 0==strcmp(buffer,"0000000000000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
        OK( r==16, "return count wrong\n");

        format = "%#020p";
        r = NSFormat::format(buffer,sizeof(buffer),format,(void *)57);
        OK( 0==strcmp(buffer,"  0X0000000000000039"),"Pointer formatted incorrectly\n");
        OK( r==20, "return count wrong\n");

//      format = "%Fp";
//      r = NSFormat::format(buffer,sizeof(buffer),format,(void *)57);
//      OK( 0==strcmp(buffer,"0000000000000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
//      OK( r==16, "return count wrong\n");

        format = "%#-020p";
        r = NSFormat::format(buffer,sizeof(buffer),format,(void *)57);
        OK( 0==strcmp(buffer,"0X0000000000000039  "),"Pointer formatted incorrectly\n");
        OK( r==20, "return count wrong\n");
    }
    else
    {
        format = "%p";
        r = NSFormat::format(buffer,sizeof(buffer),format,(void *)57);
        OK( 0==strcmp(buffer,"00000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
        OK( r==8, "return count wrong\n");

        format = "%#012p";
        r = NSFormat::format(buffer,sizeof(buffer),format,(void *)57);
        OK( 0==strcmp(buffer,"  0X00000039"),"Pointer formatted incorrectly\n");
        OK( r==12, "return count wrong\n");

        format = "%Fp";
        r = NSFormat::format(buffer,sizeof(buffer),format,(void *)57);
        OK( 0==strcmp(buffer,"00000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
        OK( r==8, "return count wrong\n");

        format = "%#-012p";
        r = NSFormat::format(buffer,sizeof(buffer),format,(void *)57);
        OK( 0==strcmp(buffer,"0X00000039  "),"Pointer formatted incorrectly\n");
        OK( r==12, "return count wrong\n");
    }

    format = "%.1s";
    r = NSFormat::format(buffer,sizeof(buffer),format,"foo");
    OK( 0==strcmp(buffer,"f"),"Precision ignored \"%s\"\n",buffer);
    OK( r==1, "return count wrong\n");

    format = "%.*s";
    r = NSFormat::format(buffer,sizeof(buffer),format,1,"foo");
    OK( 0==strcmp(buffer,"f"),"Precision ignored \"%s\"\n",buffer);
    OK( r==1, "return count wrong\n");

    format = "%*s";
    r = NSFormat::format(buffer,sizeof(buffer),format,-5,"foo");
    OK( 0==strcmp(buffer,"foo  "),"Negative field width ignored \"%s\"\n",buffer);
    OK( r==5, "return count wrong\n");

    format = "hello";
    r = NSFormat::format(buffer,sizeof(buffer), format);
    OK( 0==strcmp(buffer,"hello"), "failed\n");
    OK( r==5, "return count wrong\n");

#if (0)
    format = "%ws";
    r = NSFormat::format(buffer,sizeof(buffer), format, wide);
    OK( 0==strcmp(buffer,"wide"), "failed\n");
    OK( r==4, "return count wrong\n");

    format = "%-10ws";
    r = NSFormat::format(buffer,sizeof(buffer), format, wide );
    OK( 0==strcmp(buffer,"wide      "), "failed\n");
    OK( r==10, "return count wrong\n");

    format = "%10ws";
    r = NSFormat::format(buffer,sizeof(buffer), format, wide );
    OK( 0==strcmp(buffer,"      wide"), "failed\n");
    OK( r==10, "return count wrong\n");

    format = "%#+ -03whlls";
    r = NSFormat::format(buffer,sizeof(buffer), format, wide );
    OK( 0==strcmp(buffer,"wide"), "failed\n");
    OK( r==4, "return count wrong\n");

    format = "%w0s";
    r = NSFormat::format(buffer,sizeof(buffer), format, wide );
    OK( 0==strcmp(buffer,"0s"), "failed\n");
    OK( r==2, "return count wrong\n");

    format = "%w-s";
    r = NSFormat::format(buffer,sizeof(buffer), format, wide );
    OK( 0==strcmp(buffer,"-s"), "failed\n");
    OK( r==2, "return count wrong\n");

    format = "%ls";
    r = NSFormat::format(buffer,sizeof(buffer), format, wide );
    OK( 0==strcmp(buffer,"wide"), "failed\n");
    OK( r==4, "return count wrong\n");
#endif

    format = "%Ls";
    r = NSFormat::format(buffer,sizeof(buffer), format, "not wide" );
    OK( 0==strcmp(buffer,"not wide"), "failed\n");
    OK( r==8, "return count wrong\n");

    format = "%b";
    r = NSFormat::format(buffer,sizeof(buffer), format);
    OK( 0==strcmp(buffer,"b"), "failed\n");
    OK( r==1, "return count wrong\n");

    format = "%3c";
    r = NSFormat::format(buffer,sizeof(buffer), format,'a');
    OK( 0==strcmp(buffer,"  a"), "failed\n");
    OK( r==3, "return count wrong\n");

    format = "%3d";
    r = NSFormat::format(buffer,sizeof(buffer), format,1234);
    OK( 0==strcmp(buffer,"1234"), "failed\n");
    OK( r==4, "return count wrong\n");

    format = "%j%k%m%q%r%t%v%y%z";
    r = NSFormat::format(buffer,sizeof(buffer), format);
    OK( 0==strcmp(buffer,"jkmqrtvyz"), "failed\n");
    OK( r==9, "return count wrong\n");

    format = "asdf%n";
    x = 0;
    r = NSFormat::format(buffer,sizeof(buffer), format, &x );
    if (r == -1) {
        /* %n format is disabled by default on many systems */
        OK(x == 0, "should not write to x: %d\n", x);
    } else {
        OK(x == 4, "should write to x: %d\n", x);
        OK( 0==strcmp(buffer,"asdf"), "failed\n");
        OK( r==4, "return count wrong: %d\n", r);
    }

    format = "%-1d";
    r = NSFormat::format(buffer,sizeof(buffer), format,2);
    OK( 0==strcmp(buffer,"2"), "failed\n");
    OK( r==1, "return count wrong\n");

    format = "%2.4f";
    r = NSFormat::format(buffer,sizeof(buffer), format,8.6);
    OK( 0==strcmp(buffer,"8.6000"), "failed\n");
    OK( r==6, "return count wrong\n");

    format = "%0f";
    r = NSFormat::format(buffer,sizeof(buffer), format,0.6);
    OK( 0==strcmp(buffer,"0.600000"), "failed\n");
    OK( r==8, "return count wrong\n");

    format = "%.0f";
    r = NSFormat::format(buffer,sizeof(buffer), format,0.6);
    OK( 0==strcmp(buffer,"1"), "failed\n");
    OK( r==1, "return count wrong\n");

    format = "%2.4e";
    r = NSFormat::format(buffer,sizeof(buffer), format,8.6);
    if (r==10) {
        OK( 0==strcmp(buffer,"8.6000e+00"), "failed\n");
    } else {
        OK( r==11, "return count wrong\n");
        OK( 0==strcmp(buffer,"8.6000e+000"), "failed\n");
    }

    format = "% 2.4e";
    r = NSFormat::format(buffer,sizeof(buffer), format,8.6);
    if (r==11) {
        OK( 0==strcmp(buffer," 8.6000e+00"), "failed: %s\n", buffer);
    } else {
        OK( r==12, "return count wrong\n");
        OK( 0==strcmp(buffer," 8.6000e+000"), "failed: %s\n", buffer);
    }

    format = "% 014.4e";
    r = NSFormat::format(buffer,sizeof(buffer), format,8.6);
    OK( r==14, "return count wrong\n");
    OK( 0==strcmp(buffer," 0008.6000e+00")||0==strcmp(buffer," 008.6000e+000"), "failed: %s\n", buffer);

    format = "% 2.4e";
    r = NSFormat::format(buffer,sizeof(buffer), format,-8.6);
    if (r==11) {
        OK( 0==strcmp(buffer,"-8.6000e+00"), "failed: %s\n", buffer);
    } else {
        OK( r==12, "return count wrong\n");
        OK( 0==strcmp(buffer,"-8.6000e+000"), "failed: %s\n", buffer);
    }

    format = "%+2.4e";
    r = NSFormat::format(buffer,sizeof(buffer), format,8.6);
    if (r==11) {
        OK( 0==strcmp(buffer,"+8.6000e+00"), "failed: %s\n", buffer);
    } else {
        OK( r==12, "return count wrong\n");
        OK( 0==strcmp(buffer,"+8.6000e+000"), "failed: %s\n", buffer);
    }

    format = "%2.4g";
    r = NSFormat::format(buffer,sizeof(buffer), format,8.6);
    OK( 0==strcmp(buffer,"8.6"), "failed\n");
    OK( r==3, "return count wrong\n");

    format = "%-i";
    r = NSFormat::format(buffer,sizeof(buffer), format,-1);
    OK( 0==strcmp(buffer,"-1"), "failed\n");
    OK( r==2, "return count wrong\n");

    format = "%-i";
    r = NSFormat::format(buffer,sizeof(buffer), format,1);
    OK( 0==strcmp(buffer,"1"), "failed\n");
    OK( r==1, "return count wrong\n");

    format = "%+i";
    r = NSFormat::format(buffer,sizeof(buffer), format,1);
    OK( 0==strcmp(buffer,"+1"), "failed\n");
    OK( r==2, "return count wrong\n");

    format = "%o";
    r = NSFormat::format(buffer,sizeof(buffer), format,10);
    OK( 0==strcmp(buffer,"12"), "failed\n");
    OK( r==2, "return count wrong\n");

    format = "%p";
    r = NSFormat::format(buffer,sizeof(buffer), format,0);
    if (sizeof(void *) == 8)  {
        OK( 0==strcmp(buffer,"0000000000000000"), "failed\n");
        OK( r==16, "return count wrong\n");
    } else {
        OK( 0==strcmp(buffer,"00000000"), "failed\n");
        OK( r==8, "return count wrong\n");
    }

    format = "%s";
    r = NSFormat::format(buffer,sizeof(buffer), format,0);
    OK( 0==strcmp(buffer,"(null)"), "failed\n");
    OK( r==6, "return count wrong\n");

    format = "%s";
    r = NSFormat::format(buffer,sizeof(buffer), format,"%%%%");
    OK( 0==strcmp(buffer,"%%%%"), "failed\n");
    OK( r==4, "return count wrong\n");

    format = "%u";
    r = NSFormat::format(buffer,sizeof(buffer), format,-1);
    OK( 0==strcmp(buffer,"4294967295"), "failed\n");
    OK( r==10, "return count wrong\n");

    format = "%H";
    r = NSFormat::format(buffer,sizeof(buffer), format,-1);
    OK( 0==strcmp(buffer,"H"), "failed\n");
    OK( r==1, "return count wrong\n");

    format = "x%cx";
    r = NSFormat::format(buffer,sizeof(buffer), format, 0x100+'X');
    OK( 0==strcmp(buffer,"xXx"), "failed\n");
    OK( r==3, "return count wrong\n");

    format = "%%0";
    r = NSFormat::format(buffer,sizeof(buffer), format);
    OK( 0==strcmp(buffer,"%0"), "failed: \"%s\"\n", buffer);
    OK( r==2, "return count wrong\n");

    format = "%hx";
    r = NSFormat::format(buffer,sizeof(buffer), format, 0x12345);
    OK( 0==strcmp(buffer,"2345"), "failed \"%s\"\n", buffer);

    format = "%hhx";
    r = NSFormat::format(buffer,sizeof(buffer), format, 0x123);
    OK( 0==strcmp(buffer,"23"), "failed: \"%s\"\n", buffer);
    r = NSFormat::format(buffer,sizeof(buffer), format, 0x12345);
    OK( 0==strcmp(buffer,"45"), "failed \"%s\"\n", buffer);
}


static void
test_3(void)
{
//  char buffer[100];
//  const char *format;

    //  If a conversion specification is invalid, the behavior is undefined.
    //

//  format = "%w";
//  r = NSFormat::format(buffer,sizeof(buffer), format,-1);
//  OK( 0==strcmp(buffer,""), "failed\n");
//  OK( r==0, "return count wrong\n");

//  format = "%h";
//  r = NSFormat::format(buffer,sizeof(buffer), format,-1);
//  OK( 0==strcmp(buffer,""), "failed\n");
//  OK( r==0, "return count wrong\n");

//  format = "%z";
//  r = NSFormat::format(buffer,sizeof(buffer), format,-1);
//  OK( 0==strcmp(buffer,"z"), "failed\n");
//  OK( r==1, "return count wrong\n");

//  format = "%j";
//  r = NSFormat::format(buffer,sizeof(buffer), format,-1);
//  OK( 0==strcmp(buffer,"j"), "failed\n");
//  OK( r==1, "return count wrong\n");

//  format = "%F";
//  r = NSFormat::format(buffer,sizeof(buffer), format,-1);
//  OK( 0==strcmp(buffer,""), "failed\n");
//  OK( r==0, "return count wrong\n");
}


static void
test_4(void)
{
//TODO
//  TEST("1 2 is now available--you have 3?", -1, "%@ %@ is now available--you have %@?", NSObject(1), NSObject(2), NSObject(3));

//  NSStringTest("%1$@ %2$@ has been downloaded", 1, 2);

//  NSStringTest("%2$@ %1$@ has been downloaded", 2, 1);
}


void
NSFormatTests(void)
{
    test_1();
    test_2();
    test_3();
    test_4();
}
