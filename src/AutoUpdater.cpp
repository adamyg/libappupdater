//  $Id: AutoUpdater.cpp,v 1.33 2025/04/21 14:01:35 cvsuser Exp $
//
//  AutoUpdater: Application interface.
//
//  Registry Usage:
//
//      AutoUpdater run-time behavior can be configured by tweaking its registry settings detailed
//      in the table below. They are stored in the under "Software\<VENDOR>\<APPNAME>\AutoUpdater"
//      registry key.
//
//    Configuration:
//
//      Key             Type    Description
//      -------------------------------------------------------------------------
//      AutoCheck       bool    Whether updates should be checked
//                              automatically (default false).
//
//      AutoInterval    int     Automatic update re-check interval, expressed
//                              in days (default 2).
//
//      SkipInterval    int     Number of days skip-requests are honored before
//                              the user is re-prompted (default 14).
//
//    In addition the following settings are maintained and used internally.
//
//      Key             Type    Description
//      -------------------------------------------------------------------------
//      AutoOnce        bool    Whether the updater has run at least once.
//
//      AutoLast        time_t  Last time an update-check was performed.
//
//      SkipVersion     string  If the user skips an update, this contains the version
//                              of ignored update.
//
//      SkipTime        time_t  If the user skips an update, contains the UTC
//                              timestamp when the skip event occurred.
//
//      Note: All values are of the REG_SZ type, regardless of usage.
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

#include "common.h"

#include <ctime>
#include <cassert>
#include <memory>
#include <vector>

#include "AutoUpdater.h"
#include "IAutoUpdaterUI.h"

#include "AutoDialog.h"
#include "AutoConsole.h"

#include "AutoConfig.h"
#include "AutoVersion.h"
#include "AutoError.h"
#include "AutoLogger.h"
#include "AutoThread.h"
#include "AutoDownload.h"
#include "AutoGitHub.h"

#include "../ed25519/src/ed25519.h"
#include "../util/Format.h"
#include "../util/Base64.h"
#include "../util/Hex.h"

#include <Wincrypt.h>
#include <Rpc.h>                                // UnidCreate()
#if defined(PRAGMA_COMMENT_LIB)
#pragma comment(lib, "Rpcrt4.lib")
#endif
#include <shlobj.h>                             // SHGetFolderPath
#if defined(PRAGMA_COMMENT_LIB)
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shfolder.lib")
#endif

#include <io.h>                                 // _access()

static const char *
ConfigKeys[] = {
#define KEY_AUTOINTERVAL    "AutoInterval"
#define KEY_AUTOCHECK       "AutoCheck"
#define KEY_AUTOONCE        "AutoOnce"
#define KEY_AUTOSKIP        "AutoSkip"
#define KEY_AUTOLAST        "AutoLast"
#define KEY_AUTOCHANNEL     "AutoChannel"       // TODO
#define KEY_AUTOHOST        "AutoHost"          // TODO
#define KEY_SKIPVERSION     "SkipVersion"
#define KEY_SKIPTIME        "SkipTime"
#define KEY_SKIPINTERVAL    "SkipInterval"

    KEY_AUTOINTERVAL,
    KEY_AUTOCHECK,
    KEY_AUTOONCE,
    KEY_AUTOSKIP,
    KEY_AUTOLAST,
    KEY_AUTOCHANNEL,
    KEY_AUTOHOST,
    KEY_SKIPVERSION,
    KEY_SKIPTIME,
    KEY_SKIPINTERVAL
    };


using namespace Updater;

/////////////////////////////////////////////////////////////////////////////////////////
//  AutoUpdaterImpl
//

class AutoUpdaterImpl {
public:
    AutoUpdaterImpl(IAutoUpdaterUI *dialog) : 
        d_dialog(dialog), d_hTopWnd(0) {
    }

    ~AutoUpdaterImpl() {
        CleanTemp();
    }

    // RAII
    void SetDialog(IAutoUpdaterUI * /*dialog*/) {
        d_uibind.reset(new AutoDialogUI);
        d_dialog = d_uibind.get();
    }

    IAutoUpdaterUI *GetDialog() const {
        return d_dialog;
    }

    void RetainTemp() {
        d_tempfile.clear();                     // stop removal.
    }

    void CleanTemp() {
        if (d_tempfile.length()) {              // release temporary resources.
            ::DeleteFileA(d_tempfile.c_str());
            ::RemoveDirectoryA(d_tempdir.c_str());
            d_tempfile.clear();
        }
    }

    const std::string &LastError() const {
        return d_lasterror;
    }

    void SetLastError(const char *msg) {                    
        d_lasterror.assign(msg);
    }

    void SetLastError(const std::string &msg) {
        d_lasterror.assign(msg);
    }

    Updater::AutoManifest d_manifest;           // current application manifest.
#if defined(_MSC_VER) && (_MSC_VER <= 1500)
    std::tr1::shared_ptr <IAutoUpdaterUI> d_uibind;
#else
    std::shared_ptr<IAutoUpdaterUI> d_uibind;   // UI binding.
#endif
    IAutoUpdaterUI     *d_dialog;               // user dialog.
    HWND                d_hTopWnd;              // top window.
    std::string         d_tempdir;              // temporary working directory.
    std::string         d_tempfile;             // temporary working download file.
    std::string         d_lasterror;            // last reported error, if any.
};


/////////////////////////////////////////////////////////////////////////////////////////
//  AutoUpdaterSink
//

class AutoUpdaterSink : public FileDownloadSink {
    AutoUpdaterSink(const AutoUpdaterSink &rsh);
    AutoUpdaterSink& operator=(const AutoUpdaterSink &rsh);

public:
    AutoUpdaterSink(AutoUpdater &updater, const char *filename) :
        FileDownloadSink(filename), updater_(updater), total_(0), completed_(0), percentage_(0) {
    }

    virtual void set_size(size_t size) {
        FileDownloadSink::set_size(size); 
        total_ = size;
    }

    virtual void append(const void *data, size_t length) {
        FileDownloadSink::append(data, length);
        completed_ += length;
        if (total_) {
            const int percentage = 
                    (int)(((double)completed_ / total_) * 100.0);
            if (percentage != percentage_) {
                updater_.ProgressUpdate(completed_, total_);
                percentage_ = percentage;
            }
        }   
    }

    virtual bool cancelled() {
        return updater_.ProgressCancelled();
    }

private:
    AutoUpdater &updater_;
    size_t total_;
    size_t completed_;
    int percentage_;
};


/////////////////////////////////////////////////////////////////////////////////////////
//  AutoUpdater
//

AutoUpdater::AutoUpdater(IAutoUpdaterUI *dialog) :
    d_impl(new AutoUpdaterImpl(dialog))
{
}


AutoUpdater::~AutoUpdater()
{
    delete d_impl;
}


static void
function_reference()
{
}


HINSTANCE
AutoUpdater::ModuleHandle()
{
    static HMODULE hm = NULL;
    if (0 == hm) {
        if (::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR) &function_reference, &hm) != 0) {
            return hm;
        }
        assert(false);
    }
    return hm;
}


void
AutoUpdater::AppName(const char *appname)
{
    if (appname) Config::SetAppName(appname);
}


const char *
AutoUpdater::AppName() const
{
    return Config::GetAppName().c_str();
}


void
AutoUpdater::AppVersion(const char *appversion)
{
    if (appversion) Config::SetAppVersion(appversion);
}


const char *
AutoUpdater::AppVersion() const
{
    return Config::GetAppVersion().c_str();;
}


void
AutoUpdater::HostURL(const char *hosturl)
{
    if (hosturl) Config::SetHostURL(hosturl);
}


void
AutoUpdater::PublicKey(const char *base64, unsigned version)
{
    if (base64) Config::SetPublicKey(base64, version);
}


const char *
AutoUpdater::HostURL() const
{
    return Config::GetFeedURL().c_str();
}


void
AutoUpdater::EnableDialog()
{
    d_impl->SetDialog(new AutoDialogUI);
}


void
AutoUpdater::EnableConsole()
{
    d_impl->SetDialog(new AutoConsoleUI);
}


int
AutoUpdater::Execute(enum ExecuteMode mode, bool interactive)
{
    enum UpdateStatus status = STATUS_PROMPT;

    Logger::open_instance(AppName(), true);     // logger
    LOG<LOG_INFO>()
        << "AutoUpdate::Execute() mode=" << mode << ", interactive=" << interactive << LOG_ENDL;

    // support functionality
    switch (mode) {
    case ExecuteReset:
        Reset();
        return 0;
    case ExecuteDump:
        Dump();
        return 0;
    default:
        break;
    }

    // primary functions
    if (STATUS_PROMPT == (status = Status(mode))) {
        if (ExecuteAuto != mode || !Once()) {   // first time or prompt
            const int prompt = PromptDialog();

            LOG<LOG_DEBUG>() << "AutoUpdate: prompted check=" << prompt << LOG_ENDL;
            switch (prompt) {
            case PROMPT_AUTO:
                interactive = true;             // promoted, enable interactive
                status = STATUS_ENABLED;
                SetAuto(true);
                break;
            case PROMPT_ONCE:
                interactive = true;             // promoted, enable interactive
                status = STATUS_ENABLED;
                break;
            case PROMPT_NO:
                SetAuto(false);
                break;
            }
        }
    }

    int ret = 0;                                // 3=update available,2=installed,1=uptodate,0=nocheck

    if (STATUS_ENABLED == status) {
        try {
            const int isAvailable = IsAvailable(interactive);

            if (1 == isAvailable ||             // update available
                    (0 == isAvailable && ExecuteReinstall == mode)) {
                const bool isSkipped = IsSkipped();

                ret = 3;
                if (ExecuteReinstall == mode || ExecuteIgnoreSkip == mode || !isSkipped) {
                    LOG<LOG_DEBUG>() << "AutoUpdate: prompting install" << LOG_ENDL;
                    if (1 == InstallDialog()) { // query install
                        SetOnce(true);
                        ret = 2;
                    }
                } else {
                    LOG<LOG_DEBUG>() << "AutoUpdate: prompting skipped" << LOG_ENDL;
                }

            } else if (0 == isAvailable) {      // up-to-date
                if (interactive) {
                    UptoDateDialog();
                }
                ret = 1;
            }

            if (! d_impl->LastError().empty()) {
                d_impl->d_dialog->ErrorMessage(d_impl->LastError().c_str());
                ret = -1;
            }

        } catch (const std::exception &e) {
            std::string msg;
            msg += "An error occurred during updater operations\n";
            msg += e.what();

            LOG<LOG_ERROR>() << msg << LOG_ENDL;
            d_impl->d_dialog->ErrorMessage(msg.c_str());
            ret = -1;

        } catch (...) {
            const char *msg = "An unknown error occurred during updater operations\n";

            LOG<LOG_ERROR>() << msg << LOG_ENDL;
            d_impl->d_dialog->ErrorMessage(msg);
            ret = -1;
        }

        d_impl->CleanTemp();
    }
    return ret;
}


//
//  Determine current auto-updater status based upon the selected operational mode.
//
//      STATUS_DISABLED(0):     Disabled.
//      STATUS_ENABLED(1):      Auto-update check required.
//      STATUS_ALREADY(2):      Already performed within the last check interval.
//      STATUS_PROMPT(-1):      Not configured, prompt user.
//
enum AutoUpdater::UpdateStatus
AutoUpdater::Status(const enum ExecuteMode mode)
{
    switch (mode) {
    case ExecuteDisable:
        SetAuto(false);
        LOG<LOG_TRACE>() << "Status: disabled" << LOG_ENDL;
        return STATUS_DISABLED;

    case ExecuteEnable:
        SetAuto(true);
        LOG<LOG_TRACE>() << "Status: enabled" << LOG_ENDL;
        return STATUS_ENABLED;

    case ExecuteAuto: {
            int autointerval = 0;

            (void) Config::ReadConfigValue(KEY_AUTOINTERVAL, autointerval);
            const time_t expires =              // default, 2-days
                    ((time_t)((autointerval > 0 && autointerval <= 60 ? autointerval : 2)) * 60 * 60 * 24);

            time_t autolast = 0;
            if (Config::ReadConfigValue(KEY_AUTOLAST, autolast) && autolast) {
                const time_t autotime = autolast + expires,
                        nowtime = time(NULL);

                if (autotime > nowtime) {
                    LOG<LOG_TRACE>() << "Status: auto-delay(2), expires in "
                            << ((float)(autotime - nowtime))/(60 * 60 * 24) << " days" << LOG_ENDL;
                    return STATUS_ALREADY;      // periodic check
                }

                Config::WriteConfigValue(KEY_AUTOLAST, time(NULL));
            }
        }
        /*FALLTHRU*/

    case ExecutePrompt: {
            bool autocheck = false;

            if (Config::ReadConfigValue(KEY_AUTOCHECK, autocheck)) {
                if (autocheck) {
                    LOG<LOG_TRACE>() << "Status: auto-check(enabled)" << LOG_ENDL;
                    return STATUS_ENABLED;
                } else if (ExecuteAuto == mode) {
                    LOG<LOG_TRACE>() << "Status: auto-check(disabled)" << LOG_ENDL;
                    return STATUS_DISABLED;
                }
            }
        }
        break;

    case ExecuteIgnoreSkip:
    case ExecuteReinstall:
    default:
        break;
    }

    LOG<LOG_TRACE>() << "Status: prompt-user(-1)" << LOG_ENDL;
    return STATUS_PROMPT;
}


bool
AutoUpdater::Once() const
{
    bool autoonce = false;
    if (Config::ReadConfigValue(KEY_AUTOONCE, autoonce)) {
        return autoonce;
    }
    return false;
}


void
AutoUpdater::SetOnce(bool state)
{
    Config::WriteConfigValue(KEY_AUTOONCE, state);
}


bool
AutoUpdater::GetAuto() const
{
    bool autocheck = false;
    Config::ReadConfigValue(KEY_AUTOCHECK, autocheck);
    return autocheck;
}


void
AutoUpdater::SetAuto(bool state)
{
    Config::WriteConfigValue(KEY_AUTOCHECK, state);
    Config::WriteConfigValue(KEY_AUTOLAST, (state ? time(NULL) : 0));
}


void
AutoUpdater::Reset()
{
    for (unsigned i = 0; i < (sizeof(ConfigKeys)/sizeof(ConfigKeys[0])); ++i) {
        Config::DeleteConfigValue(ConfigKeys[i]);
    }
}


void
AutoUpdater::Dump()
{
    for (unsigned i = 0; i < (sizeof(ConfigKeys)/sizeof(ConfigKeys[0])); ++i) {
        std::string value;
        Config::ReadConfigValue(ConfigKeys[i], value);
    }
}


//
//  Determine whether an update is available ...
//      1=yes,0=no,-1=canceled/error
//
int
AutoUpdater::IsAvailable(bool interactive)
{
    Logger::open_instance(AppName(), true);     // logger

    // Retrieve and load manifest, plus optional description
    const std::string app_version = Config::GetAppVersion();
    const std::string feed_url = Config::GetFeedURL();
    if (feed_url.empty()) {
        throw AppException("Host URL not configured.");
    }

    LOG<LOG_INFO>() << "Manifest source <" << feed_url << ">" << LOG_ENDL;
    if (interactive) {
        ProgressStart(NULL, true, "Checking for updates");
    }

    int ret = -1;

    try {                                       // guard progress dialog.
        Updater::AutoManifest &d_manifest = d_impl->d_manifest;
        Updater::GitHub github;
        StringDownloadSink manifest;
        std::string manifest_url;
        Download inet;

        // GitHub redirection
        if (github.IsEndpoint(feed_url)) {
            std::string result;

            if (! github.GetLatestRelease(feed_url, inet, DownloadFlags(), result)) {
                d_impl->SetLastError(result);

            } else if (result.empty()) {
                d_impl->SetLastError("GitHub manifest not available");

            } else {
                manifest_url = result;
            }

        } else {
            manifest_url = feed_url;
        }

        // Retrieve manifest
        if (! manifest_url.empty() &&
                inet.get(manifest_url, manifest, DownloadFlags())) {

            if (! inet.completion()) {          // manifest available.
                d_impl->SetLastError("Unable to download manifest");

            } else if (! d_manifest.Load(manifest.data(),
                            Config::GetChannel(), Config::GetOSLabel())) {
                d_impl->SetLastError("Channel/label not available");
                ret = -2;                       // channel not available.

            } else if (! ProgressCancelled()) {
                //
                //  Signature required
                //
                bool suitable = true;

                if (Config::PublicKeyNumber()) {
                    if (d_manifest.attributeEDSignature.empty()) {
                        d_impl->SetLastError("Manifest missing edSignature, contact maintainer.");
                        suitable = false;

                    } else if (! Config::PublicKeyFind(d_manifest.attributeEDKeyVersion)) {
                        d_impl->SetLastError(Updater::format("Manifest: unknown edKeyVersion <%s>, contact maintainer.",
                                    d_manifest.attributeEDKeyVersion.c_str()));
                        suitable = false;
                    }
                }

                //
                //  Check if our version is out of date.
                //
                if (suitable) {
                    LOG<LOG_INFO>() << "current version=" << app_version
                        << ", manifest=" << d_manifest.attributeVersion << LOG_ENDL;

                    if (AutoVersion::Compare(app_version, d_manifest.attributeVersion) >= 0) {
                        LOG<LOG_INFO>() << "same or newer version" << LOG_ENDL;
                        ret = 0;                // same or newer version is already installed.

                    } else {                    // load description, if available.
                        LOG<LOG_INFO>() << "update available" << LOG_ENDL;
                        ret = 1;

                        if (! d_manifest.releaseNotesLink.empty()) {
                            StringDownloadSink content(&d_manifest.releaseNotesContent);

                            if (! inet.get(d_manifest.releaseNotesLink, content) ||
                                    ! inet.completion()) {
                                d_impl->SetLastError("Unable to download release notes");
                                ret = -1;
                            }
                        }
                    }
                }               
            }
        }

    } catch (const std::exception &e) {
        LOG<LOG_ERROR>() << "IsAvailable: exception : " << e.what() << LOG_ENDL;
        d_impl->SetLastError(e.what());
        ProgressStop();
        ret = -1;

    } catch (...) {
        LOG<LOG_ERROR>() << "IsAvailable: unhandled exception" << LOG_ENDL;
        d_impl->SetLastError("unhandled exception");
        ProgressStop();
        ret = -1;
    }

    if (interactive && ProgressStop()) {
        ret = -1;                               // user canceled
    }

    return ret;                                 // channel not available

}


bool
AutoUpdater::IsSkipped()
{
    Updater::AutoManifest &d_manifest = d_impl->d_manifest;
    std::string skipped;

    if (Config::ReadConfigValue(KEY_SKIPVERSION, skipped) && skipped.length()) {

        if (skipped == d_manifest.attributeVersion) {
            time_t skiptime = 0;

            if (Config::ReadConfigValue(KEY_SKIPTIME, skiptime) && skiptime > 0) {
                int skipinterval = 0;           // internal in days (0..180), -1=disables

                (void) Config::ReadConfigValue(KEY_SKIPINTERVAL, skipinterval);
                const time_t expires =          // default, 14 days
                        ((time_t)((skipinterval > 0 && skipinterval <= 180 ? skipinterval : 14))
                                * (60 * 60 * 24) /*day*/);

                if (skipinterval >= 0) {
                    const time_t expiretime = skiptime + expires,
                            nowtime = time(NULL);

                    if (expiretime <= nowtime) {
                        Config::WriteConfigValue(KEY_SKIPVERSION, "");
                        Config::WriteConfigValue(KEY_SKIPTIME, 0);
                        LOG<LOG_TRACE>() << "IsSkipped: skip timer expired, false" << LOG_ENDL;
                        return false;
                    }

                    LOG<LOG_TRACE>() << "IsSkipped: expires in "
                        << ((float)(expiretime - nowtime))/(60 * 60 * 24) << " days, true" << LOG_ENDL;
                    return true;
                }
            }

            LOG<LOG_TRACE>() << "IsSkipped: true" << LOG_ENDL;
            return true;
        }

        Config::WriteConfigValue(KEY_SKIPVERSION, "");
        Config::WriteConfigValue(KEY_SKIPTIME, 0);
        LOG<LOG_TRACE>() << "IsSkipped: skip version expired, false" << LOG_ENDL;
        return false;
    }

    LOG<LOG_TRACE>() << "IsSkipped: false" << LOG_ENDL;
    return false;
}


bool
AutoUpdater::InstallNow(IInstallNow &updater, bool interactive)
{
    const Updater::AutoManifest &d_manifest = d_impl->d_manifest;
    const std::string &targetName = GetTargetName();
    const int exeDirect = TRUE;                 // TODO: configuration option.

    LOG<LOG_TRACE>() << "Install: downloading <" << d_manifest.attributeURL << ">" << LOG_ENDL;
    LOG<LOG_TRACE>() << "   target <" << targetName << ">" << LOG_ENDL;

                                                // progress and browser requirement.
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (interactive) {
        updater("Downloading update ...");
        ProgressStart(updater.GetParent(), true, "Downloading update ...");
    }

    AutoUpdaterSink filesink(*this, targetName.c_str());
    Download inet;                              // download.

    bool getfile = inet.get(d_manifest.attributeURL, filesink);
    if (getfile) {
        getfile = inet.completion();
    }

    const bool wasCancelled = ProgressCancelled();
    ProgressStop();

    // verify signature.
    bool verified = false;
    if (getfile && !wasCancelled) {             // verify image.

        if (interactive) {
            updater("Verifying installer image ...");
            ProgressStart(updater.GetParent(), true, "Verifying installer ...");
        }

        verified = Verify(targetName);
        ProgressStop();

        if (verified) {                         // execute installer.
            updater("Running installer ...");
            if (exeDirect) {
                char szCommandLine[1024] = {0};
                PROCESS_INFORMATION pi = {0};
                STARTUPINFOA si = {0};

                if (! d_manifest.installerArguments.empty()) {
                    sprintf_s(szCommandLine, sizeof(szCommandLine)-1,
                                "\"%s\" %s", targetName.c_str(), d_manifest.installerArguments.c_str());
                } else {
                    sprintf_s(szCommandLine, sizeof(szCommandLine)-1,
                                "\"%s\"", targetName.c_str());
                }

                si.cb = sizeof(si);
                if (::CreateProcessA(targetName.c_str(), szCommandLine,
                            NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi)) {
                    ::CloseHandle(pi.hThread);
                    ::CloseHandle(pi.hProcess);
                    d_impl->RetainTemp();
                } else {
                    const char *msg = "Unable to execute installer";
                    if (interactive) {
                        updater.message("ERROR - %s", msg);
                    }
                    throw SysException(msg);
                }

            } else {
                if (((INT_PTR) ::ShellExecuteA(NULL, "open", targetName.c_str(),
                            d_manifest.installerArguments.c_str(), d_impl->d_tempdir.c_str(), SW_SHOWNORMAL)) >= 32) {
                    d_impl->RetainTemp();
                } else {
                    if (ERROR_CANCELLED == GetLastError()) {
                        const char *msg = "Unable to execute installer";
                        if (interactive) {
                            updater.message("ERROR - %s", msg);
                        }
                        throw SysException(msg);
                    }
                }
            }
            return true;                        // complete.

        } else {
            const char *msg = "Installer verification failure.";
            if (interactive) {
                updater.message("ERROR - %s", msg);
            }
            d_impl->SetLastError(msg);
        }

    } else if (! getfile) {
        if (wasCancelled) {
            if (interactive) {
                updater("Install canceled.");
            }

        } else {
            const char *msg = "Unable to download installer";
            if (interactive) {
                updater.message("ERROR - %s", msg);
            }
            d_impl->SetLastError(msg);
        }

    } else {
        if (wasCancelled) {
            if (interactive) {
                updater("Install canceled.");
            }
        }
    }

    return false;
}


const Updater::AutoManifest&
AutoUpdater::Manifest() const
{
    return d_impl->d_manifest;
}


const std::string&
AutoUpdater::GetTargetName()
{
    const Updater::AutoManifest &d_manifest = d_impl->d_manifest;
    char temppath[MAX_PATH+1] = {0};
    size_t len = 0;

    // working directory CSIDL_INTERNET_CACHE otherwise TEMP
    temppath[0] = 0;
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_INTERNET_CACHE, NULL, 0, temppath)) &&
            (len = strlen(temppath)) > 0) {
        if (_access(temppath, 0) ||
                0 == (::GetFileAttributesA(temppath) & FILE_ATTRIBUTE_DIRECTORY)) {
            temppath[0] = 0;
        }
    }

    if (!*temppath) {
        DWORD dwRetVal = 0;
        dwRetVal = ::GetTempPathA(sizeof(temppath), temppath);
        if (0 == dwRetVal || dwRetVal > MAX_PATH) {
            throw SysException("Unable to retrieve temporary directory");
        }
        len = dwRetVal;
    }

    if (len && temppath[len - 1] != '\\') {
        temppath[len++] = '\\';
        temppath[len] = '\0';
    }

    // create temporary working directory
    for (;;) {
        std::string tempdir(temppath);
        RPC_CSTR uuidStr = 0;
        UUID uuid = {0};

        // generate UUID
        (void) ::UuidCreate(&uuid);
        (void) ::UuidToStringA(&uuid, &uuidStr);
        tempdir += reinterpret_cast<const char *>(uuidStr);
        ::RpcStringFreeA(&uuidStr);

        // create localised unique directory
        if (::CreateDirectoryA(tempdir.c_str(), NULL)) {
            char tempfile[MAX_PATH];

            // download image
            if (d_manifest.attributeName.length()) {
                sprintf_s(tempfile, sizeof(tempfile), "%s\\%s",
                            tempdir.c_str(), d_manifest.attributeName.c_str());
            } else {
                sprintf_s(tempfile, sizeof(tempfile), "%s\\installer-%s.exe",
                            tempdir.c_str(), d_manifest.attributeVersion.c_str());
            }

            d_impl->d_tempdir = tempdir;
            d_impl->d_tempfile.assign(tempfile);
            return d_impl->d_tempfile;

        } else if (GetLastError() != ERROR_ALREADY_EXISTS) {
            throw SysException("Cannot create temporary directory");
        }
    }
    /*NOTREACHED*/
}


bool
AutoUpdater::Verify(const std::string &filename)
{
    const Updater::AutoManifest &d_manifest = d_impl->d_manifest;
    uint8_t ed25519_signature[ED25519_SIGNATURE_LENGTH] = {0};
    const void *ed25519_public_key = NULL;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD fileSize;
    HANDLE hFile;

    LOG<LOG_TRACE>() << "Verify: target image <" << filename << ">" << LOG_ENDL;

    // Signature
    if (Config::PublicKeyNumber()) {
        unsigned key_type = 0;
        size_t key_length = 0;

        ed25519_public_key = 
                Config::PublicKeyFind(d_manifest.attributeEDKeyVersion, key_type, key_length);
        if (ed25519_public_key == NULL || key_type != 0x01 /*ed25519*/) {
            throw AppException(Updater::format("Verify: unknown key-version <%s>", 
                        d_manifest.attributeEDKeyVersion.c_str()));
        }

        if (key_length != ED25519_PUBLIC_LENGTH) {
            throw AppException(Updater::format("Verify: invalid public key length, key-version <%s>",
                    d_manifest.attributeEDKeyVersion.c_str()));
        }

        const std::string &edSignature = d_manifest.attributeEDSignature;
        if (edSignature.empty()) {
            throw AppException(Updater::format("Verify: edSignature missing for key-version <%s>",
                        d_manifest.attributeEDKeyVersion.c_str()));
        }

        const size_t signature_length =
                Updater::Base64::decode(edSignature.c_str(), edSignature.size(), ed25519_signature, sizeof(ed25519_signature));
        if (signature_length != ED25519_SIGNATURE_LENGTH) {
            throw AppException(Updater::format("Verify: invalid signature length, key-version <%s>",
                        d_manifest.attributeEDKeyVersion.c_str()));
        }
    }

    // Open source
    if (INVALID_HANDLE_VALUE == (hFile = CreateFileA(filename.c_str(),
                    GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL))) {
        throw SysException("Unable to open temporary file.");
    }

    fileSize = GetFileSize(hFile, NULL);
    LOG<LOG_TRACE>() << "Verify: target-size=" << fileSize << LOG_ENDL;

    if (0 == fileSize) {
        LOG<LOG_WARN>() << "target-length incorrect (empty) and " << d_manifest.attributeLength << ")" << LOG_ENDL;
        CloseHandle(hFile);
        return false;

    } else if (fileSize != static_cast<DWORD>(strtol(d_manifest.attributeLength.c_str(), NULL, 0))) {
        LOG<LOG_WARN>() << "target-length incorrect (" << fileSize
                << " and " << d_manifest.attributeLength << ")" << LOG_ENDL;
        CloseHandle(hFile);
        return false;
    }

    // Crypt provider handle
    const int hashType =
        (d_manifest.attributeSHASignature.length() ? CALG_SHA : CALG_MD5);

    if (! CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ||
            ! CryptCreateHash(hProv, hashType, 0, 0, &hHash)) {
        DWORD dwStatus = GetLastError();
        if (hProv) CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);
        throw SysException(dwStatus, "CryptAcquireContext failed.");
    }

    // Calculate hash
    int ed22519_verification = 1;               // ed22519 verify result.
    std::string hash;                           // MD5/SHA derived hash.

    {
#define IOBUFFER_SIZE (64 * 1024)
        void *ed22519_context = NULL;
        const char *dwMessage = NULL;
        DWORD dwStatus = 0;
        BYTE *ioBuffer;

        if (NULL == (ioBuffer = static_cast<BYTE *>(malloc(IOBUFFER_SIZE)))) {
            throw SysException(ERROR_NOT_ENOUGH_MEMORY, "Memory allocation.");
        }

        if (ed25519_public_key != NULL) {       // ed22519 signature
            ed22519_context = ed25519_verify_init(ed25519_signature, static_cast<const uint8_t *>(ed25519_public_key));
            ed22519_verification = -1;
        }

        while (1) {
            DWORD ioSize = 0;

            if (! ::ReadFile(hFile, ioBuffer, IOBUFFER_SIZE, &ioSize, NULL)) {
                dwMessage = "Unable to read working file.";
                dwStatus = GetLastError();
                break;
            }

            if (ioSize == 0) {
                break;
            }

            if (! CryptHashData(hHash, ioBuffer, ioSize, 0)) {
                dwMessage = "CryptHashData failed.";
                dwStatus = GetLastError();
                break;
            }

            ed25519_verify_update(ed22519_context, ioBuffer, ioSize);
        }

        CloseHandle(hFile);
        free(ioBuffer);

        if (NULL == dwMessage) {
            BYTE hashBuffer[20] = {0};          // 16=MD5,20=SHA
            DWORD hashSize = sizeof(hashBuffer);

            if (CryptGetHashParam(hHash, HP_HASHVAL, hashBuffer, &hashSize, 0)) {
                hash = Updater::Hex::to_string(hashBuffer, hashSize);
            }
        }

        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);

        if (ed22519_context) {                  // ed22519 signature
            ed22519_verification = ed25519_verify_final(ed22519_context);
            ed22519_context = NULL;
        }

        if (dwMessage) {
            throw SysException(dwStatus, dwMessage);
        }
    }

    // Hash/sign comparisons
    LOG<LOG_TRACE>() << "Verify: target-hash=<" << hash << ">" << LOG_ENDL;

    if ((hashType == CALG_SHA && hash == d_manifest.attributeSHASignature) ||
            (hashType == CALG_MD5 && hash == d_manifest.attributeMD5Signature)) {

        if (ed25519_public_key) {
            if (ed22519_verification != 1) {
                LOG<LOG_WARN>() << "ed25519-verify failed" << LOG_ENDL;
                return false;
            }
            LOG<LOG_TRACE>() << "Verify: target-signature=<ed25519-verified>" << LOG_ENDL;
        }
        return true;
    }

    LOG<LOG_WARN>() << "target-hash incorrect" << LOG_ENDL;
    return false;
}


void
AutoUpdater::InstallLater()
{
    Config::WriteConfigValue(KEY_AUTOLAST, time(NULL));
    Config::WriteConfigValue(KEY_SKIPVERSION, "");
    Config::WriteConfigValue(KEY_SKIPTIME, 0);
}


void
AutoUpdater::InstallSkip()
{
    const Updater::AutoManifest &d_manifest = d_impl->d_manifest;
    Config::WriteConfigValue(KEY_SKIPVERSION, d_manifest.attributeVersion.c_str());
    Config::WriteConfigValue(KEY_SKIPTIME, time(NULL));
}


enum PromptResponse
AutoUpdater::PromptDialog()
{
    if (IAutoUpdaterUI *dialog = d_impl->GetDialog()) {
        return dialog->PromptDialog(*this);
    }
    return PROMPT_AUTO;
}


int
AutoUpdater::InstallDialog()
{
    if (IAutoUpdaterUI *dialog = d_impl->GetDialog()) {
        return dialog->InstallDialog(*this);
    }
    return -1;
}


void
AutoUpdater::UptoDateDialog()
{
    if (IAutoUpdaterUI *dialog = d_impl->GetDialog()) {
        dialog->UptoDateDialog(*this);
    }
}


void
AutoUpdater::ProgressStart(HWND parent, bool indeterminate, const char *msg)
{
    if (IAutoUpdaterUI *dialog = d_impl->GetDialog()) {
        dialog->ProgressStart(*this, parent, indeterminate, msg);
    }
}


void
AutoUpdater::ProgressUpdate(int completed, int total)
{
    if (IAutoUpdaterUI *dialog = d_impl->GetDialog()) {
        dialog->ProgressUpdate(completed, total);
    }
}


bool
AutoUpdater::ProgressCancelled()
{
    if (IAutoUpdaterUI *dialog = d_impl->GetDialog()) {
        return dialog->ProgressCancelled();
    }
    return true;
}


bool
AutoUpdater::ProgressStop()
{
    if (IAutoUpdaterUI *dialog = d_impl->GetDialog()) {
        return dialog->ProgressStop();
    }
    return true;
}

//end
