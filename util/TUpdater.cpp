// $Id: TUpdater.cpp,v 1.9 2025/04/22 06:19:43 cvsuser Exp $
//
// AutoUpdater -- console test/example application.
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
#include "util.h"
#include "upgetopt.h"

static void                 Usage();

static const char *         progname;
static const char *         hosturldefault =
    "https://api.github.com/repos/adamyg/mcwin32~mcwin32.manifest";

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
    const char *version = "1.0.3", *hosturl = hosturldefault;
    const char *public_pem = NULL;
    unsigned key_version = 1;
    int mode = 2, interactive = 0;
    int ch;

    progname = Updater::Util::Basename(argv[0]);
    while (-1 != (ch = Updater::Getopt(argc, argv, "V:H:K:x:iL:vh"))) {
        switch (ch) {
        case 'V':   /* application version */
            version = Updater::optarg;
            break;
        case 'H':   /* host URL */
            hosturl = Updater::optarg;
            break;
        case 'K':   /* Public key */
            public_pem = Updater::optarg;
            break;
        case 'x':   /* key version; default 1 */
            key_version = static_cast<unsigned>(strtoul(Updater::optarg, NULL, 0));
            break;
        case 'i':   /* interactive */
            ++interactive;
            break;
        case 'L':   /* log path */
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
            progname << ": expected arguments <mode>" << std::endl;
        Usage();
    } else if (argc > 1) {
        std::cerr << "\n" <<
            progname << ": unexpected arguments '" << argv[1] << "' ..." << std::endl;
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
            progname << ": unknown mode '" << arg << "'" << std::endl;
        Usage();
    }

    autoupdate_set_console_mode(1);
    if (mode >= 1) {
        if (public_pem) {
            autoupdate_ed25519_pem(public_pem, key_version);
        }
        autoupdate_appversion_set(version);
        autoupdate_hosturl_set(hosturl);
    }

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
        "Console updater\n"\
        "Version (" << autoupdate_version_string() << ")\n" \
        "\n"\
        "   consoleupdater [options] mode\n"\
        "\n"\
        "Modes:\n"\
        "   auto -              Periodically check for updates.\n"\
        "   prompt -            Re-prompt user when periodic updates are disabled.\n"\
        "   force -             Unconditionally prompt, even when skipped.\n"\
        "   reinstall -         Prompt for install, even if up-to-date.\n"\
        "   enable -            Enable periodic checks.\n"\
        "   disable -           Disable automatic periodic checks.\n"\
        "   reset -             Reset the updater status.\n"\
        "\n"\
        "   config -            Configuration.\n"\
        "\n"\
        "Options:\n"\
        "   -V <version>        Version label.\n"\
        "   -H <host>           Host URL, default <" << hosturldefault << ">.\n"\
        "   -K <public-key>     Public key.\n"\
        "   -i                  Interactive ('auto' only).\n"\
        "   -v                  Verbose diagnostics.\n"\
        "\n" << std::endl;
    exit(99);
}

/*end*/
