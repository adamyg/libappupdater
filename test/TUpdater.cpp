// $Id: TUpdater.cpp,v 1.6 2025/02/21 19:03:24 cvsuser Exp $
//
// AutoUpdater -- console test application.
//
//  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2025 Adam Young
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
#include <iostream>

#include "../libautoupdater.h"
#include "upgetopt.h"

static void                 Usage();
static const char *         Basename(const char *name);

static const char *         x_progname;


//  Function: Main
//      Application entry.
//
//  Returns:
//      0  - No check performed.
//      1  - Up-to-date.
//      2  - Installed.
//      3  - Update available.
//      99 - Usage
//
int
main(int argc, char *argv[])
{
    const char *version = "1.0.2",
            *hosturl = "https://sourceforge.net/projects/grief/files/grief.manifest/download";
    int mode = 2, interactive = 0;
    int ch;

    x_progname = Basename(argv[0]);
    while (-1 != (ch = Updater::Getopt(argc, argv, "V:H:iL:vh"))) {
        switch (ch) {
        case 'V':   /* application version */
            version = Updater::optarg;
            break;
        case 'H':   /* host URL */
            hosturl = Updater::optarg;
            break;
        case 'i':   /* interactive */
            ++interactive;
            break;
        case 'L':   /* logpath */
            autoupdate_logger_path(Updater::optarg);
            break;
        case 'v':   /* verbose */
            autoupdate_logger_stdout(1);
            break;
        case 'h':
        default:
            Usage();
            break;
        }
    }

    argv += Updater::optind;
    if ((argc -= Updater::optind) < 1) {
        std::cerr << "\n" <<
            x_progname << ": expected arguments <mode>" << std::endl;
        Usage();
    } else if (argc > 1) {
        std::cerr << "\n" <<
            x_progname << ": unexpected arguments '" << argv[1] << "' ..." << std::endl;
        Usage();
    }

    const char *arg = argv[0];

    if (0 == _stricmp("disable", arg)) {
        mode = 0;
    } else if (0 == _stricmp("enable", arg)) {
        mode = 1;
    } else if (0 == _stricmp("auto", arg)) {
        mode = 2;
    } else if (0 == _stricmp("prompt", arg)) {
        interactive = 1;
        mode = 3;
    } else if (0 == _stricmp("force", arg)) {
        interactive = 1;
        mode = 4;
    } else if (0 == _stricmp("reinstall", arg)) {
        mode = 5;
    } else if (0 == _stricmp("reset", arg)) {
        mode = -1;
    } else if (0 == _stricmp("dump", arg)) {
        mode = -2;
    } else if (0 == _stricmp("config", arg)) {
        std::cout
            << "Version: " << version << "\n"
            << "Host:    " << hosturl << "\n";
        return 0;
    } else {
        std::cerr << "\n" <<
            x_progname << ": unknown mode '" << arg << "'" << std::endl;
        Usage();
    }

    if (mode >= 1) {
        autoupdate_appversion_set(version);
        autoupdate_hosturl_set(hosturl);
    }

    autoupdate_set_console_mode(1);
    return autoupdate_execute(mode, interactive);
}


//  Function: Usage
//      Echo the command line usage and exit.
//
//  Parameters:
//      none
//
//  Returns:
//      n/a
//
static void
Usage()
{
    std::cerr <<
        "\n"\
        "Console updater                                            version 1.02\n"\
        "\n"\
        "   consoleupdater [options] mode\n"\
        "\n"\
        "Modes:\n"\
        "   auto -              Periodically check for updates.\n"\
        "   prompt -            Re-prompt user when periodic updates are disabled.\n"\
        "   force -             Unconditionally prompt, even when skipped.\n"\
        "   reinstall -         Prompt for install, even if uptodate.\n"\
        "   enable -            Enable periodic checks.\n"\
        "   disable -           Disable automatic periodic checks.\n"\
        "   reset -             Reset the updater status.\n"\
        "\n"\
        "   config -            Configuration.\n"\
        "\n"\
        "Options:\n"\
        "   -V <version>        Version label.\n"\
        "   -H <host>           Host URL.\n"\
        "   -i                  Interactive ('auto' only).\n"\
        "   -v                  Verbose diagnostice.\n"\
        "\n" << std::endl;
    exit(99);
}


//  Function: Basename
//      Retrieve the file basename from the specified file path.
//
static const char *
Basename(const char *filename)
{
    const char *name;
    return (NULL != (name = strrchr(filename, '/')))
                || (NULL != (name = strrchr(filename, '\\'))) ? name + 1 : filename;
}

/*end*/
