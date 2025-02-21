#ifndef AUTOMANIFEST_H_INCLUDED
#define AUTOMANIFEST_H_INCLUDED
//  $Id: AutoManifest.h,v 1.12 2025/02/21 19:03:23 cvsuser Exp $
//
//  AutoUpdater: application manifest.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2025, Adam Young
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

#include <time.h>
#include <string>

namespace Updater {
class AutoManifest {
public:
    std::string     BuildLabel;                 // Optional build label, "release", "debug" etc.
    std::string     OSLabel;                    // Operating system target "windows[-x86|-x64]"

    std::string     title;
    std::string     link;                       // Site link.

    std::string     version;                    // Updater min version.
    std::string     minimumSystemVersion;       // Min target OS version.
    std::string     criticalUpdate;             // Critical update, less then version or "yes" by default.
    std::string     installerArguments;         // Optional installer options.

    time_t          published;                  // Published time-stamp.
    std::string     pubDate;                    // Human readable publish time-stamp.
    std::string     description;                // Release description.
    std::string     releaseNotesLink;           // Release notes.
    std::string     releaseNotesContent;        // Loaded content.

    std::string     attributeURL;               // Download URL.
    std::string     attributeName;              // Public name.
    std::string     attributeVersion;           // Version.
    std::string     attributeLength;            // Image length, in bytes
    std::string     attributeType;              // Image type.
    std::string     attributeSHASignature;      // SHA signature.
    std::string     attributeMD5Signature;      // MD5 signature.
    std::string     attritbuteEDSignature;      // ED signature (TODO).

    mutable unsigned weight;

    bool            Load(const std::string& xml, const std::string &channel, const std::string &os_label);
    bool            IsCriticalUpdate(const std::string &current_version) const;
};

}   // namespace Updater

#endif  //AUTOMANIFEST_H_INCLUDED
