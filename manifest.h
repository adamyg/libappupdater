//  $Id: manifest.h,v 1.5 2021/08/12 15:01:11 cvsuser Exp $
//
//  Application manifest
//

#if defined(_MSC_VER)
//  Stop providing crtassem.h symbols when compiling with Visual Studio 2010,
//  msvcr100.dll is not a platform assembly anymore.
//
#if (_MSC_VER >= 1500 && _MSC_VER < 1600)
#include <crtassem.h>                       // VC8 & 9
#endif
#endif  //_MSC_VER

#if defined(_M_IX86)
#define MANIFEST_PROCESSORARCHITECTURE      "x86"
#elif defined(_M_AMD64)
#define MANIFEST_PROCESSORARCHITECTURE      "amd64"
#elif defined(_M_IA64)
#define MANIFEST_PROCESSORARCHITECTURE      "ia64"
#else
#error Unknown processor architecture.
#endif

#if defined(_DEBUG)
#define __LIBRARIES_ASSEMBLY_NAME_DEBUG     "Debug"
#else
#define __LIBRARIES_ASSEMBLY_NAME_DEBUG     ""
#endif

#if defined(_CRT_ASSEMBLY_VERSION)
#pragma comment(linker, \
    "/manifestdependency:\"type='win32' "\
        "name='" __LIBRARIES_ASSEMBLY_NAME_PREFIX "." __LIBRARIES_ASSEMBLY_NAME_DEBUG "CRT' "\
        "version='" _CRT_ASSEMBLY_VERSION "' "\
        "processorArchitecture='" MANIFEST_PROCESSORARCHITECTURE "' "\
        "publicKeyToken='" _VC_ASSEMBLY_PUBLICKEYTOKEN "'\"")

#pragma comment(linker, \
    "/manifestdependency:\"type='win32' "\
        "name='" __LIBRARIES_ASSEMBLY_NAME_PREFIX "." __LIBRARIES_ASSEMBLY_NAME_DEBUG "MFC' "\
        "version='" _CRT_ASSEMBLY_VERSION "' "\
        "processorArchitecture='" MANIFEST_PROCESSORARCHITECTURE "' "\
        "publicKeyToken='" _VC_ASSEMBLY_PUBLICKEYTOKEN "'\"")
#endif

#pragma comment(linker, \
    "/manifestdependency:\"type='win32' "\
        "name='Microsoft.Windows.Common-Controls' "\
        "version='6.0.0.0' "\
        "processorArchitecture='" MANIFEST_PROCESSORARCHITECTURE "' "\
        "publicKeyToken='6595b64144ccf1df' "\
        "language='*'\"")

/*end*/
