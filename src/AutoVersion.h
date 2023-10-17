#ifndef AUTOVERSION_H_INCLUDED
#define AUTOVERSION_H_INCLUDED
//  $Id: AutoVersion.h,v 1.11 2023/10/17 12:33:58 cvsuser Exp $
//
//  AutoUpdater: Version comparison.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2023, Adam Young
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

namespace Updater {

//  Dedian stype version comparsion, examples:
//
//      o 2.10 comes after 2.9
//      o 2.11-beta comes after 2.11-alpha
//      o 2.11 comes after 2.11-beta
//      o 7 comes after 2003
//      o the revision that was checked in on 1/30/2009 and never given a version
//          number comes after the revision that was checked in on 8/1/2008 and never
//          given a version number.
//
//  Policy:
//      Every package has a version number recorded in its Version control file field,
//      described in Version, Section 5.6.12.
//
//      The package management system imposes an ordering on version numbers, so that
//      it can tell whether packages are being up- or downgraded and so that package
//      system front end applications can tell whether a package it finds available is
//      newer than the one installed on the system. The version number format has the
//      most significant parts (as far as comparison is concerned) at the beginning.
//
//      If an upstream package has problematic version numbers they should be converted
//      to a sane form for use in the Version field.
//
//  Source:
//      http://www.debian.org/doc/debian-policy/ch-controlfields.html
//
//  Version:
//
//      The version number of a package, the format is:
//
//              [epoch:]upstream_version[-debian_revision]
//
//      The three components here are:
//
//      o epoch
//
//          This is a single (generally small) unsigned integer. It may be omitted, in
//          which case zero is assumed. If it is omitted then the upstream_version may
//          not contain any colons.
//
//          It is provided to allow mistakes in the version numbers of older versions
//          of a package, and also a package's previous version numbering schemes, to
//          be left behind.
//
//      o upstream_version
//
//          This is the main part of the version number. It is usually the version
//          number of the original ("upstream") package from which the .deb file has
//          been made, if this is applicable. Usually this will be in the same format
//          as that specified by the upstream author(s); however, it may need to be
//          reformatted to fit into the package management system's format and
//          comparison scheme.
//
//          The comparison behavior of the package management system with respect to
//          the upstream_version is described below. The upstream_version portion of
//          the version number is mandatory.
//
//          The upstream_version may contain only alphanumerics[36] and the characters
//          . + - : ~ (full stop, plus, hyphen, colon, tilde) and should start with a
//          digit. If there is no debian_revision then hyphens are not allowed; if
//          there is no epoch then colons are not allowed.
//
//      o debian_revision
//
//          This part of the version number specifies the version of the Debian package
//          based on the upstream version. It may contain only alphanumerics and the
//          characters + . ~ (plus, full stop, tilde) and is compared in the same way
//          as the upstream_version is.
//
//          It is optional; if it isn't present then the upstream_version may not
//          contain a hyphen. This format represents the case where a piece of software
//          was written specifically to be a Debian package, where the Debian package
//          source must always be identical to the pristine source and therefore no
//          revision indication is required.
//
//          It is conventional to restart the debian_revision at 1 each time the
//          upstream_version is increased.
//
//          The package management system will break the version number apart at the
//          last hyphen in the string (if there is one) to determine the
//          upstream_version and debian_revision. The absence of a debian_revision is
//          equivalent to a debian_revision of 0.
//
//  Algorithm:
//
//      When comparing two version numbers, first the epoch of each are compared, then
//      the upstream_version if epoch is equal, and then debian_revision if
//      upstream_version is also equal. epoch is compared numerically. The
//      upstream_version and debian_revision parts are compared by the package
//      management system using the following algorithm:
//
//      The strings are compared from left to right.
//
//      First the initial part of each string consisting entirely of non-digit
//      characters is determined. These two parts (one of which may be empty) are
//      compared lexically. If a difference is found it is returned. The lexical
//      comparison is a comparison of ASCII values modified so that all the letters
//      sort earlier than all the non-letters and so that a tilde sorts before anything,
//      even the end of a part. For example, the following parts are in sorted order
//      from earliest to latest: ~~, ~~a, ~, the empty part, a.[37]
//
//      Then the initial part of the remainder of each string which consists entirely
//      of digit characters is determined. The numerical values of these two parts are
//      compared, and any difference found is returned as the result of the comparison.
//      For these purposes an empty string (which can only occur at the end of one or
//      both version strings being compared) counts as zero.
//
//      These two steps (comparing and removing initial non-digit strings and initial
//      digit strings) are repeated until a difference is found or both strings are
//      exhausted.
//
//      Note that the purpose of epochs is to allow us to leave behind mistakes in
//      version numbering, and to cope with situations where the version numbering
//      scheme changes. It is not intended to cope with version numbers containing
//      strings of letters which the package management system cannot interpret (such
//      as ALPHA or pre-), or with silly orderings.
//
class AutoVersion {
public:
    AutoVersion():  epoch_(0), upstream_(NULL), revision_(NULL)
        { }
    AutoVersion(const AutoVersion& other) {
        *this = other;
    }
    explicit AutoVersion(const char *version);
    ~AutoVersion();

    int             Epoch() const {
        return epoch_;
    }

    const char *    Upstream() const {
        return upstream_.c_str();
    }

    const char *    Revision() const {
        return revision_.c_str();
    }

    void            Epoch(int new_epoch) {
        epoch_ = new_epoch;
    }

    void            Upstream(const char *new_upstream) {
        upstream_ = new_upstream;
    }

    void            Revision(const char *new_revision) {
        revision_ = new_revision;
    }

    void            ClearRevision() {
        Revision("0");
    }

    int             cmp(const AutoVersion& other) const;
    AutoVersion& operator=(const AutoVersion& other);
    bool operator   <(const AutoVersion& other) const;
    bool operator   ==(const AutoVersion& other) const;

    static int      Compare(const char *v1, const char *v2);
    static int      Compare(const std::string &v1, const std::string &v2);

protected:
    int             epoch_;
    std::string     upstream_;
    std::string     revision_;

    static int      CompareComponent(const char *a, const char *b);
};


inline
AutoVersion::~AutoVersion()
{
}


inline bool
AutoVersion::operator==(const AutoVersion& other) const
{
    return Epoch() == other.Epoch()
            && 0 == strcmp(Upstream(), other.Upstream())
            && 0 == strcmp(Revision(), other.Revision());
}


inline AutoVersion&
AutoVersion::operator=(const AutoVersion& other)
{
    epoch_ = other.Epoch();
    upstream_ = other.Upstream();
    revision_ = other.Revision();
    return *this;
}

void operator<<(std::ostream& o, const AutoVersion& ver);

}   //namespace Updater

#endif  /*AUTOVERSION_H_INCLUDED*/
