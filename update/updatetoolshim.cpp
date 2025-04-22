//  $Id: updatetoolshim.cpp,v 1.4 2025/04/22 08:18:36 cvsuser Exp $
//
//  Midnight Commander AutoUpdater command line.
//

#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdlib>
#include <string.h>
#include <iostream>

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include "updatetoolshim.h"

#include "../libautoupdater.h"

#include "../util/upgetopt.h"
#include "../util/util.h"

static void Usage(const struct UpdateToolArgs& args);

static const char *progname;

//  Function: UpdateToolShim
//      UpdateTool application shim.
//
//  Returns:
//      0  - No check performed.
//      1  - Up-to-date.
//      2  - Installed.
//      3  - Update available.
//      99 - Usage
//
int
UpdateToolShim(int argc, char* argv[], const struct UpdateToolArgs *args)
{
    const char *options = (args->hosturlalt ? "V:H:AK:x:L:icvh" : "V:H:K:x:L:icvh");
    const char *version = args->version,
            *hosturl = args->hosturl,
            *public_key = args->publickey;
    unsigned key_version = args->keyversion;
    int mode = 2, interactive = 0;
    int ch;

    // arguments
    progname = (args->appname ? args->appname : Updater::Util::Basename(argv[0]));
    while (-1 != (ch = Updater::Getopt(argc, argv, options))) {
        switch (ch) {
        case 'V':   // application version
            version = Updater::optarg;
            break;
        case 'H':   // host URL
            hosturl = Updater::optarg;
            break;
        case 'A':   // alternative source
            hosturl = args->hosturlalt;
            break;

        case 'K':   // private key
            public_key = Updater::optarg;
            break;
        case 'x':   // key version; default 1
            key_version = static_cast<unsigned>(strtoul(Updater::optarg, NULL, 0));
            break;

        case 'L':   // log-path
            autoupdate_logger_path(Updater::optarg);
            break;
        case 'i':   // interactive
            ++interactive;
            break;
        case 'c':   // console
            autoupdate_set_console_mode(1);
            break;
        case 'v':   // verbose
            autoupdate_logger_stdout(1);
            break;

        case 'h':
        default:
            Usage(*args);
            break;
        }
    }

    argv += Updater::optind;
    if ((argc -= Updater::optind) < 1) {
        std::cerr << "\n" <<
            progname << ": expected arguments <mode>" << std::endl;
        Usage(*args);
    } else if (argc > 1) {
        std::cerr << "\n" <<
            progname << ": unexpected arguments '" << argv[1] << "' ..." << std::endl;
        Usage(*args);
    }

    if (!version || !*version) {
        std::cerr << "\n" <<
            progname << ": -V <version> expected." << std::endl;
        Usage(*args);
    }

    if (!hosturl || !*hosturl) {
        std::cerr << "\n" <<
            progname << ": -H <host-url> expected." << std::endl;
        Usage(*args);
    }

    if (public_key && 0 == key_version) {
        std::cerr << "\n" <<
            progname << ": -x <version> required, public key without version." << std::endl;
        Usage(*args);
    }

    const char *arg = argv[0];

#if defined(__MINGW32__)
#define STRICMP(__a,__b) strcasecmp(__a,__b)
#else
#define STRICMP(__a,__b) _stricmp(__a,__b)
#endif

    if (0 == STRICMP("disable", arg)) {
        mode = 0;
    } else if (0 == STRICMP("enable", arg)) {
        mode = 1;
    } else if (0 == STRICMP("auto", arg)) {
        mode = 2;
    } else if (0 == STRICMP("prompt", arg)) {
        mode = 3;
    } else if (0 == STRICMP("force", arg)) {
        mode = 4;
    } else if (0 == STRICMP("reinstall", arg)) {
        mode = 5;
    } else if (0 == STRICMP("reset", arg)) {
        mode = -1;
    } else if (0 == STRICMP("dump", arg)) {
        mode = -2;
    } else if (0 == STRICMP("config", arg)) {
        if (args->apptitle)
          std::cout
            << args->apptitle << "\n";
        std::cout
            << "Built:   " << __DATE__ << "\n"
            << "Version: " << version << "\n"
            << "Host:    " << hosturl << "\n";
        if (public_key)
          std::cout
            << "Key:     ed25519\n";
        return 0;
    } else {
        std::cerr << "\n" <<
            progname << ": unknown mode '" << arg << "'" << std::endl;
        Usage(*args);
    }

    if (mode >= 1) {
        autoupdate_appversion_set(version);
        autoupdate_hosturl_set(hosturl);
        if (public_key) {
            autoupdate_ed25519_key(public_key, key_version); // public key
        }
        autoupdate_appname_set(args->productname ? args->productname : progname); 
            // note: name should align with installer
    }

    return autoupdate_execute(mode, interactive);
}


//  Function: Usage
//      Command line usage and exit.
//
static void
Usage(const struct UpdateToolArgs& args)
{
    const char *apptitle =
        (args.apptitle && *args.apptitle ? args.apptitle : "AutoUpdater application check");

    std::cout.flush();
    std::cerr <<
        "\n"\
        << apptitle << ".\n"\
        "Version (" << autoupdate_version_string() << ")\n" \
        "\n"\
        "   " << progname << " [options] mode\n"\
        "\n"\
        "Modes:\n"\
        "   auto -                  Periodically check for updates.\n"\
        "   prompt -                Re-prompt user when periodic updates are disabled.\n"\
        "   force -                 Prompt ignoring skip status.\n"\
        "   reinstall -             Prompt unconditionally, even if up-to-date/skipped.\n"\
        "\n"\
        "   enable -                Enable periodic checks.\n"\
        "   disable -               Disable automatic periodic checks.\n"\
        "   reset -                 Reset the updater status.\n"\
        "\n"\
        "   config -                Configuration.\n"\
        "\n"\
        "Options:\n"\
        "   -V <version>            Version label, form <x.x[.x[.x.]]>.\n";

    std::cerr <<
        "   -H <host-url>           Explicit source URL" \
        ", default <" << (args.hosturl ? args.hosturl : "none") << ">.\n";

    if (NULL != args.hosturlalt)
      std::cerr <<
        "   -A                      alternative source URL <" << args.hosturlalt << ">.\n";

    if (NULL == args.publickey || 0 == args.keyversion)
      std::cerr <<
        "   -K <private-key>        Private key image, generates a ed25519 signature.\n"\
        "   -x <version>            KeyVersion, default <1>.\n"\
        "\n";

    std::cerr <<
        "   -L <logpath>            Diagnostics log path.\n"\
        "   -i                      Interactive ('auto' only).\n"\
        "   -c                      Console mode.\n"\
        "   -v                      Verbose diagnostics.\n"\
        "\n" << std::endl;
    std::exit(99);
}

/*end*/
