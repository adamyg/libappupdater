//  $Id: AutoGitHub.cpp,v 1.1 2025/04/16 11:33:48 cvsuser Exp $
//
//  AutoUpdater: Update manifest.
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

#include "common.h"

#include "AutoGitHub.h"
#include "AutoDownLoad.h"
#include "AutoLogger.h"

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cassert>

#include "../cjson/CJSON.h"

namespace Updater {

GitHub::GitHub()
{
}


GitHub::~GitHub()
{
}

// Example:
//  https://api.github.com/repos/adamyg/mcwin32~mcwin32.manifest
//
bool
GitHub::IsEndpoint(const std::string &url) const
{
    return (url.find_first_of("https://api.github.com/repos/") == 0 && url.find("~") != std::string::npos);
}


// Example:
//  https://api.github.com/repos/adamyg/mcwin32/releases/latest
// 
// Returns:
//  4.8.33.232
//
bool
GitHub::GetLatestRelease(const std::string &url, Download& downloader, int flags, std::string &result)
{   
    if (! IsEndpoint(url)) {
        result = "Invalid GitHub endpoint";
        return false;
    }

    const size_t delimiter = url.find("~");
    std::string api_url = url.substr(0, delimiter),
            manifest = url.substr(delimiter + 1);
    if (manifest.empty()) {
        result = "GitHub endpoint missing manifest";
        return false;
    }

    StringDownloadSink latest;

    api_url += "/releases/latest";
    if (! downloader.get(api_url, latest, flags) || ! downloader.completion()) {
        result = "Unable to retrieve releases/latest";
        return false;
    }

    // Payload:
    // 
    //  {
    //      "url": "https://api.github.com/repos/adamyg/mcwin32/releases/211120389",
    //      "assets_url" : "https://api.github.com/repos/adamyg/mcwin32/releases/211120389/assets",
    //      "upload_url" : "https://uploads.github.com/repos/adamyg/mcwin32/releases/211120389/assets{?name,label}",
    //      "html_url" : "https://github.com/adamyg/mcwin32/releases/tag/4.8.33.232",
    //      "id" : 211120389,
    //      "author" : {
    //          :
    //      },
    //      "draft" : false,
    //      "prerelease" : false,
    //      "assets" : [ {
    //              "url": "https://api.github.com/repos/adamyg/mcwin32/releases/assets/246305363",
    //                  :   
    //              "name" : "mcwin32-build232-setup.exe",
    //              "size" : 4461253,
    //              "download_count" : 68,
    //              "created_at" : "2025-04-14T14:50:37Z",
    //              "updated_at" : "2025-04-14T14:50:41Z",
    //              "browser_download_url" : "https://github.com/adamyg/mcwin32/releases/download/4.8.33.232/mcwin32-build232-setup.exe"// 
    //          }
    //              : 
    //          ]
    //      
    const std::string &payload = latest.data();
    cJSON *json;

    if (NULL == (json = cJSON_ParseWithLength((const char *)payload.data(), payload.length()))) {
        const char *pos = cJSON_GetErrorPtr();

        result = "Usable to parse releases/latest";
        if (pos) {
            char offset[80];
            sprintf_s(offset, sizeof(offset), ", at offset:%u", (unsigned)(pos - payload.data()));
            result += offset;
        }
        return false;
    }

    const char *formatted = cJSON_Print(json);
    if (formatted) {
        const char *cursor = formatted;

        for (unsigned line = 1; cursor && *cursor; ++line) {
            const char *nl = strchr(cursor, '\n'), *end;

            if ((end = nl) != NULL) {
                ++nl; // consume new-line
            } else {
                end = cursor + strlen(cursor); // eof
            }

            const character_view view(cursor, end);
            std::ostream &os = LOG<LOG_TRACE>();
            os << line << "] ";
                view.push(os) << LOG_ENDL;

            cursor = nl;
        }
        free((void *)formatted);
    }

    if (cJSON_HasObjectItem(json, "assets")) {
        cJSON* assets;
        
        if (NULL != (assets = cJSON_GetObjectItem(json, "assets")) && cJSON_IsArray(assets)) {
            const size_t nassets = 
                static_cast<size_t>(cJSON_GetArraySize(assets));

            for (unsigned index = 0; index < nassets; ++index) {
                const cJSON *asset = cJSON_GetArrayItem(assets, index);
                const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(asset, "name")),
                    *browser_download_url = cJSON_GetStringValue(cJSON_GetObjectItem(asset, "browser_download_url"));

                if (name && 0 == _stricmp(manifest.data(), name)) {
                    if (browser_download_url) {
                        result = browser_download_url;
                        break;
                    }
                }
            }
        }
    }

    cJSON_Delete(json);
    return true;
}

}   // namespace Updater

//end
