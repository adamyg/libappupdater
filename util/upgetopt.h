#pragma once
#ifndef UPGETOPT_H_INCLUDED
#define UPGETOPT_H_INCLUDED
// $Id: upgetopt.h,v 1.4 2025/04/21 13:58:28 cvsuser Exp $
//
//  getopt() implementation
//

#include <wchar.h>

namespace Updater {

extern int          optind,                     /* index into parent argv vector */
                    optopt;                     /* character checked for validity */
extern const char  *optarg;                     /* argument associated with option */
extern const wchar_t *woptarg;                  /* argument associated with option */

extern int          Getopt(int nargc, char **nargv, const char *ostr);
extern int          Getopt(int nargc, wchar_t **nargv, const char *ostr);


} //namespace Updater

#endif  /*UPGETOPT_H_INCLUDED*/
