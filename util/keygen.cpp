// $Id: keygen.cpp,v 1.3 2025/04/21 13:58:28 cvsuser Exp $
//
//  KeyGen - Ed255519 key generator
//

#include <string>
#include <iostream>
#include <io.h>

#include "../libautoupdater.h"
#include "Base64.h"
#include "Hex.h"
#include "upgetopt.h"

static const char *progname = "keygen";

static void Usage();

int 
main(int argc, char* argv[])
{
    const char *private_pem = "private.pem", *public_pem = "public.pem";
    int ch;

    progname = argv[0];
    while (-1 != (ch = Updater::Getopt(argc, argv, "K:k:h"))) {
        switch (ch) {
        case 'K':   // private
            private_pem = Updater::optarg;
            break;
        case 'k':   // public
            public_pem = Updater::optarg;
            break;
        case 'h':
        default:
            Usage();
            break;
        }
    }

    argv += Updater::optind;
    argc -= Updater::optind;
    if (argc > 0) {
        std::cerr << "\n" <<
            progname << ": unexpected arguments '" << argv[0] << "' ..." << std::endl;
        Usage();
    }

    struct SignKeyPair keypair;

    if (0 == _access(private_pem, 0)) {
        std::cout << "Loading <" << private_pem << ">\n";
        if (-1 == ed25519_load_pem(private_pem, NULL, &keypair)) {
            return EXIT_FAILURE;
        }

    } else {
        std::cout << "Generating <" << private_pem << "> and <" << public_pem << ">\n";
        if (-1 == ed25519_generate_pem(private_pem, public_pem, NULL) ||
                -1 == ed25519_load_pem(private_pem, public_pem, &keypair)) {
            return EXIT_FAILURE;
        }
    }

    std::cout <<
        "A key-pair has been generated and saved locally.\n"
        "Add the following public-key into your updater application.\n"
        "\n" <<
        "   \"" << Updater::Base64::encode_to_string(keypair.public_key, sizeof(keypair.public_key)) << "\"\n"
        "\n";

#if !defined(NDEBUG)
    // See: https://blog.mozilla.org/warner/2011/11/29/ed25519-keys/
    std::cout <<
        "Private:" << Updater::Hex::to_string(keypair.private_key, sizeof(keypair.private_key)) << "\n"
        " Public:" << Updater::Hex::to_string(keypair.public_key, sizeof(keypair.public_key)) << "\n";
#endif

    memset(&keypair, 0, sizeof(keypair));
    return EXIT_SUCCESS;
}


static void
Usage()
{
    std::cerr <<
        "\n"\
        "keygen -K <private.pem> -k <public.pen>"\
        "\n" << std::endl;
    exit(99);
}

//end