//  $Id: AutoLogger.cpp,v 1.18 2022/06/09 08:46:30 cvsuser Exp $
//
//  AutoUpdater: logger.
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

#include <shlobj.h>                             /* SHGetFolderPath */

#include <iostream>
#include <cassert>

#include <io.h>                                 /* _access() */
#include <time.h>

#include "AutoLogger.h"

#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shfolder.lib")

namespace Updater {

CriticalSection Logger::lock_;
Logger *Logger::global_instance_ = 0;
char Logger::log_path_[MAX_PATH] = {0};
char Logger::log_name_[MAX_PATH] = {"autoupdater.log"};

namespace {
static int
PathCheck(char *buffer, size_t buflen, size_t leading, const char *application)
{
    assert(leading < buflen);
    if (leading &&
            (buffer[leading - 1] != '\\' && buffer[leading - 1] != '/')) {
        buffer[leading++] = '\\';
    }
    sprintf_s(buffer + leading, buflen - leading, "%s%s",
        (application ? application : ""), (application ? "\\" : ""));

    if (0 == _access(buffer, 0) &&
            (::GetFileAttributesA(buffer) & FILE_ATTRIBUTE_DIRECTORY)) {
        return TRUE;
    }
    ::CreateDirectoryA(buffer, NULL);
    if (0 == _access(buffer, 0) &&
            (::GetFileAttributesA(buffer) & FILE_ATTRIBUTE_DIRECTORY)) {
        return TRUE;
    }
    return FALSE;
}


static std::string
GetLogPath(const char *application)
{
    char buffer[MAX_PATH];
    size_t len;

    //  CSIDL_APPDATA
    //      The file system directory that serves as a common
    //      repository for application-specific data. A typical path
    //      is C:\Documents and Settings\username\Application Data.
    //
    buffer[0] = 0;
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, buffer)) &&
            (len = strlen(buffer)) > 0) {
        if (! PathCheck(buffer, sizeof(buffer), len, application)) {
            buffer[0] = 0;
        }
    }

    //  CSIDL_COMMON_APPDATA
    //      The file system directory that contains application data for all users. A typical
    //      path is C:\Documents and Settings\All Users\Application Data. This folder is used for
    //      application data that is not user specific
    //
    //      This information will not roam and is available to anyone using the computer.
    //
    if (!*buffer) {
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, buffer)) &&
                (len = strlen(buffer)) > 0) {
            if (! PathCheck(buffer, sizeof(buffer), len, application)) {
                buffer[0] = 0;
            }
        }
    }

    //  Temporary Directory
    //
    if (!*buffer) {
        DWORD ret;
        if (0 == (ret = GetTempPathA(sizeof(buffer), buffer)) > 0 || ret > MAX_PATH) {
            strcpy(buffer, "C:\\");
        }
    }

    return std::string(buffer);
}

}   //namespace anon


Logger *
Logger::open_instance(const char *application, bool append)
{
    CriticalSection::Guard guard(lock_);
    if (0 == global_instance_) global_instance_ = new Logger();
    if (! global_instance_->file_.is_open()) {
        global_instance_->OpenFile(application, append);
    }
    return global_instance_;
}


Logger *
Logger::get_instance()
{
    CriticalSection::Guard guard(lock_);
    if (0 == global_instance_) global_instance_ = new Logger();
    return global_instance_;
}


void
Logger::release_instance()
{
    CriticalSection::Guard guard(lock_);
    delete global_instance_;
    global_instance_ = NULL;
}


Logger::Logger()
    : level_(LOG_DEBUG), stdout_(false)
{
}


Logger::~Logger()
{
    CloseFile();
}


void
Logger::thread_instance_destroy(void *ptr)
{
    if (ptr) {
        Logger::thread_instance *instance =
            reinterpret_cast<Logger::thread_instance *>(ptr);
        delete instance;
    }
}


struct Logger::thread_instance&
Logger::instance()
{
    Logger::thread_instance *instance =
        reinterpret_cast<Logger::thread_instance *>(LoggerTlsGetValue());
    if (NULL == instance) {
        instance = new thread_instance;
        LoggerTlsSetValue(instance, thread_instance_destroy);
    }
    return *instance;
}


std::ostream&
Logger::Get(LogLevel level)
{
    thread_instance& self(instance());
    char buffer[64];

    Flush();
    self.textlevel = level;
    self.text << Timestamp(buffer, sizeof(buffer)) <<  " "
            << Level(buffer, sizeof(buffer), level) << ": ";
    return self.text;
}


void
Logger::Flush()
{
    thread_instance& self(instance());

    if (self.textlevel <= level_) {
#if defined(__WATCOMC__) //strstream
        size_t length = self.text.pcount();
#else
        size_t length = (size_t)self.text.tellp();
#endif
        char *text = self.buffer;

        assert(length < (sizeof(self.buffer) - 2));
        if (length) {
            if (text[length - 1] != '\n') {
                text[length - 1] = '\n', ++length; //newline
            }
            text[length] = 0; //terminate

            {   CriticalSection::Guard guard(lock_);
                if (file_.is_open()) file_ << text;
                if (stdout_) std::cout << text;
            }

#if defined(_DEBUG)
#if defined(__WATCOMC__)
            if (length > 512) { // WATCOM WD[W}, crashes if >1k; truncate
                strcpy(text + 512, " ...\n");
            }
#endif
            ::OutputDebugStringA(text);
#endif
        }
    }
    self.text.clear();
    self.text.seekp(0, std::ios::beg);
}


void
Logger::SetLevel(LogLevel level)
{
    level_ = level;
}


LogLevel
Logger::GetLevel() const
{
    return level_;
}


void
Logger::SetStdout(bool val)
{
    stdout_ = val;
}


void
Logger::SetBasePath(const char *basepath)
{
    if (basepath && *basepath) {
        strncpy(log_path_, basepath, sizeof(log_path_));
    }
}


void
Logger::SetName(const char *name)
{
    if (name && *name) {
        strncpy(log_name_, name, sizeof(log_name_));
    }
}


void
Logger::OpenFile(const char *application, bool append)
{
    std::string t_filename;

    if (log_path_[0] &&
            PathCheck(log_path_, sizeof(log_path_), strlen(log_path_), NULL)) {
        t_filename.assign(log_path_);
    } else {
        t_filename = GetLogPath(application);
    }
    t_filename += log_name_;

    file_.close();

#if defined(__WATCOMC__)
    unsigned mode = std::ios::out;
#else
    std::ios_base::openmode mode = std::ios::out;
#endif
    if (append) mode |= std::ios::app;
    file_.open(t_filename.c_str(), mode);
    if (! file_.is_open()) {
        throw std::runtime_error("LOGGER: Unable to open an output stream");
    }
}


void
Logger::CloseFile()
{
    Flush();
    file_.close();
}


const char *
Logger::Timestamp(char *buffer, size_t buflen)
{
#if defined(__MINGW64_VERSION_MAJOR)
    __time64_t now = _time64(NULL);
#else
    time_t now = time(NULL);
#endif
    struct tm tm = {0};

#if defined(__WATCOMC__)
    _localtime(&now, &tm);
#else
    _localtime64_s(&tm, &now);
#endif
 // strftime(buffer, buflen, "%a %b %e %H:%M:%S %Y", &tm);
    strftime(buffer, buflen, "%c", &tm);
    return buffer;
}


const char *
Logger::Level(char *buffer, size_t buflen, LogLevel level)
{
    switch (level) {
    case LOG_ERROR: return "ERROR";
    case LOG_WARN:  return "WARN ";
    case LOG_INFO:  return "INFO ";
    case LOG_TRACE: return "TRACE";
    case LOG_DEBUG: return "DEBUG";
    default:
        sprintf_s(buffer, buflen, "lvl-%d", level - (LOG_DEBUG - 1));
        break;
    }
    return buffer;
}

}   // namespace Updater

