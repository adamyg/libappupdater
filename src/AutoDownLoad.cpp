//  $Id: AutoDownLoad.cpp,v 1.13 2021/08/14 05:23:47 cvsuser Exp $
//
//  AutoUpdater: download/inet functionality.
//
//  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2021 Adam Young
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
#include <cassert>

#include "AutoString.h"
#include "AutoConfig.h"
#include "AutoDownload.h"
#include "AutoError.h"

#include <wininet.h>
#pragma comment(lib, "Wininet.lib")

// supported on Windows 10, version 1507 and later.
#if !defined(INTERNET_OPTION_ENABLE_HTTP_PROTOCOL)
#define INTERNET_OPTION_ENABLE_HTTP_PROTOCOL 148
#endif
#if !defined(HTTP_PROTOCOL_FLAG_HTTP2)
#define HTTP_PROTOCOL_FLAG_HTTP2 0x2
#endif

namespace Updater {

namespace {
class INETHandle {
    INETHandle(INETHandle &rhs);
    INETHandle& operator=(INETHandle &rhs);

public:
    INETHandle(HINTERNET handle = 0) : handle_(handle), callback_(false) { }

    ~INETHandle() {
        close();
    }

    INETHandle& operator=(HINTERNET handle) {
        if (handle != handle_) {
            close(), handle_ = handle;
        }
        return *this;
    }

    operator HINTERNET() const {
        return handle_;
    }

    bool set_callback(INTERNET_STATUS_CALLBACK callback) {
        INTERNET_STATUS_CALLBACK CallbackPointer =
                ::InternetSetStatusCallback(handle_, callback);
        if (INTERNET_INVALID_STATUS_CALLBACK == CallbackPointer) {
            return false;
        }
        callback_ = true;
        return true;
    }

    void close() {
        if (HINTERNET handle = handle_) {
            handle_ = 0;
            if (callback_) { //unhook callback, stop closing notification.
                ::InternetSetStatusCallback(handle, NULL);
                callback_ = false;
            }
            ::InternetCloseHandle(handle);
        }
    }

private:
    HINTERNET handle_;
    bool callback_;
};


struct FileDownloadSink : public IDownloadSink {
    FileDownloadSink(const char *filename = NULL) :
        filename_(filename?filename:""), filesize_(-1), hfile_(INVALID_HANDLE_VALUE) {
    }

    ~FileDownloadSink() {
        if (INVALID_HANDLE_VALUE != hfile_) {
            ::CloseHandle(hfile_);
        }
    }

    virtual void set_size(size_t size) {
        filesize_ = size;
    }

    virtual bool open() {
        if (INVALID_HANDLE_VALUE == hfile_) {
            hfile_ = ::CreateFileA(filename_.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        }
        return (INVALID_HANDLE_VALUE != hfile_);
    }

    virtual void append(const void *data, size_t len) {
        DWORD dwWriteSize = (DWORD)len, dwWriteNum = 0;
        if (! ::WriteFile(hfile_, data, dwWriteSize, &dwWriteNum, NULL) ||
                    dwWriteSize != dwWriteNum) {
            throw SysException();
        }
    }

    std::string filename_;
    size_t filesize_;
    HANDLE hfile_;
};

}   //namespace anon


class DownloadContext {
public:
    DownloadContext(Download &owner, const std::string &url, IDownloadSink &sink, unsigned flags);
    DownloadContext(Download &owner, const std::string &url, const char *filename, unsigned flags);

    bool start();
    bool completion(bool pump = true);
    void shutdown();
    void release();

private:
    ~DownloadContext();
    bool execute();

private:
    static unsigned int __cdecl threadproc(void *param);
    static VOID CALLBACK callback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus,
            LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);

public:
    Download &owner;
    const std::string url;
    FileDownloadSink file_sink;
    IDownloadSink &sink;
    const unsigned flags;
    CriticalSection lock;
    unsigned references;
    INETHandle session_handle;
    INETHandle request_handle;
    HANDLE callback_trigger;
    HANDLE completion_trigger;
    bool success;
};


/////////////////////////////////////////////////////////////////////////////////////////
//  Download
//

Download::Download() :
    context_(NULL), connect_timeout_(-1), response_timeout_(-1), enable_login_(false)
{
}


Download::~Download()
{
    if (context_) {
        context_->shutdown();
    }
}


bool
Download::get(const std::string &url, IDownloadSink &sink, unsigned flags /*= 0*/)
{
    if (! context_) {
        DownloadContext *context = new DownloadContext(*this, url, sink, flags);
        if (context->start()) {
            context_ = context;
            return true;
        }
        context->release();
    }
    return false;
}


bool
Download::get(const std::string &url, const char *filename, unsigned flags /*= 0*/)
{
    if (! context_) {
        DownloadContext *context = new DownloadContext(*this, url, filename, flags);
        if (context->start()) {
            context_ = context;
            return true;
        }
        context->release();
    }
    return false;
}


bool
Download::completion(bool pump /*true*/)
{
    if (context_) {
        DownloadContext *context = context_;
        context_ = NULL;
        return context->completion(pump);
    }
    return false;
}


void
Download::cancel()
{
    if (context_) {
        DownloadContext *context = context_;
        context_ = NULL;
        context->shutdown();
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
//  DownloadContext
//

DownloadContext::DownloadContext(Download &owner, const std::string &url, IDownloadSink &sink, unsigned flags) :
        owner(owner), url(url), file_sink(), sink(sink), flags(flags),
    references(1), callback_trigger(INVALID_HANDLE_VALUE), completion_trigger(INVALID_HANDLE_VALUE),
    success(false)
{
}


DownloadContext::DownloadContext(Download &owner, const std::string &url, const char *filename, unsigned flags) :
        owner(owner), url(url), file_sink(filename), sink(file_sink), flags(flags),
    references(1), callback_trigger(INVALID_HANDLE_VALUE), completion_trigger(INVALID_HANDLE_VALUE),
    success(false)
{
}


bool
DownloadContext::start()
{
    assert(INVALID_HANDLE_VALUE == completion_trigger);
    assert(INVALID_HANDLE_VALUE == callback_trigger);
    if (NULL != (completion_trigger = ::CreateEventW(NULL, FALSE, FALSE, NULL)) &&
                NULL != (callback_trigger = ::CreateEventW(NULL, FALSE, FALSE, NULL))) {

        Updater::Thread *thread;
        if (NULL != (thread = Updater::Thread::Begin(threadproc, (void *)this))) {
            ++references;
            thread->SetAutoDelete();
            thread->ResumeThread();
            return true;
        }

        ::CloseHandle(callback_trigger);
        ::CloseHandle(completion_trigger);
        callback_trigger = INVALID_HANDLE_VALUE;
        completion_trigger = INVALID_HANDLE_VALUE;
    }
    return false;
}


bool
DownloadContext::completion(bool pump)
{
    HANDLE t_trigger;
    MSG msg = {0};

    assert(references);

    {   CriticalSection::Guard guard(lock);
        assert(INVALID_HANDLE_VALUE != completion_trigger);
        if (INVALID_HANDLE_VALUE == (t_trigger = completion_trigger)) {
            return false;
        }
    }

    for (bool done = false; !done;) {       // message pump loop.
        if (MsgWaitForMultipleObjects(1, &t_trigger,
                    FALSE, 100, QS_ALLEVENTS) == WAIT_OBJECT_0) {
            CriticalSection::Guard guard(lock);
            if (INVALID_HANDLE_VALUE != completion_trigger) {
                assert(t_trigger == completion_trigger);
                completion_trigger = INVALID_HANDLE_VALUE;
                ::CloseHandle(t_trigger);
            }
            done = true;                    // trigger complete or quit.
            break;
        }

        if (pump) {
            while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
                if (WM_QUIT == msg.message) {
                    done = true;
                    break;
                } else if (::GetMessage(&msg, NULL, 0, 0) > 0) {
                    if (! IsDialogMessage(NULL, &msg)) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
        }
    }

    const bool t_success = success;
    release();
    return t_success;
}


void
DownloadContext::shutdown()
{
    CriticalSection::Guard guard(lock);
    if (session_handle) {
        if (request_handle) {
            request_handle.close();
            ::SetEvent(callback_trigger);
        }
        session_handle.close();
        ::CloseHandle(callback_trigger);
        callback_trigger = INVALID_HANDLE_VALUE;
    }
}


void
DownloadContext::release()
{
    unsigned t_references;
    {   CriticalSection::Guard guard(lock);
        assert(references);
        t_references = --references;
    }
    if (0 == t_references) {
        delete this;
    }
}


DownloadContext::~DownloadContext()
{
    assert(0 == references);
    if (INVALID_HANDLE_VALUE != callback_trigger) {
        ::CloseHandle(callback_trigger);
    }
    if (INVALID_HANDLE_VALUE != completion_trigger) {
        ::CloseHandle(completion_trigger);
    }
}


//static/private
unsigned int __cdecl
DownloadContext::threadproc(void *param)
{
    DownloadContext *self = reinterpret_cast<DownloadContext *>(param);
    assert(self && self->references);
    if (self) {
        try {
            self->success = self->execute();
#if defined(_DEBUG)
            ::Sleep(5 * 1000);
#endif
            LOG<LOG_INFO>() << "Download: <" << self->url << "> complete" << LOG_ENDL;
        } catch (std::exception &e) {
            LOG<LOG_ERROR>() << "Download: <" << self->url << "> exception : " << e.what() << LOG_ENDL;
        } catch (...) {
            LOG<LOG_ERROR>() << "Download: <" << self->url << "> unhandled exception" << LOG_ENDL;
        }

        if (INVALID_HANDLE_VALUE != self->completion_trigger) {
            ::SetEvent(self->completion_trigger);
        }

        self->shutdown();
        self->release();
    }
    return 0;
}


//private
bool
DownloadContext::execute()
{
    //  local resources
    //  -------------------------------------------------

    if (0 == url.find("\\\\.\\") || 0 == url.find("file:///")) {
        //
        //  allows local paths, in two forms:
        //      \\.\C:\appname.manifest
        //      file:///X:/appname.manifest
        //
        const char *source = url.c_str();
        const char *filename = ('f' == *source ? source + 8 : source);
        HANDLE fd;

        LOG<LOG_DEBUG>() << "Download: local file=" << filename << LOG_ENDL;

        fd = ::CreateFileA(filename, GENERIC_READ,
                    FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == fd) {
            std::string message;
            message = "opening <", message += source, message += ">";
            throw SysException(message);
        }

        size_t result = 0;
        sink.set_size((size_t) GetFileSize(fd, NULL));

        if (sink.open()) {
            char buffer[8 * 1024];
            for (;;) {
                DWORD read = 0;

                if (! ::ReadFile(fd, buffer, sizeof(buffer), &read, NULL)) {
                    std::string message;
                    message = "reading <", message += filename, message += ">";
                    ::CloseHandle(fd);
                    throw SysException(message);
                }

                if (0 == read) break; // EOF
                sink.append(buffer, read);
                result += read;
            }
        }

        LOG<LOG_DEBUG>() << "Download: size=" << result << LOG_ENDL;
        ::CloseHandle(fd);
        return true;
    }

    // remote resources
    // ---------------------------------------------

    // extract url components
    URL_COMPONENTSA uc = { sizeof(uc) };

    uc.dwHostNameLength = 1;
    uc.dwUrlPathLength = 1;
    uc.dwExtraInfoLength = 1;
    if (! InternetCrackUrlA(url.c_str(), 0, 0, &uc)) {
        throw SysException("Invalid URL");
    }

    if (NULL == uc.lpszHostName || 0 == uc.dwHostNameLength) {
        throw AppException("Invalid URL, missing hostname");

    } else if (NULL == uc.lpszUrlPath || 0 == uc.dwUrlPathLength) {
        throw AppException("Invalid URL, missing url_path");

    } else if (INTERNET_SCHEME_HTTP == uc.nScheme) {
        throw AppException("Invalid URL, insecure transport");
    }

    // open session
    const DWORD dwFlags =
            INTERNET_FLAG_RELOAD | // Forces a download of the requested file from the origin server, not from the cache.
            INTERNET_FLAG_NO_CACHE_WRITE | // Does not add the returned entity to the cache.
            (uc.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_FLAG_SECURE : 0) | // Secure transaction semantics.
            (Download::NOCACHED & flags ? INTERNET_FLAG_PRAGMA_NOCACHE : 0);

    const std::string user_agent =
        Config::GetAppName() + "/" +  Config::GetAppVersion() + " AutoUpdate";
    session_handle =
        InternetOpenA(user_agent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL,  0);
    if (! session_handle) {
        throw SysException("Opening internet connection");
    }

    DWORD http2_option = HTTP_PROTOCOL_FLAG_HTTP2;
    ::InternetSetOption(session_handle, INTERNET_OPTION_ENABLE_HTTP_PROTOCOL, &http2_option, sizeof(http2_option));

    if (owner.connect_timeout_ >= 0) {
        DWORD dw = (owner.connect_timeout_ == 0 ? 0xFFFFFFFF : owner.connect_timeout_ * 1000);
        ::InternetSetOption(session_handle, INTERNET_OPTION_CONNECT_TIMEOUT, &dw, sizeof(dw));
    }

    if (owner.response_timeout_ >= 0) {
        DWORD dw = (owner.response_timeout_ == 0 ? 0xFFFFFFFF : owner.response_timeout_ * 1000);
        ::InternetSetOption(session_handle, INTERNET_OPTION_SEND_TIMEOUT, &dw, sizeof(dw));
        ::InternetSetOption(session_handle, INTERNET_OPTION_RECEIVE_TIMEOUT, &dw, sizeof(dw));
    }

    // url canonicalization
    char canonical_url[2000] = {0};
    DWORD nSize = sizeof(canonical_url);
    if (! InternetCanonicalizeUrlA(url.c_str(), canonical_url, &nSize, ICU_BROWSER_MODE)) {
        throw SysException("Invalid URL");
    }

    session_handle.set_callback(callback);

    // request
again:
    request_handle =
        ::InternetOpenUrlA(session_handle, canonical_url, NULL, -1, dwFlags, (DWORD_PTR)this);
    if (! request_handle) {
        if (ERROR_IO_PENDING != GetLastError()) {
            throw SysException("Opening internet connection");
        }
    }

    ::WaitForSingleObject(callback_trigger, 0);

    // status
    DWORD http_res = 0, http_res_len = sizeof(http_res);
    if (! HttpQueryInfo(request_handle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &http_res, &http_res_len, 0)) {
        throw SysException("Reading internet connection");

    } else if (http_res >= 400) {               // error, decode and report
        DWORD http_msg_len = 0;

        if (owner.enable_login_) {
            if (HTTP_STATUS_PROXY_AUTH_REQ == http_res) {
                if (InternetErrorDlg(GetDesktopWindow(), request_handle, ERROR_INTERNET_INCORRECT_PASSWORD,
                        FLAGS_ERROR_UI_FILTER_FOR_ERRORS |FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
                        FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS, NULL) == ERROR_INTERNET_FORCE_RETRY) {
                    goto again;
                }
            }

            if (HTTP_STATUS_DENIED == http_res) {
                if (InternetErrorDlg(GetDesktopWindow(), request_handle, ERROR_INTERNET_INCORRECT_PASSWORD,
                        FLAGS_ERROR_UI_FILTER_FOR_ERRORS |FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
                        FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS, NULL) == ERROR_INTERNET_FORCE_RETRY) {
                    goto again;
                }
            }
        }

        if (! ::HttpQueryInfoA(request_handle, HTTP_QUERY_STATUS_TEXT, NULL, &http_msg_len, 0) &&
                    GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            std::string http_msg;

            http_msg.resize(http_msg_len);
            if (::HttpQueryInfoA(request_handle, HTTP_QUERY_STATUS_TEXT, NULL, &http_msg_len, 0)) {
                throw AppException(http_msg);
            }
        }

        throw AppException("Unable to download component");
    }

    // context size, if available
    DWORD content_length = 0, content_length_len = sizeof(content_length);
    if (::HttpQueryInfo(request_handle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                &content_length, &content_length_len, 0)) {
        // Content-Length: 200
        //  Note: Wont reflect the size of any attachments.
        sink.set_size(content_length);
    }

    // read content
    char buffer[8 * 1024] = {0};

    if (sink.open()) {
        for (;;) {
            DWORD read = 0;
            if (! InternetReadFile(request_handle, buffer, sizeof(buffer), &read)) {
                throw SysException("Reading internet connection");
            }
            if (0 == read) break;               // EOF
            sink.append(buffer, read);
        }
    }

    return true;
}


//static/private
VOID CALLBACK
DownloadContext::callback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus,
        LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
    DownloadContext *self = reinterpret_cast<DownloadContext *>(dwContext);

    assert(1 == self->references || 2 == self->references);
    switch (dwInternetStatus) {
    case INTERNET_STATUS_COOKIE_SENT:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Cookie found and will be sent with request" << LOG_ENDL;
        break;

    case INTERNET_STATUS_COOKIE_RECEIVED:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Cookie Received" << LOG_ENDL;
        break;

    case INTERNET_STATUS_COOKIE_HISTORY: {
            InternetCookieHistory cookieHistory =
                    *((InternetCookieHistory*)lpvStatusInformation);

            LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
                    " Status: Cookie History" << LOG_ENDL;

            if (cookieHistory.fAccepted) {
                LOG<LOG_DEBUG>() << "Download: Cookie Accepted" << LOG_ENDL;
            }
            if (cookieHistory.fLeashed) {
                LOG<LOG_DEBUG>() << "Download: Cookie Leashed" << LOG_ENDL;
            }
            if (cookieHistory.fDowngraded) {
                LOG<LOG_DEBUG>() << "Download: Cookie Downgraded" << LOG_ENDL;
            }
            if (cookieHistory.fRejected) {
                LOG<LOG_DEBUG>() << "Download: Cookie Rejected" << LOG_ENDL;
            }
        }
        break;

    case INTERNET_STATUS_CLOSING_CONNECTION:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Closing Connection" << LOG_ENDL;
        ::SetEvent(self->callback_trigger);
        break;

    case INTERNET_STATUS_CONNECTED_TO_SERVER:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Connected to Server=" <<  ((const char *)lpvStatusInformation) << LOG_ENDL;
        break;

    case INTERNET_STATUS_CONNECTING_TO_SERVER:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Connecting to Server=" << ((const char *)lpvStatusInformation) << LOG_ENDL;
        break;

    case INTERNET_STATUS_CONNECTION_CLOSED:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Connection Closed" << LOG_ENDL;
        break;

    case INTERNET_STATUS_HANDLE_CREATED: {
            const INTERNET_ASYNC_RESULT *res = (const INTERNET_ASYNC_RESULT*)lpvStatusInformation;
            LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
                " Created: Handle created" << LOG_ENDL;
            self->request_handle = (HINTERNET)(res->dwResult);
        }
        break;

    case INTERNET_STATUS_HANDLE_CLOSING:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Handle Closing" << LOG_ENDL;
        break;

    case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Intermediate response" << LOG_ENDL;
        break;

    case INTERNET_STATUS_RECEIVING_RESPONSE:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Receiving Response" << LOG_ENDL;
        break;

    case INTERNET_STATUS_RESPONSE_RECEIVED:
        assert(dwStatusInformationLength == sizeof(DWORD));
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Response received=" << *((LPDWORD)lpvStatusInformation) << " bytes" << LOG_ENDL;
        break;

    case INTERNET_STATUS_REDIRECT:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Redirect=" << to_string((const wchar_t *)lpvStatusInformation) << LOG_ENDL;
        break;

    case INTERNET_STATUS_REQUEST_COMPLETE: {
            const INTERNET_ASYNC_RESULT *res = (const INTERNET_ASYNC_RESULT*)lpvStatusInformation;
            LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
                " Status: Request complete, result=" << res->dwResult << ", error=" << res->dwError << LOG_ENDL;
        }
        break;

    case INTERNET_STATUS_REQUEST_SENT:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Request sent=" << *((LPDWORD)lpvStatusInformation) << " bytes" << LOG_ENDL;
        break;

    case INTERNET_STATUS_DETECTING_PROXY:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Detecting Proxy" << LOG_ENDL;
        break;

    case INTERNET_STATUS_RESOLVING_NAME:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Resolving Name=" << to_string((const wchar_t *)lpvStatusInformation) << LOG_ENDL;
        break;

    case INTERNET_STATUS_NAME_RESOLVED:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Name Resolved=" << ((const char *)lpvStatusInformation) << LOG_ENDL;
        break;

    case INTERNET_STATUS_SENDING_REQUEST:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Sending request" << LOG_ENDL;
        break;

    case INTERNET_STATUS_STATE_CHANGE:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: State Change" << LOG_ENDL;
        break;

    case INTERNET_STATUS_P3P_HEADER:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Received P3P header" << LOG_ENDL;
        break;

    default:
        LOG<LOG_DEBUG>() << "Download: " << (void *)hInternet <<
            " Status: Unknown <" << dwInternetStatus << ">" << LOG_ENDL;
        break;
    }
}

}   // namespace Updater

