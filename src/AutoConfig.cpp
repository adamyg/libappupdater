//  $Id: AutoConfig.cpp,v 1.21 2025/02/21 19:03:23 cvsuser Exp $
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
#include <stdio.h>
#include <assert.h>

#include "AutoConfig.h"
#include "AutoLogger.h"
#include "AutoError.h"
#include "AutoString.h"

#if defined(PRAGMA_COMMENT_LIB)
#pragma comment(lib, "version.lib")
#endif

namespace Updater {

CriticalSection     Config::critical_section_;
int                 Config::console_mode_ = 0;
unsigned            Config::queried_status_ = 0;
Config::VersionInfo Config::version_info_;
std::string         Config::string_file_info_;
std::string         Config::language_;
std::string         Config::host_URL_;
std::string         Config::channel_;
std::string         Config::os_label_;
std::string         Config::registry_path_;
std::string         Config::default_registry_path_;
std::string         Config::company_name_;
std::string         Config::application_name_;
std::string         Config::application_version_;
std::string         Config::build_label_;

namespace {

struct TranslationInfo {
    WORD wLanguage;
    WORD wCodePage;
};

typedef LONG (WINAPI * RegDeleteKeyExA_t)(HKEY hKey, const char *lpSubKey, REGSAM samDesired, DWORD Reserved);
RegDeleteKeyExA_t x_RegDeleteKeyExA = NULL;


//  RegDeleteKeyExA() emulation, which simply clears the registry value.
//
LONG WINAPI
MyRegDeleteKeyExA(HKEY hKey, const char *lpSubKey, REGSAM /*samDesired*/, DWORD /*Reserved*/)
{
    return RegSetValueExA(hKey, lpSubKey, 0, REG_SZ, (const BYTE *)"", 1);
}


//  Localised version of RegDeleteKeyExA(),
//  required as RegDeleteKeyEx() is only available on XP x64 Professional or greater.
//
LONG WINAPI
XRegDeleteKeyExA(HKEY hKey, const char *lpSubKey, REGSAM samDesired, DWORD Reserved)
{
    if (NULL == x_RegDeleteKeyExA) {            // XP x64 Professional or greater.
        HMODULE hAdvAPI32 = LoadLibraryA("AdvAPI32.dll");

        assert(hAdvAPI32 != NULL);
#if defined(GCC_VERSION) && (GCC_VERSION >= 80000)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
        x_RegDeleteKeyExA = (RegDeleteKeyExA_t)GetProcAddress(hAdvAPI32, "RegDeleteKeyExA");
        if (NULL == x_RegDeleteKeyExA) {        // assign enumlation
            LOG<LOG_DEBUG>() << "RegDeleteKeyExA enumlation" << LOG_ENDL;
            x_RegDeleteKeyExA = MyRegDeleteKeyExA;
            FreeLibrary(hAdvAPI32);

        } else {
            LOG<LOG_DEBUG>() << "RegDeleteKeyExA available" << LOG_ENDL;
        }
#if defined(GCC_VERSION) && (GCC_VERSION >= 80000)
#pragma GCC diagnostic pop
#endif
    }
    return (*x_RegDeleteKeyExA)(hKey, lpSubKey, samDesired, Reserved);
}


//  Retrieve the common VERSIONINFO block.
//
const VS_FIXEDFILEINFO *
GetVerInfoFixed(const void *fi)
{
    VS_FIXEDFILEINFO *info = NULL;
    UINT size = 0;

    if (! ::VerQueryValueA((void *)fi, "\\", (LPVOID *)&info, &size)) {
        throw SysException("Unable to access executable FIXEDFILEINFO");
    }
    assert(size == sizeof(VS_FIXEDFILEINFO));
    return info;
}

}   // anonymous namespace


//private
//  Retrieve the application VERIOSINFO image.
//
const void *
Config::GetVerInfoData()
{
    if (version_info_.data()) {
        return version_info_.data();
    }

    wchar_t module[MAX_PATH + 1] = {0};

    if (! GetModuleFileNameW(NULL, module, MAX_PATH)) {
        throw SysException("GetModuleFileName");
    }

    LOG<LOG_INFO>() << "Config::Module=" << Updater::to_string(module) << LOG_ENDL;

    DWORD dwHandle = 0;
    DWORD fiSize = GetFileVersionInfoSizeW(module, &dwHandle);
    if (0 == fiSize) {
        throw SysException("Executable missing VERSIONINFO resource");
    }

    version_info_.allocate(fiSize);
    if (NULL == version_info_.data() ||
            ! GetFileVersionInfoW(module, dwHandle, fiSize, version_info_.data())) {
        throw SysException("GetFileVersionInfo");
    }

    const VS_FIXEDFILEINFO *fxi = GetVerInfoFixed(version_info_.data());
    const DWORD dwFileFlags = (fxi->dwFileFlags & VS_FFI_FILEFLAGSMASK);
    std::string file_flags;

    file_flags  = " [";
    if (dwFileFlags & VS_FF_DEBUG) file_flags += "DEBUG,";
    if (dwFileFlags & VS_FF_PATCHED) file_flags += "PATCHED,";
    if (dwFileFlags & VS_FF_PRERELEASE) file_flags += "PRERELEASE,";
    if (dwFileFlags & VS_FF_PRIVATEBUILD) file_flags += "PRIVATEBUILD,";
    if (dwFileFlags & VS_FF_SPECIALBUILD) file_flags += "SPECIALBUILD,";
    if (file_flags.size() > 2) file_flags.resize(file_flags.size () - 1);
    file_flags += "]";

    LOG<LOG_INFO>() << "Config::VERSIONINFO" << LOG_ENDL;
    LOG<LOG_INFO>() << " FILEVERSION    "
        << HIWORD(fxi->dwFileVersionMS) << "," << LOWORD(fxi->dwFileVersionMS) << ","
        << HIWORD(fxi->dwFileVersionLS) << "," << LOWORD(fxi->dwFileVersionLS) << LOG_ENDL;
    LOG<LOG_INFO>() << " PRODUCTVERSION "
        << HIWORD(fxi->dwProductVersionMS) << "," << LOWORD(fxi->dwProductVersionMS) << ","
        << HIWORD(fxi->dwProductVersionLS) << "," << LOWORD(fxi->dwProductVersionLS) << LOG_ENDL;
    LOG<LOG_INFO>() << " FILEFLAGSMASK  0x" << std::hex << fxi->dwFileFlagsMask << std::dec << LOG_ENDL;
    LOG<LOG_INFO>() << " FILEFLAGS      0x" << std::hex << fxi->dwFileFlags << file_flags << std::dec << LOG_ENDL;
    LOG<LOG_INFO>() << " FILEOS         0x" << std::hex << fxi->dwFileOS << std::dec << LOG_ENDL;
    LOG<LOG_INFO>() << " FILETYPE       0x" << std::hex << fxi->dwFileType << std::dec << LOG_ENDL;
    LOG<LOG_INFO>() << " FILESUBTYPE    0x" << std::hex << fxi->dwFileSubtype << std::dec << LOG_ENDL;

    return version_info_.data();
}


//private
//  Retrieve the resource language StringFileInfo key.
//
const std::string&
Config::GetStringFileInfo()
{
    if (! string_file_info_.empty()) {
        return string_file_info_;
    }

    const struct TranslationInfo *translations = NULL;
    UINT count = 0, phase, idx = 0;
    WORD lang;

    if (! ::VerQueryValue((void *)GetVerInfoData(), TEXT("\\VarFileInfo\\Translation"), (LPVOID *)&translations, &count)) {
        throw SysException("Executable does not have required VERSIONINFO\\VarFileInfo resource");
    }

    count /= sizeof(struct TranslationInfo);
    if (0 == count) {
        throw AppException("No translations in VarFileInfo resource?");
    }

    if (count > 1) {                            // multiple, resolve
        for (phase = 0, idx = count; idx >= count && phase < 4; ++phase) {
            switch (phase) {
            case 0: lang = GetUserDefaultLangID(); break;
            case 1: lang = GetSystemDefaultLangID(); break;
            case 2: lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US); break;
            case 3: lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL); break;
            }

            for (idx = 0; idx < count; ++idx) {
                if (translations[idx].wLanguage == lang) {
                    break;                      // match
                }
            }
        }

        if (idx >= count) {
            idx = 0;                            // no match, use first.
        }
    }

    char t_result[32];
    sprintf_s(t_result, sizeof(t_result), "\\StringFileInfo\\%04x%04x",
        (unsigned)translations[idx].wLanguage, (unsigned) translations[idx].wCodePage);

    LOG<LOG_INFO>() << "Config::VERSIONINFO" << t_result << LOG_ENDL;
    string_file_info_.assign(t_result);

    return string_file_info_;
}


//private
// Retrieve the given VERSIONINFO/StringFileInfo field.
//
std::string
Config::GetVerInfoField(const char *field, bool required)
{
    const std::string key(GetStringFileInfo() + "\\" + field);
    const char *value;
    UINT len;

    if (! ::VerQueryValueA((void *)GetVerInfoData(), (LPSTR)(key.c_str()), (void **)&value, &len)) {
        if (required) {
            throw SysException("Executable does not have required key in StringFileInfo");
        }
        value = "";
    }
    LOG<LOG_INFO>() << "Config::VERSIONINFO" << key << "=" << value << LOG_ENDL;
    return value;
}


//private
//  Retrieve the private resource field value.
//
std::string
Config::GetPrivateValue(const char *name, const char *type, bool required)
{
    const HINSTANCE module = 0;                 // main executable
    HRSRC hRes = FindResourceA(module, name, type);
    if (hRes) {
        HGLOBAL hData = LoadResource(module, hRes);
        if (hData) {
            const char *data = (const char*)::LockResource(hData);
            size_t size = ::SizeofResource(module, hRes);

            if (data && size) {
                if (data[size-1] == '\0') {     // null-terminated string
                    --size;
                }
                std::string value(data, size);
                LOG<LOG_INFO>() << "Config::" << name << "\\" << type << "=" << value << LOG_ENDL;
                return value;
            }
        }
    }

    if (required) {
        std::string message("Executable does not have required resource ");
            message += name, message += "\\", message += type;
        throw SysException(message);
    }

    LOG<LOG_INFO>() << "Config::" << name << "\\" << type << "=" << LOG_ENDL;
    return std::string();
}


//public
void
Config::SetConsoleMode(int mode)
{
    CriticalSection::Guard lock(critical_section_);
    console_mode_ = mode;
    LOG<LOG_INFO>() << "Config::SetConsoleMode=" << console_mode_ << LOG_ENDL;
}


//public
void
Config::SetLanguage(const char *language)
{
    CriticalSection::Guard lock(critical_section_);
    language_.assign(language?language:"");
    LOG<LOG_INFO>() << "Config::SetLanguage=" << language_ << LOG_ENDL;
}


//public
void
Config::SetHostURL(const char *url)
{
    CriticalSection::Guard lock(critical_section_);
    host_URL_.assign(url?url:"");
    LOG<LOG_INFO>() << "Config::SetHostURL=" << host_URL_.c_str() << LOG_ENDL;
}


//public
void
Config::SetChannel(const char *channel)
{
    CriticalSection::Guard lock(critical_section_);
    channel_.assign(channel?channel:"");
    LOG<LOG_INFO>() << "Config::SetChannel=" << channel_ << LOG_ENDL;
}


//public
void
Config::SetOSLabel(const char *os_label)
{
    CriticalSection::Guard lock(critical_section_);
    os_label_.assign(os_label?os_label:"");
    LOG<LOG_INFO>() << "Config::SetChannel=" << os_label_ << LOG_ENDL;
}


//public
void
Config::SetAppName(const char *appname)
{
    CriticalSection::Guard lock(critical_section_);
    application_name_.assign(appname?appname:"");
    LOG<LOG_INFO>() << "Config::SetAppName=" << application_name_ << LOG_ENDL;
}


//public
void
Config::SetAppVersion(const char *version)
{
    CriticalSection::Guard lock(critical_section_);
    application_version_.assign(version?version:"");
    LOG<LOG_INFO>() << "Config::SetAppVersion=" << application_version_ << LOG_ENDL;
}


//public
void
Config::SetCompanyName(const char *coname)
{
    CriticalSection::Guard lock(critical_section_);
    company_name_.assign(coname?coname:"");
    LOG<LOG_INFO>() << "Config::SetCompanyName=" << company_name_ << LOG_ENDL;
}


//public
void
Config::SetRegistryPath(const char *path)
{
    CriticalSection::Guard lock(critical_section_);
    registry_path_.assign(path?path:"");
    LOG<LOG_INFO>() << "Config::SetRegistryPath=" << registry_path_ << LOG_ENDL;
}


//public
int
Config::GetConsoleMode()
{
    return console_mode_;
}


//public
const std::string&
Config::GetFeedURL()
{
    CriticalSection::Guard lock(critical_section_);
    if (host_URL_.empty()) {
        if (0 == (queried_status_ & QUERY_HOSTURL)) {  // non-optional resource
            //
            //  UPDATER
            //      FeedURL         Update manifest feed URL.
            //
            host_URL_ = GetPrivateValue("UPDATER", "FeedURL");
            queried_status_ |= QUERY_HOSTURL;
        }
    }
    return host_URL_;
}


//public
const std::string&
Config::GetChannel()
{
    CriticalSection::Guard lock(critical_section_);
    if (channel_.empty()) {
        if (0 == (queried_status_ & QUERY_CHANNEL)) { // optional resource
            //
            //  UPDATER
            //      Channel         Optional development channel, for example "debug".
            //
            channel_ = GetPrivateValue("UPDATER", "Channel", false);
            queried_status_ |= QUERY_CHANNEL;
        }
    }
    return channel_;
}


//public
const std::string&
Config::GetOSLabel()
{
    CriticalSection::Guard lock(critical_section_);
    if (os_label_.empty()) {
        if (0 == (queried_status_ & QUERY_OSLABEL)) { // optional resource
            os_label_ = GetPrivateValue("UPDATER", "OSLabel", false);
            queried_status_ |= QUERY_OSLABEL;
        }
    }
    return os_label_;
}


//public
const std::string&
Config::GetAppName()
{
    CriticalSection::Guard lock(critical_section_);
    if (application_name_.empty()) {
        if (0 == (queried_status_ & QUERY_PRODUCTNAME)) {
            //
            //  VERSIONINFO
            //      ProductName     Name of the product with which the file is distributed.
            //                      This string is required.
            //
            application_name_ = GetVerInfoField("ProductName");
            queried_status_ |= QUERY_PRODUCTNAME;
        }
    }
    return application_name_;
}


//public
const std::string&
Config::GetAppVersion()
{
    CriticalSection::Guard lock(critical_section_);
    if (application_version_.empty()) {
        if (0 == (queried_status_ & QUERY_PRODUCTVERSION)) {
            //
            //  VERSIONINFO
            //      ProductVersion  Version of the product with which the file is distributed,
            //                      for example, "3.10" or "5.00.RC2". This string is required.
            //
            application_version_ = GetVerInfoField("ProductVersion");
            queried_status_ |= QUERY_PRODUCTVERSION;
        }
    }
    return application_version_;
}


//public
const std::string&
Config::GetCompanyName()
{
    CriticalSection::Guard lock(critical_section_);
    if (company_name_.empty()) {
        if (0 == (queried_status_ & QUERY_COMPANYNAME)) {
            //
            //  VERSIONINFO
            //      CompanyName     Company that produced the file, for example,
            //                      "Microsoft Corporation" or "Standard Microsystems Corporation, Inc."
            //                      This string is required.
            //
            company_name_ = GetVerInfoField("CompanyName");
            if (company_name_.find("<Company") != std::string::npos ||
                    company_name_.find("<company") != std::string::npos ||
                    company_name_.find("TODO") != std::string::npos) {
                company_name_ = "";            // ignore defaults
            }
            queried_status_ |= QUERY_COMPANYNAME;
        }
    }
    return company_name_;
}


//public
const std::string&
Config::GetBuildLabel()
{
    CriticalSection::Guard lock(critical_section_);
    if (build_label_.empty()) {
        if (0 == (queried_status_ & QUERY_BUILDLABEL)) {
            //
            //  VERSIONINFO
            //      PrivateBuild    Information about a private version of the file, for example,
            //                      "Built by TESTER1 on \TESTBED". This string should be present only
            //                      if VS_FF_PRIVATEBUILD is specified in the fileflags parameter of the root block.
            //
            //      SpecialBuild    Text that indicates how this version of the file differs from the standard version,
            //                      for example, "Private build for TESTER1 solving mouse problems on M250 and M250E computers".
            //                      This string should be present only if VS_FF_SPECIALBUILD is specified in the
            //                      fileflags parameter of the root block.
            //
            const VS_FIXEDFILEINFO *fxi = GetVerInfoFixed(GetVerInfoData());
            const DWORD dwFileFlags = (fxi ? fxi->dwFileFlags : 0);
            std::string label;

            if (dwFileFlags & VS_FF_DEBUG) label += "debug,";
            if (dwFileFlags & VS_FF_PATCHED) label += "patched,";
            if (dwFileFlags & VS_FF_PRERELEASE) label += "prerelease,";
         // if (dwFileFlags & VS_FF_PRIVATEBUILD) label += "PRIVATEBUILD,";
         // if (dwFileFlags & VS_FF_SPECIALBUILD) label += "SPECIALBUILD,";
            if (label.size() > 2) label.resize(label.size () - 1);

            queried_status_ |= QUERY_BUILDLABEL;
            build_label_ = label;
        }
    }
    return build_label_;
}


//public
const std::string&
Config::GetRegistryPath()
{
    CriticalSection::Guard lock(critical_section_);
    if (registry_path_.empty()) {
        registry_path_ = GetDefaultRegistryPath();
        LOG<LOG_DEBUG>() << "RegistryPath = \"" << registry_path_ << "\"" << LOG_ENDL;
    }
    return registry_path_;
}


//private
std::string
Config::GetDefaultRegistryPath()
{
    if (default_registry_path_.empty()) {
        default_registry_path_ = "Software\\";
        std::string vendor = Config::GetCompanyName();
        if (! vendor.empty()) {
            default_registry_path_ += vendor + "\\";
        }
        default_registry_path_ += Config::GetAppName();
        default_registry_path_ += "\\AutoUpdate";
    }
    return default_registry_path_;
}


namespace {
CriticalSection x_config_critical_section;

void
RegistryWrite(const char *name, const char *value)
{
    const std::string subkey = Config::GetRegistryPath();

    HKEY key;
    LONG result = RegCreateKeyExA(HKEY_CURRENT_USER, subkey.c_str(),
                        0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &key, NULL);
    if (result != ERROR_SUCCESS) {
        throw SysException("Cannot write registry setting");
    }

    result = RegSetValueExA(key, name, 0, REG_SZ, (const BYTE*)value, (DWORD)(strlen(value) + 1));
    if (result != ERROR_SUCCESS) {
        throw SysException("Cannot write registry setting");
    }

    RegCloseKey(key);
}


bool
RegistryRead(HKEY root, const char *name, char *buf, size_t len)
{
    const std::string subkey = Config::GetRegistryPath();

    HKEY key;
    LONG result = RegOpenKeyExA(root, subkey.c_str(), 0, KEY_QUERY_VALUE, &key);
    if (result != ERROR_SUCCESS) {
        if (ERROR_FILE_NOT_FOUND == result) {
            return false;
        }
        throw SysException("Cannot read registry setting");
    }

    DWORD type, buflen = (DWORD)len;
    result = RegQueryValueExA(key, name, 0, &type, (BYTE*)buf, &buflen);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS) {
        if (ERROR_FILE_NOT_FOUND == result) {
            return false;
        }
        throw SysException("Cannot read registry setting");
    }

    if (type != REG_SZ) {                       // incorrect type, ignore
        return false;
    }
    return true;
}


bool
RegistryRead(const char *name, char *buf, size_t len)
{
    if (RegistryRead(HKEY_CURRENT_USER, name, buf, len) ||
            RegistryRead(HKEY_LOCAL_MACHINE, name, buf, len)) {
        return true;
    }
    return false;
}


bool
RegistryDelete(HKEY root, const char *name)
{
    const std::string subkey = Config::GetRegistryPath();

    HKEY key;
    LONG result = RegOpenKeyExA(root, subkey.c_str(), 0, KEY_SET_VALUE, &key);
    if (result != ERROR_SUCCESS) {
        if (ERROR_FILE_NOT_FOUND == result) {
            return false;
        }
        throw SysException("Cannot delete registry setting");
    }

    // Remove the name.
    result = XRegDeleteKeyExA(key, name, 0, 0);
    RegCloseKey(key);

    LOG<LOG_DEBUG>() << "RegDeleteKeyExA=" << result << LOG_ENDL;
    if (result != ERROR_SUCCESS) {
        if (ERROR_FILE_NOT_FOUND != result) {
            throw SysException("Cannot delete registry setting");
        }
    }
    return true;
}


bool
RegistryDelete(const char *name)
{
    bool ret = false;
    ret |= (RegistryDelete(HKEY_CURRENT_USER, name) ? true : false);
    ret |= (RegistryDelete(HKEY_LOCAL_MACHINE, name) ? true : false);
    return ret;
}

}   // anonymous namespace


void
Config::WriteConfigValueImpl(const char *name, const char *value)
{
    CriticalSection::Guard lock(x_config_critical_section);
    RegistryWrite(name, value);
    LOG<LOG_DEBUG>() << "RegistryWrite(" << name << ") = \"" << value << "\"" << LOG_ENDL;
}


std::string
Config::ReadConfigValueImpl(const char *name)
{
    CriticalSection::Guard lock(x_config_critical_section);
    char buf[1025];
    if (! RegistryRead(name, buf, sizeof(buf))) {
        buf[0] = 0;
    }
    LOG<LOG_DEBUG>() << "RegistryRead(" << name << ") = \"" << buf << "\"" << LOG_ENDL;
    return std::string(buf);
}


bool
Config::DeleteConfigValue(const char *name)
{
    CriticalSection::Guard lock(x_config_critical_section);
    const bool ret = RegistryDelete(name);
    LOG<LOG_DEBUG>() << "RegistryDelete(" << name << ") = " << ret << LOG_ENDL;
    return ret;
}


#if defined(__WATCOMC__)

bool Config::ReadConfigValue(const char *name, bool &value)
{
    const std::string v = Config::ReadConfigValueImpl(name);
    if (v.empty())
        return false;
    value = (bool) std::atoi(v.c_str());
    return true;
}

bool Config::ReadConfigValue(const char *name, int &value)
{
    const std::string v = Config::ReadConfigValueImpl(name);
    if (v.empty())
        return false;
    value = (int) std::atoi(v.c_str());
    return true;
}

bool Config::ReadConfigValue(const char *name, unsigned long &value)
{
    const std::string v = Config::ReadConfigValueImpl(name);
    if (v.empty())
        return false;
    value = (int) std::atoi(v.c_str());
    return true;
}

bool Config::ReadConfigValue(const char *name, std::string &value)
{
    value = Config::ReadConfigValueImpl(name);
    if (value.empty())
        return false;
    return true;
}

void Config::WriteConfigValue(const char *name, const bool& value)
{
    char t_buffer[32];
    sprintf(t_buffer, "%u", (int)value);
    WriteConfigValueImpl(name, t_buffer);
}

void Config::WriteConfigValue(const char *name, const int& value)
{
    char t_buffer[32];
    sprintf(t_buffer, "%d", value);
    WriteConfigValueImpl(name, t_buffer);
}

void Config::WriteConfigValue(const char *name, const unsigned long& value)
{
    char t_buffer[32];
    sprintf(t_buffer, "%lu", value);
    WriteConfigValueImpl(name, t_buffer);
}

void Config::WriteConfigValue(const char *name, const std::string &value)
{
    WriteConfigValueImpl(name, value.c_str());
}

#endif  //__WATCOMC__

}  // namespace Updater

//end
