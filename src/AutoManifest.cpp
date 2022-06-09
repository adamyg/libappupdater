//  $Id: AutoManifest.cpp,v 1.12 2021/10/26 13:15:47 cvsuser Exp $
//
//  AutoUpdater: Update manifest.
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

#include "AutoManifest.h"
#include "AutoVersion.h"
#include "AutoError.h"
#include "AutoLogger.h"

#include <cstdio>

#include <algorithm>
#include <vector>
#include <string>
#include <cassert>

#include <time.h>
#if defined(_MSC_VER) && (_MSC_VER < 1900)
#include "../compat/expat.h"
#else
#include "../expat/expat.h"
#endif

namespace Updater {
namespace {
//
//  Modified RSS style structure, based on Sparkle/WinSparkle
//
//      [ <rss> ] or [ <manifest> ]
//          [ <channel [name="xxx" /*extension*/]> ]
//              <item [os="xxx" /*extension*/]>
//                  <title> ... </title>
//
//                  <link> ... </link>
//
//                  <description [url=""]> ... </description>
//               or <updater:releaseNotesLink>xxx</updater:releaseNotesLink>
//
//                  <published> ... </published>
//               or <pubDate> ... </pubDate>
//
//                  <updater:version>1.0</updater:version>
//
//                  <updater:minimumSystemVersion>xxx</updater:minimumSystemVersion>
//
//          TODO:   <updater:phasedRolloutInterval>xxx</updater:phasedRolloutInterval>
//
//                  <updater:criticalUpdate [updater:version="1.2.4"]></updater:criticalUpdate>
//                                                  ^ optional less than comparision.
//
//                  <updater:installerArguments>xxx</installerArguments>
//
//                  <updater:tags>
//                  </updater:tags>
//
//                  <enclosure ...
//                      name=
//                      url=
//                      length=
//                      type="application/octet-stream"
//                      build=""
//                      os=""
//                      version=
//                      shaSignature=
//                      md5Signature=
//          TODO:       edSignature=
//                  />
//              </item>
//                   :  :
//
//          [ </channel> ]
//      [ </rss> or </manifest> ]
//
//      Note: "updater" or "sparkle"
//

#define DEFAULTOS               "windows"

#define ATOM_CHANNEL            "channel"
#define ATOM_ITEM               "item"
#define ATOM_TITLE              "title"
#define ATOM_DESCRIPTION        "description"
#define ATOM_LINK               "link"
#define ATOM_RELEASENOTESLINK   "releaseNotesLink"
#define ATOM_VERSION            "version"
#define ATOM_MINIMUMSYSTEMVERSION "minimumSystemVersion"
#define ATOM_CRITICALUPDATE     "criticalUpdate"
#define ATOM_PUBLISHED          "published"
#define ATOM_PUBDATE            "pubDate"
#define ATOM_INSTALLERARGUMENTS "installerArguments"
#define ATOM_TAGS               "tags"

#define ATOM_ENCLOSURE          "enclosure"
#define ATTR_NAME               "name"
#define ATTR_URL                "url"
#define ATTR_VERSION            "version"
#define ATTR_LENGTH             "length"
#define ATTR_BUILD              "build"
#define ATTR_TYPE               "type"
#define ATTR_OS                 "os"
#define ATTR_SHASIGNATURE       "shaSignature"
#define ATTR_MD5SIGNATURE       "md5Signature"
#define ATTR_EDSIGNATURE        "edSignature"

namespace {

// context data for the parser
struct ParserContext {
    enum ChannelStatus {CHANNEL_OMITTED = -1, CHANNEL_NONE = 0, CHANNEL_ACTIVE = 1, CHANNEL_INACTIVE = 2};

    ParserContext(XML_Parser parser, const char *required_channel)
        : parser(parser), required_channel(required_channel?required_channel:""),
            channel_status(CHANNEL_NONE), in_item(false), in_tags(false),
            title_level(0),
            link_level(0),
            description_level(0),
            releaseNotesLink_level(0),
            version_level(0),
            minimumSystemVersion_level(0),
            criticalUpdate_level(0),
            installerArguments_level(0),
            published_level(0),
            pubDate_level(0),
            manifest(NULL) {
    }

    static bool PrefixFieldMatch(const char *field, const char *required) {
        if (0 == strncmp(field, "updater:", 8))
            field += 8;
        else if (0 == strncmp(field, "sparkle:", 8))
            field += 8;
        return (0 == strcmp(field, required));
    }

    static bool EmptyOrRelease(const std::string &channel) {
        return (channel.empty() || channel == "release");
    }

    bool ChannelMatch(const std::string &channel) {
        if (channel == required_channel ||
                (EmptyOrRelease(channel) && EmptyOrRelease(required_channel))) {
            return true;
        }
        return false;
    }

    void ParserError(const char *msg) {
        error.assign(msg);
        XML_StopParser(parser, XML_FALSE);
    }

    void ParserError(const std::string &msg) {
        error.assign(msg);
        XML_StopParser(parser, XML_FALSE);
    }

    void ParserWarning(const char *msg) {
        warnings.push_back(msg);
    }

    void ParserWarning(const std::string &msg) {
        warnings.push_back(msg);
    }

    void ParserComplete() {
        XML_StopParser(parser, XML_TRUE);
    }

    unsigned LineNumber() const {
        return XML_GetCurrentLineNumber(parser);
    }

    static DWORD SelectionWeight(const AutoManifest &manifest);

    const AutoManifest *best_match() const {
        std::vector<AutoManifest>::const_iterator first(manifests.begin()),
            last(manifests.end());

        if (first != last) {
            std::vector<AutoManifest>::const_iterator largest = first;
            DWORD weight = SelectionWeight(*largest);

            for (++first; first != last; ++first) {
                DWORD first_weight = SelectionWeight(*first);
                if (weight < first_weight) {
                    weight  = first_weight;
                    largest = first;
                }
            }

            if (weight) {
                return &(*largest);
            }
        }
        return NULL;
    }

    XML_Parser      parser;                     // XML parser instance.
    const std::string required_channel;         // required channel; optional.
    std::string     error;                      // last error.
    std::vector<std::string> warnings;          // none or more warnings.
    enum ChannelStatus channel_status;          // <channel> status
    std::string     channel_name;               // and associated channel name; if any.
    bool            in_item;                    // <item>
    bool            in_tags;                    // <tags>
    int             title_level;                // <title>
    int             link_level;                 // <link>
    int             description_level;          // <description>
    int             releaseNotesLink_level;     // <updater:releaseNotesLink>
    int             version_level;              // <updater:version>
    int             minimumSystemVersion_level; // <updater:minimumSystemVersion>
    int             tags_level;                 // <updater:tags>
    int             criticalUpdate_level;       // <updater:criticalUpdate>
    int             installerArguments_level;   // <updater:installerArguments>
    int             published_level;            // <published>
    int             pubDate_level;              // <pubDate>

    std::vector<AutoManifest> manifests;
    AutoManifest*   manifest;
};

typedef DWORD (__stdcall *RtlGetVersion_t)(OSVERSIONINFOW *);

static BOOL RtlGetVersion(OSVERSIONINFOW &version) {
    static RtlGetVersion_t fnRtlGetVersion = NULL;

    if (NULL == fnRtlGetVersion) {              // one-shot
        HMODULE ntdll = ::GetModuleHandleA("ntdll.dll");
        if (ntdll) {
            fnRtlGetVersion = (RtlGetVersion_t) ::GetProcAddress(ntdll, "RtlGetVersion");
            if (NULL == fnRtlGetVersion) {
                fnRtlGetVersion = (RtlGetVersion_t)-1;
            }
        }
    }

    if ((RtlGetVersion_t)-1 != fnRtlGetVersion) {
        if (fnRtlGetVersion(&version) == 0 /*STATUS_SUCCESS*/) {
            return true;
        }
    }

    memset(&version, 0, sizeof(version));
    return false;
}

DWORD
ParserContext::SelectionWeight(const AutoManifest &manifest)
{
    DWORD weight = 0, version = 0;

    manifest.weight = 0;

    //  Operation system
    //
    if (manifest.OSLabel.empty() || manifest.OSLabel == "windows") {
        weight = 1;                             // generic match
#ifdef _WIN64
    } else if (manifest.OSLabel == "windows-x64") {
        weight = 2;                             // specific match
#else
    } else if (manifest.OSLabel == "windows-x86") {
        weight = 2;                             // specific match
#endif
    } else {
        return 0;                               // unknown
    }

    //  Version
    //
    if (! manifest.minimumSystemVersion.empty()) {
        //  In Windows 8.1 and Windows 10, the GetVersion and GetVersionEx functions have been deprecated.
        //  In Windows 10, the VerifyVersionInfo function has also been deprecated.
        //
        //  While you can still call the deprecated functions, if your application does not specifically
        //  target Windows 8.1 or Windows 10, the functions will return the Windows 8 version (6.2).
        //  This requires a "suitable" manifest to be embedded.
        //
        OSVERSIONINFOEXW vi = { sizeof(vi) };
        OSVERSIONINFOW cvi = { sizeof(cvi) };

        const int elements = std::sscanf(manifest.minimumSystemVersion.c_str(), "%lu.%lu.%hu",
                    &vi.dwMajorVersion, &vi.dwMinorVersion, &vi.wServicePackMajor);

#pragma warning(disable:4996)
        if (elements >= 1 && GetVersionExW(&cvi)) {
            if (cvi.dwMajorVersion > 6 ||       // vista+
                    (cvi.dwMajorVersion == 6 && cvi.dwMinorVersion >= 2)) {

                if (RtlGetVersion(cvi)) {
                    if ((cvi.dwMajorVersion <  vi.dwMajorVersion) ||
                        (cvi.dwMajorVersion == vi.dwMajorVersion &&
                                cvi.dwMinorVersion < vi.dwMinorVersion)) {
                        return 0;               // no match
                    }
                }

#if (0)
                if (3 == elements) {            // TODO -- dynamic load
                    DWORDLONG const dwlConditionMask = VerSetConditionMask(
                        VerSetConditionMask(
                            VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
                            VER_MINORVERSION, VER_GREATER_EQUAL),
                            VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

                    if (! VerifyVersionInfoW(&vi,
                            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) == FALSE) {
                        return 0;               // no match
                    }
                }
#endif

            } else {                            // XP+
                if ((cvi.dwMajorVersion <  vi.dwMajorVersion) ||
                    (cvi.dwMajorVersion == vi.dwMajorVersion &&
                            cvi.dwMinorVersion < vi.dwMinorVersion)) {
                    return 0;                   // no match
                }
            }
        }

        if (vi.dwMinorVersion > 0xffff) vi.dwMinorVersion = 0xffff;
        version =
            ((vi.dwMajorVersion & 0x0fff) << 16)
          |  (vi.dwMinorVersion & 0xffff);
    }

    manifest.weight = ((weight << 30) | version);
    return manifest.weight;
}

// attribute list to string
const std::string
AttrsToString(const char **attrs)
{
    std::string str;
    unsigned i = 0;

    while (attrs[i] && attrs[i+1]) {
        if (i) str += " ";
        str += attrs[i++];
        str += "=";
        str += attrs[i++];
    }
    return str;
}

// default white-space specialization
static const char* ws = " \t\n";

// trim from end of string (right)
inline std::string& rtrim(std::string& s, const char* t = ws)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = ws)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from both ends of string (right then left)
inline std::string& trim(std::string& s, const char* t = ws)
{
    return ltrim(rtrim(s, t), t);
}

};  //namespace anon


void XMLCALL
OnStartElement(void *data, const char *name, const char **attrs)
{
    ParserContext& ctx = *static_cast<ParserContext*>(data);

    // containers, allowing for two forms:
    //
    //      <channel [name="xxx"]>
    //          <item>
    //              ::
    //          </item>
    //      </channel>
    //
    //  or
    //      <item>
    //      </item>
    //          ::
    //
    if (ParserContext::CHANNEL_NONE == ctx.channel_status) {
        if (0 == strcmp(name, ATOM_CHANNEL)) {
            // <channel>
            if (ctx.channel_status) {
                ctx.ParserError("nested channel components");
                return;
            }

            std::string channel;
            for (int i = 0; attrs[i]; i += 2) {
                const char *var = attrs[i], *value = attrs[i+1];
                if (0 == strcmp(var, "name")) {
                    channel = value;
                    break;
                }
            }

            if (ctx.ChannelMatch(channel)) {
                LOG<LOG_TRACE>() << "manifest[" << ctx.LineNumber() << "]"
                            << "->channel<" << channel << ">" << LOG_ENDL;
                ctx.channel_status = ParserContext::CHANNEL_ACTIVE;
                ctx.channel_name = channel;
            } else {
                LOG<LOG_DEBUG>() << "manifest[" << ctx.LineNumber() << "],"
                            << " channel<" << AttrsToString(attrs) << "> ignored" << LOG_ENDL;
                ctx.channel_status = ParserContext::CHANNEL_INACTIVE;
                ctx.channel_name.clear();
            }
            return;

        } else if (0 == strcmp(name, ATOM_ITEM)) {
            // <item> only
            if (ctx.in_item) {
                ctx.ParserError("nested manifest components");
            } else {
                ctx.manifests.push_back(AutoManifest());
                ctx.manifest = &(ctx.manifests.back());
                ctx.channel_status = ParserContext::CHANNEL_OMITTED;
                ctx.in_item = true;
            }
            return;
        }

    } else {
        if (0 == strcmp(name, ATOM_CHANNEL)) {
            if (ParserContext::CHANNEL_ACTIVE == ctx.channel_status ||
                    ParserContext::CHANNEL_INACTIVE == ctx.channel_status) {
                ctx.ParserError("nested channel components");
            } else {
                ctx.ParserError("unexpected channel component");
            }
            return;

        } else if (0 == strcmp(name, ATOM_ITEM)) {
            if (ctx.in_item) {
                ctx.ParserError("nested manifest components");
            } else {
                if (ParserContext::CHANNEL_INACTIVE != ctx.channel_status) {
                    ctx.manifests.push_back(AutoManifest());
                    ctx.manifest = &(ctx.manifests.back());
                    ctx.manifest->BuildLabel.assign(ctx.channel_name);
                    ctx.in_item = true;
                }
            }
            return;
        }
    }

    // items and tags
    if (ctx.in_tags) {

    } else if (ctx.in_item) {
        if (0 == strcmp(name, ATOM_TITLE)) {    // <title> ...
            ++ctx.title_level;
                                                // <link> ...
        } else if (0 == strcmp(name, ATOM_LINK)) {
            ++ctx.link_level;
                                                // <description [url]>[text]
        } else if (0 == strcmp(name, ATOM_DESCRIPTION)) {
            if (0 == ctx.description_level && ctx.manifest) {
                for (unsigned i = 0; attrs[i] && attrs[i+i]; i += 2) {
                    const char *var = attrs[i], *value = attrs[i+1];
                    if (0 == strcmp(var, ATTR_URL)) {
                        ctx.manifest->releaseNotesLink = value;
                        break;
                    }
                }
            }
            ++ctx.description_level;
                                                // <updater:releaseNotesLink>
        } else if (ctx.PrefixFieldMatch(name, ATOM_RELEASENOTESLINK)) {
            ++ctx.releaseNotesLink_level;
                                                // <updater:version>
        } else if (ctx.PrefixFieldMatch(name, ATOM_VERSION)) {
            ++ctx.version_level;
                                                // <updater:minimumSystemVersion>
        } else if (ctx.PrefixFieldMatch(name, ATOM_MINIMUMSYSTEMVERSION)) {
            ++ctx.minimumSystemVersion_level;
                                                // <published>
        } else if (0 == strcmp(name, ATOM_PUBLISHED)) {
            ++ctx.published_level;
                                                // <pubDate>
        } else if (0 == strcmp(name, ATOM_PUBDATE)) {
            ++ctx.pubDate_level;
                                                // <criticalUpdate>
        } else if (ctx.PrefixFieldMatch(name, ATOM_CRITICALUPDATE)) {
            if (0 == ctx.criticalUpdate_level && ctx.manifest) {
                ctx.manifest->criticalUpdate = "*";
                for (unsigned i = 0; attrs[i] && attrs[i+i]; i += 2) {
                    const char *var = attrs[i], *value = attrs[i+1];
                    if (ctx.PrefixFieldMatch(var, ATOM_VERSION)) {
                        ctx.manifest->criticalUpdate = value;
                        break;
                    }
                }
            }
            ++ctx.criticalUpdate_level;
                                                // <installerArguments>
        } else if (ctx.PrefixFieldMatch(name, ATOM_INSTALLERARGUMENTS)) {
            ++ctx.installerArguments_level;
                                                // <updater:tags>
        } else if (ctx.PrefixFieldMatch(name, ATOM_TAGS)) {
            if (ctx.in_tags) {
                ctx.ParserError("nested tags component");
                return;
            }
            ctx.in_tags = true;
                                                // <enclosure [options]>
        } else if (0 == strcmp(name, ATOM_ENCLOSURE)) {
            if (AutoManifest *manifest = ctx.manifest) {
                for (unsigned i = 0; attrs[i]; i += 2) {
                    const char *var = attrs[i], *value = attrs[i+1];

                    LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                            << "->enclosure<" << var << "=" << value << ">" << LOG_ENDL;
                    if (0 == strcmp(var, ATTR_NAME)) {
                        manifest->attributeName = value;
                    } else if (0 == strcmp(var, ATTR_URL)) {
                        manifest->attributeURL = value;
                    } else if (0 == strcmp(var, ATTR_VERSION)) {
                        manifest->attributeVersion = value;
                    } else if (0 == strcmp(var, ATTR_LENGTH)) {
                        manifest->attributeLength = value;
                    } else if (0 == strcmp(var, ATTR_TYPE)) {
                        manifest->attributeType = value;
                    } else if (0 == strcmp(var, ATTR_BUILD)) {
                        if (!manifest->BuildLabel.empty() && manifest->BuildLabel != value) {
                            ctx.ParserError("build attribute redefined");
                        } else {
                            manifest->BuildLabel = value;
                        }
                    } else if (0 == strcmp(var, ATTR_OS)) {
                        if (!manifest->OSLabel.empty() && manifest->OSLabel != value) {
                            ctx.ParserError("os attribute redefined");
                        } else {
                            manifest->OSLabel = value;
                        }
                    } else if (0 == strcmp(var, ATTR_SHASIGNATURE)) {
                        manifest->attributeSHASignature = value;
                    } else if (0 == strcmp(var, ATTR_MD5SIGNATURE)) {
                        manifest->attributeMD5Signature = value;
                    } else if (0 == strcmp(var, ATTR_EDSIGNATURE)) {
                        manifest->attritbuteEDSignature = value;
                    }
                }
            }
        }
    }
}


void XMLCALL
OnEndElement(void *data, const char *name)
{
    ParserContext& ctx = *static_cast<ParserContext*>(data);
    AutoManifest *manifest = ctx.manifest;

    if (ctx.in_tags) {
        if (ctx.PrefixFieldMatch(name, ATOM_TAGS)) {
            assert(0 == ctx.criticalUpdate_level);
            ctx.in_tags = false;
        }

    } else if (ctx.in_item) {
                                                // </title>
        if (0 == strcmp(name, ATOM_TITLE)) {
            if (1 == ctx.title_level && manifest) {
                trim(manifest->title);
                LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                        << "->title<" << manifest->title << ">" << LOG_ENDL;
            }
            --ctx.title_level;
                                                // </link>
        } else if (0 == strcmp(name, ATOM_LINK)) {
            if (1 == ctx.link_level && manifest) {
                trim(manifest->link);
                LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                        << "->link<" << manifest->link << ">" << LOG_ENDL;
            }
            --ctx.link_level;
                                                // </description>
        } else if (0 == strcmp(name, ATOM_DESCRIPTION)) {
            if (1 == ctx.description_level && manifest) {
                trim(manifest->description);
                LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                        << "->description<" << manifest->description << ">" << LOG_ENDL;
                if (! manifest->releaseNotesLink.empty()) {
                    trim(manifest->releaseNotesLink);
                    LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                            << "->releaseNotesLink<" << manifest->releaseNotesLink << ">" << LOG_ENDL;
                }
            }
            --ctx.description_level;
                                                // </updater:releaseNotesLink>
        } else  if (ctx.PrefixFieldMatch(name, ATOM_RELEASENOTESLINK)) {
            if (1 == ctx.releaseNotesLink_level && manifest) {
                trim(manifest->releaseNotesLink);
                LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                        << "->releaseNotesLink<" << manifest->releaseNotesLink << ">" << LOG_ENDL;
            }
            --ctx.releaseNotesLink_level;
                                                // </updater:version>
        } else if (ctx.PrefixFieldMatch(name, ATOM_VERSION)) {
            if (1 == ctx.version_level && manifest) {
                trim(manifest->version);
                LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                        << "->version<" << manifest->version << ">" << LOG_ENDL;
            }
            --ctx.version_level;
                                                // </updater:minimumSystemVersion>
        } else if (ctx.PrefixFieldMatch(name, ATOM_MINIMUMSYSTEMVERSION)) {
            if (1 == ctx.minimumSystemVersion_level && manifest) {
                trim(manifest->minimumSystemVersion);
                LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                        << "->minimumSystemVersion" << manifest->minimumSystemVersion << ">" << LOG_ENDL;
            }
            --ctx.minimumSystemVersion_level;
                                                // </updater:criticalUpdate>
        } else if (ctx.PrefixFieldMatch(name, ATOM_CRITICALUPDATE)) {
            if (1 == ctx.criticalUpdate_level && manifest) {
                trim(manifest->criticalUpdate);
                LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                        << "->criticalUpdate" << manifest->criticalUpdate << ">" << LOG_ENDL;
            }
            --ctx.criticalUpdate_level;
                                                // <installerArguments>
        } else if (ctx.PrefixFieldMatch(name, ATOM_INSTALLERARGUMENTS)) {
            if (1 == ctx.installerArguments_level && manifest) {
                trim(manifest->installerArguments);
                LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                        << "->installerArguments<" << manifest->installerArguments << ">" << LOG_ENDL;
            }
            --ctx.installerArguments_level;
                                                // </published>
        } else if (0 == strcmp(name, ATOM_PUBLISHED)) {
            if (1 == ctx.published_level && manifest) {
                trim(manifest->pubDate);
                LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                        << "->published<" << manifest->published << ">" << LOG_ENDL;
                LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                        << "->pubDate<" << manifest->pubDate << ">" << LOG_ENDL;
            }
            --ctx.published_level;
                                                // </pubDate>
        } else if (0 == strcmp(name, ATOM_PUBDATE)) {
            if (1 == ctx.pubDate_level && manifest) {
                trim(manifest->pubDate);
                LOG<LOG_TRACE>() << "Manifest[" << ctx.LineNumber() << "]"
                        << "->pubDate<" << manifest->pubDate << ">" << LOG_ENDL;
            }
            --ctx.pubDate_level;
                                                // </item>
        } else if (0 == strcmp(name, ATOM_ITEM)) {
            ctx.manifest = NULL;
            ctx.in_item = false;
        }
                                                // </channel>
    } else if (0 == strcmp(name, ATOM_CHANNEL)) {
        if (ParserContext::CHANNEL_ACTIVE == ctx.channel_status) {
            ctx.ParserComplete();
                // channel matched, complete.
        }
        ctx.channel_status = ParserContext::CHANNEL_NONE;
    }
}


void XMLCALL
OnText(void *data, const char *s, int len)
{
    ParserContext& ctx = *static_cast<ParserContext*>(data);
    AutoManifest *manifest = ctx.manifest;

    if (ctx.in_tags) {

    } else if (ctx.in_item && manifest) {
        if (1 == ctx.title_level) {             // <title>
            manifest->title.append(s, len);

        } else if (1 == ctx.link_level) {       // <link>
            manifest->link.append(s, len);

        } else if (1 == ctx.description_level) {// <description>
            manifest->description.append(s, len);

        } else if (1 == ctx.releaseNotesLink_level) { // <updater:releaseNotesLink>
            if (! manifest->releaseNotesLink.empty()) {
                ctx.ParserWarning("description url attribute and releaseNotesLink element are mutually exclusive");
            }
            manifest->releaseNotesLink.append(s, len);

        } else if (1 == ctx.version_level) {    // <updater:version>
            manifest->version.append(s, len);
                                                // <updater:minimumSystemVersion>
        } else if (1 == ctx.minimumSystemVersion_level) {
            manifest->minimumSystemVersion.append(s, len);

                                                // <installerArguments>
        } else if (1 == ctx.installerArguments_level) {
            manifest->installerArguments.append(s, len);

        } else if (1 == ctx.published_level) {  // <published>
            if (len) {
                const std::string t_buffer(s, len);
#if defined(__MINGW64_VERSION_MAJOR)
                const __time64_t published = (__time64_t)strtoull(t_buffer.c_str(), NULL, 10);
#else
                const time_t published = atoi(t_buffer.c_str());
#endif

                manifest->published = published;
                if (published > 0 && manifest->pubDate.empty()) {
                    struct tm tm = {0};
                    char pubDate[32] = {0};

#if defined(__WATCOMC__)
                    _localtime(&published, &tm);// Sat, 20 Dec 2014 10:00:00
#else
                    _localtime64_s(&tm, &published);// Sat, 20 Dec 2014 10:00:00
#endif
                    strftime(pubDate, sizeof(pubDate), "%a %d %b %Y" /*" %H:%M:%S"*/, &tm);
                    manifest->pubDate = pubDate;
                }
            }

        } else if (1 == ctx.pubDate_level) {    // <pubDate>
            manifest->pubDate.append(s, len);
        }
    }
}

}   // anonymous namespace

bool
AutoManifest::Load(const std::string& xml, const std::string &channel, const std::string &os_label)
{
    LOG<LOG_INFO>() << "Parsing XML for channel=" << channel << ", osl=" << os_label << LOG_ENDL;

    // load manifest
    XML_Parser parser = XML_ParserCreate(NULL);
    if (NULL == parser) {
        throw AppException("Failed to create XML parser.");
    }

    ParserContext ctx(parser, channel.c_str());
    XML_SetElementHandler(parser, OnStartElement, OnEndElement);
    XML_SetCharacterDataHandler(parser, OnText);
    XML_SetUserData(parser, &ctx);

    XML_Status st = XML_Parse(parser, xml.c_str(), (int)xml.size(), XML_TRUE);
    if (st == XML_STATUS_ERROR) {
        std::string msg("XML parser error: ");
        msg.append(XML_ErrorString(XML_GetErrorCode(parser)));
        XML_ParserFree(parser);
        throw AppException(msg);
    }
    XML_ParserFree(parser);

    // select suitable element
    if (const AutoManifest *manifest = ctx.best_match()) {
        *this = *manifest;
        return true;
    }
    return false;
}


bool
AutoManifest::IsCriticalUpdate(const std::string &app_version) const
{
    if (! criticalUpdate.empty()) {
        //  <criticalUpdate [version=""></criticalUpdate> tag at the top-level element.
        //
        //  Additionally, the version that was last critical can be specified.
        //
        //      <updater:criticalUpdate updater:version="1.2.4"></updater:criticalUpdate>
        //
        //  For example, when 1.2.5 is released you can specify that only versions less
        //  than 1.2.4 should treat this as a critical update.
        //
        if (criticalUpdate == "*" ||
                AutoVersion::Compare(app_version, criticalUpdate) < 0) {
            LOG<LOG_INFO>() << "IsCritical: yes, current=" << app_version
                << ", critical=" << criticalUpdate << LOG_ENDL;
            return true;
        }
    }

    LOG<LOG_INFO>() << "IsCritical: no, current=" << app_version
            << ", critical=" << criticalUpdate << LOG_ENDL;
    return false;
}

}   // namespace Updater

