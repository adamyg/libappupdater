//  $Id: AutoVersion.cpp,v 1.9 2021/08/14 15:38:10 cvsuser Exp $
//
//  AutoVersion: Version comparison.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2022, Adam Young
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

#include "common.h"

#include <ctime>
#include <vector>

#include "AutoVersion.h"

namespace Updater {

int
AutoVersion::Compare(const char *v1, const char *v2)
{
    AutoVersion r1(v1), r2(v2);
    return r1.cmp(r2);
}


int
AutoVersion::Compare(const std::string &v1, const std::string &v2)
{
    AutoVersion r1(v1.c_str()), r2(v2.c_str());
    return r1.cmp(r2);
}


AutoVersion::AutoVersion(const char *version)
{
    const char *epoch_end = strchr(version, ':');

    epoch_ = (epoch_end ? atoi(version) : 0);

    const char *upstream_start = (epoch_end ? epoch_end + 1 : version);
    const char *upstream_end = strrchr(upstream_start, '-');

    size_t upstream_size;
    if (! upstream_end) {
        upstream_size = strlen(upstream_start);
    } else {
        upstream_size = upstream_end - upstream_start;
    }

    upstream_.assign(upstream_start, upstream_size);

    if (! upstream_end) {
        revision_ = "0";
    } else {
        revision_ = (upstream_end + 1);
    }
}


/**
 *  Compare two components of a Debian-style version.
 *
 *      Return -1, 0, or 1 if a is less than, equal to,
 *      or greater than b, respectively.
 */
int
AutoVersion::CompareComponent(const char *a, const char *b)
{
    while (*a && *b) {
        while (*a && *b && !isdigit(*a) && !isdigit(*b)) {
            if (*a != *b) {
                if (*a == '~') return -1;
                if (*b == '~') return 1;
                return *a < *b ? -1 : 1;
            }
            a++;
            b++;
        }

        if (*a && *b && (!isdigit(*a) || !isdigit(*b))) {
            if (*a == '~') return -1;
            if (*b == '~') return 1;
            return isdigit(*a) ? -1 : 1;
        }

        char *next_a, *next_b;
        const long num_a = strtol(a, &next_a, 10);
        const long num_b = strtol(b, &next_b, 10);
        if (num_a != num_b) {
            return num_a < num_b ? -1 : 1;
        }
        a = next_a;
        b = next_b;
    }

    if (!*a && !*b) {
        return 0;
    } else if (*a) {
        return *a == '~' ? -1 : 1;
    } else {
        return *b == '~' ? 1 : -1;
    }
}


int
AutoVersion::cmp(const AutoVersion& other) const
{
    if (Epoch() != other.Epoch()) {
        return (Epoch() < other.Epoch() ? -1 : 1);
    }
    int result = CompareComponent(Upstream(), other.Upstream());
    if (0 == result) {
        result = CompareComponent(Revision(), other.Revision());
    }
    return result;
}


bool
AutoVersion::operator<(const AutoVersion& other) const
{
    return (-1 == cmp(other));
}


void
operator<<(std::ostream& o, const AutoVersion& ver)
{
    if (ver.Epoch() != 0) {
        o << ver.Epoch() << ":";
    }
    o << ver.Upstream();
    if (0 != strcmp(ver.Revision(), "0")) {
        o << "-" << ver.Revision();
    }
}

}   // namespace Updater
