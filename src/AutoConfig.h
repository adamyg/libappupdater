#ifndef AUTOCONFIG_H_INCLUDED
#define AUTOCONFIG_H_INCLUDED
//  $Id: AutoConfig.h,v 1.17 2025/02/21 19:03:23 cvsuser Exp $
//
//  AutoUpdater: configuration management.
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

#include <vector>
#include <string>
#if !defined(__WATCOMC__)
#include <sstream>
#endif

#include "AutoLogger.h"

namespace Updater {
class Config {
    enum {
        QUERY_HOSTURL       = (1 << 0),
        QUERY_CHANNEL       = (1 << 1),
        QUERY_OSLABEL       = (1 << 2),
        QUERY_PRODUCTNAME   = (1 << 3),
        QUERY_PRODUCTVERSION = (1 << 4),
        QUERY_COMPANYNAME   = (1 << 5),
        QUERY_BUILDLABEL    = (1 << 6)
    };

public:
    /// Retrieve the console mode.
    static int               GetConsoleMode();

    /// Retrieve host location.
    static const std::string& GetFeedURL();

    /// Retrieve channel name.
    static const std::string& GetChannel();

    /// Retrieve OSItem name.
    static const std::string& GetOSLabel();

    /// Retrieve application name.
    static const std::string& GetAppName();

    /// Retrieve application version.
    static const std::string& GetAppVersion();

    /// Retrieve company name.
    static const std::string& GetCompanyName();

    /// Retrieve build label.
    static const std::string& GetBuildLabel();

    /// Retrieve registry path, derived from the application details
    static const std::string& GetRegistryPath();

    /// Set the console interface mode
    static void             SetConsoleMode(int mode);

    /// Set language
    static void             SetLanguage(const char *language);

    /// Set host location
    static void             SetHostURL(const char *url);

    /// Get channel name
    static void             SetChannel(const char *channel);

    /// Get OS label
    static void             SetOSLabel(const char *os_label);

    /// Set application name
    static void             SetAppName(const char *appname);

    /// Set application version
    static void             SetAppVersion(const char *version);

    /// Set company name
    static void             SetCompanyName(const char *coname);

    /// Set Windows registry path to store settings in (relative to HKCU/KHLM).
    static void             SetRegistryPath(const char *path);

    /*
     *  Access to runtime configuration.
     */

    // Writes given value to the registry under this name.
#if defined(__WATCOMC__)
    static void             WriteConfigValue(const char *name, const bool& value);
    static void             WriteConfigValue(const char *name, const int& value);
    static void             WriteConfigValue(const char *name, const unsigned long& value);
    static void             WriteConfigValue(const char *name, const std::string &value);
#else
    template<typename T>
    static void WriteConfigValue(const char *name, const T& value)
        {
            std::ostringstream s;
            s << value;
            WriteConfigValueImpl(name, s.str().c_str());
        }
#endif

    // Reads a value from the registry. Returns true if it was present, false otherwise.
#if defined(__WATCOMC__)
    static bool             ReadConfigValue(const char *name, bool& value);
    static bool             ReadConfigValue(const char *name, int& value);
    static bool             ReadConfigValue(const char *name, unsigned long& value);
    static bool             ReadConfigValue(const char *name, std::string &value);
#else
    template<typename T>
    inline static bool      ReadConfigValue(const char *name, T& value)
        {
            const std::string v = ReadConfigValueImpl(name);
            if (v.empty())
                return false;
            std::istringstream s(v);
            s >> value;
            return !s.fail();
        }
#endif

    // Delete a value from the registry
    static bool             DeleteConfigValue(const char *name);

private:
    Config();                                   // cannot be instantiated

    // Retrieve the VERSIONINFO resource.
    static const void *     GetVerInfoData();

    // Retrieve the VERSIONINFO//StringFileInfo resource.
    static const std::string& GetStringFileInfo();

    // Retrieve the given VERSIONINFO/StringFileInfo field.
    static std::string      GetVerInfoField(const char *field)
        { return GetVerInfoField(field, true); }
    static std::string      TryGetVerInfoField(const char *field)
        { return GetVerInfoField(field, false); }
    static std::string      GetVerInfoField(const char *field, bool required);

    // Retrieve the custom resource field value.
    static std::string      GetPrivateValue(const char *name, const char *type, bool required= true);

    static std::string      GetDefaultRegistryPath();

    static void             WriteConfigValueImpl(const char *name, const char *value);
    static std::string      ReadConfigValueImpl(const char *name);

private:
    // RAII for VersionInfo storage.
    class VersionInfo {
    public:
        VersionInfo() : data_(NULL) { }
        ~VersionInfo() {
            ::free(data_);
        }
        void allocate(size_t bytes) {
            data_ = ::calloc(bytes, 1);
        }
        void *data() {
            return data_;
        }
    private:
        void *data_;
    };

private:
    static CriticalSection  critical_section_;
    static int              console_mode_;
    static unsigned         queried_status_;
    static VersionInfo      version_info_;
    static std::string      string_file_info_;
    static std::string      language_;
    static std::string      host_URL_;
    static std::string      channel_;
    static std::string      os_label_;
    static std::string      registry_path_;
    static std::string      default_registry_path_;
    static std::string      company_name_;
    static std::string      application_name_;
    static std::string      application_version_;
    static std::string      build_label_;
};

}   // namespace Updater

#endif  //AUTOSETTINGS_H_INCLUDED
