// $Id: signtoolshim.cpp,v 1.5 2025/04/22 17:24:04 cvsuser Exp $
//
//  AutoUpdater: Manifest generation tool.
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

#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string>
#include <iostream>

#include <time.h>
#include <io.h>

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include "signtoolshim.h"
#include "signmanifest.h"

#include "../libautoupdater.h"
#include "../util/upgetopt.h"
#include "../util/Util.h"

#pragma comment(lib, "Version.lib")

static const char *progname = "";

static void Usage(const struct SignToolArgs &args);
static const char *ExeVersion(const char *executable, char *version, size_t versize);


//  Function: SignToolShim
//      Signtool application shim.
//
int
SignToolShim(int argc, char *argv[], const struct SignToolArgs *args)
{
    const char *options = (args->hosturlalt ? "H:AK:x:V:E:h" : "H:K:x:V:E:h");
    const char *private_pem = NULL;
    const char *version = args->version,
        *hosturl = args->hosturl;
    const char *exename = NULL;
    unsigned key_version = 1;
    int ch;

    // arguments
    progname = (args->progname ? args->progname : Updater::Util::Basename(argv[0]));
    while (-1 != (ch = Updater::Getopt(argc, argv, options))) {
        switch (ch) {
        case 'H':   // host URL template
            hosturl = Updater::optarg;
            break;
        case 'A':   // alternative host URL
            hosturl = args->hosturlalt;
            break;
        case 'K':   // private key
            private_pem = Updater::optarg;
            break;
        case 'x':   // key version; default 1
            key_version = static_cast<unsigned>(strtoul(Updater::optarg, NULL, 0));
            break;
        case 'V':   // application version
            version = Updater::optarg;
            break;
        case 'E':   // executable name
            exename = Updater::optarg;
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
            progname << ": expected arguments <input> [<output>]" << std::endl;
        Usage(*args);

    } else if (argc > 2) {
        std::cerr << "\n" <<
            progname << ": unexpected arguments '" << argv[2] << "' ..." << std::endl;
        Usage(*args);
    }

    if (version && exename) {
        std::cerr << "\n" <<
            progname << ": -V and -E are mutually exclusive options." << std::endl;
        Usage(*args);
    }

    if (!hosturl || !*hosturl) {
        std::cerr << "\n" <<
            progname << ": -H <host-url> expected." << std::endl;
        Usage(*args);
    }

    // application inputs
    char exeversion[64] = {0};
    const char *inputname = argv[0], *outputname = argv[1];
    size_t inputlen;

    if ((inputlen = strlen(inputname)) < 5 ||
            (0 != _stricmp(inputname + (inputlen - 4), ".exe") &&
             0 != _stricmp(inputname + (inputlen - 4), ".msi"))) {
        std::cerr << "\n" <<
            progname << ": <input> should reference an installer image." << std::endl;
        Usage(*args);
    }

    if (outputname && 0 == strcmp(inputname, outputname)) { // optional
        std::cerr << "\n" <<
            progname << ": <input> and <output> names must be different." << std::endl;
        Usage(*args);
    }

    // derive version
    if (exename) { // optional executable name.
        if (NULL != ExeVersion(exename, exeversion, sizeof(exeversion))) {
            version = exeversion;
        }
    }
    if (!version || !*version) { // otherwise query installer.
        if (NULL != ExeVersion(inputname, exeversion, sizeof(exeversion))) {
            version = exeversion;
        }
    }
    if (!version || !*version) {
        std::cerr << "\n" <<
            progname << ": -V <version> required, no exe version information available." << std::endl;
        Usage(*args);
    }

    if (private_pem && 0 == key_version) {
        std::cerr << "\n" <<
            progname << ": -x <version> required, private key without version." << std::endl;
        Usage(*args);
    }

    // generate manifest
    if (NULL != private_pem) {
        if (0 != _access(private_pem, 0)) {
            std::cout << "Private key <" << private_pem << "> not found.\n";
            return EXIT_FAILURE;
        }

        struct SignKeyPair keypair = {0};

        if (0 == ed25519_load_pem(private_pem, NULL, &keypair)) {
            SignManifestEd(inputname, version, hosturl, &keypair, key_version);
        } else {
            std::cerr << "\n" <<
                progname << ": error reading key files." << std::endl;
            return 1;
        }

    } else {
        SignManifest(inputname, version, hosturl);
    }
    return 0;
}


//  Function: Usage
//      Command line usage and exit.
//
static void
Usage(const struct SignToolArgs &args)
{
    const char *progtitle =
        (args.progtitle && *args.progtitle ? args.progtitle : "AutoUpdater manifest generator");

    std::cout.flush();
    std::cerr <<
        "\n"\
        << progtitle << ".\n"\
        "Engine Version (" << autoupdate_version_string() << ")\n"\
        "\n"\
        "   " << progname << " [options] <input> [<output>]\n"\
        "\n"\
        "Options:\n"\
        "   -V <version>            Version label, form <x.x[.x[.x.]]>.\n"\
        "   or -E <installer>       otherwise installer path.\n"\
        "\n";

    std::cerr <<
        "   -H <host-url>           Explicit source URL"\
            ", default <" << (args.hosturl ? args.hosturl : "none") << ">.\n";

    if (args.hosturlalt)
        std::cerr <<
        "   -A                      alternative source URL <" << args.hosturlalt << ">.\n";

    std::cerr <<
        "   -K <private-key>        Private key image, generates a Ed25519 signature.\n"\
        "   -x <version>            KeyVersion, default <1>.\n"\
        "\n"\
        "Arguments:\n"\
        "   input                   Name of the input file.\n"\
        "   output                  Optional name of the results output file, otherwise stdout.\n"\
        "\n" << std::endl;

    exit(3);
}


//  Function: ExeVersion
//      Retrieve the executable version information.
//
static const char *
ExeVersion(const char *executable, char *version, size_t versize)
{
    char sub_block[2] = { '\\', '\0' };
    char path[MAX_PATH] = {0};
    void *vi = NULL, *sb = NULL;
    DWORD visz, dummy;
    UINT sbsz;

    // determine the size of the version info
    if (! SearchPathA(NULL, executable, NULL, sizeof(path), path, NULL)) {
        std::cerr << "Cannot find <" << executable << ">\n";
        return NULL;
    }

    if (0 == (visz = GetFileVersionInfoSizeA(path, &dummy))) {
        switch (GetLastError()) {
        case ERROR_RESOURCE_TYPE_NOT_FOUND:
            std::cerr << "<" << path << "> does not contain version info; this is probably an executable\n";
            break;
        default:
            std::cerr << "GetFileVersionInfoSize() failed : " << GetLastError() << "\n";
            break;
        }
        return NULL;
    }

    if (NULL == (vi = (void *)GlobalAlloc(GMEM_FIXED, visz))) {
        std::cerr << "Out of memory\n";
        return NULL;
    }

    // retrieve the version info
    if (! GetFileVersionInfoA(path, 0, visz, vi)) {
        std::cerr << "GetFileVersionInfo() failed : " << GetLastError() << "\n";
        return NULL;
    }

    // extract the VS_FIXEDFILEINFO from the version info
    if (! VerQueryValueA(vi, sub_block, &sb, &sbsz)) {
        std::cerr << "VerQueryValue() failed : " << GetLastError() << "\n";
        return NULL;
    }

    const VS_FIXEDFILEINFO *ffi = (const VS_FIXEDFILEINFO *)sb;
    if (ffi->dwProductVersionMS && ffi->dwProductVersionLS) {
        _snprintf(version, versize, "%u.%u.%u.%u",
            (HIWORD(ffi->dwProductVersionMS) & 0xFF), (LOWORD(ffi->dwProductVersionMS) & 0xFF),
            (HIWORD(ffi->dwProductVersionLS) & 0xFF), (LOWORD(ffi->dwProductVersionLS) & 0xFF));
    } else {
        version = NULL;
    }

    GlobalFree(vi);
    return version;
}

//end
