//  $Id: NSRes.cpp,v 1.2 2021/08/26 14:31:31 cvsuser Exp $
//
//  NSRes - Localization resource converter.
//

#include <string>
#include <iostream>
#include <fstream>

#include "../test/upgetopt.h"
#include "NSLocalizedCollectionImpl.h"

static const char *progname = "NSRes";

static void Usage();

int
main(int argc, char *argv[])
{
    const char *output = "";
    int ch;

    progname = argv[0];
    while (-1 != (ch = Updater::Getopt(argc, argv, "o:vh"))) {
        switch (ch) {
        case 'o': 
            output = Updater::optarg;
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
            progname << ": expected input" << std::endl;
        Usage();
    } else if (argc > 1) {
        std::cerr << "\n" <<
            progname << ": unexpected arguments '" << argv[1] << "' ..." << std::endl;
        Usage();
    }

    NSLocalizedCollectionImpl collection;
    const char *filename = argv[0];

    if (collection.load(NSLocalizedCollectionImpl::Filename(filename), filename)) {
        NSLocalizedCollectionImpl::Exporter exporter(output);
        if (exporter.is_open()) {
            collection.for_each(exporter);
            return 1;
        }
        std::cerr << "Unable to create <" << output << "> : " << errno;
    }
    return 0;
}


static void
Usage()
{
    std::cerr <<
        "\n"\
        "Localization resource converter                    version 1.01\n"\
        "\n"\
        "   NSRes [options] input\n"\
        "\n"\
        "Options:\n"\
        "   -o <output-file>        Output filename.\n"\
        "\n" << std::endl;

  /*    "   -r <output-rc>          Output resource filename.\n"*/
     exit(99);
}

#include "NSLocalizedCollectionImpl.cpp"
