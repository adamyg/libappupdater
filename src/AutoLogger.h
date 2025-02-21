#ifndef AUTOLOGGER_H_INCLUDED
#define AUTOLOGGER_H_INCLUDED
//  $Id: AutoLogger.h,v 1.17 2025/02/21 19:03:23 cvsuser Exp $
//
//  AutoUpdater: logger.
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

#include <fstream>
#if defined(__WATCOMC__)
#include <strstream>
#include <iostream>
#endif

#if !defined(__WATCOMC__)
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4244)
#pragma warning(disable:4355)
#endif
#include "BufferStream.hpp"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif

#include "AutoThread.h"

namespace Updater {

enum LogLevel {
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_TRACE,
    LOG_DEBUG
};

class Logger {
private:
    Logger(const Logger&);
    Logger& operator=(const Logger&);

private:
    Logger();
    virtual ~Logger();

public:
    /**
      * newInstance will return the create or recreate the current logger instance,
      * so it can be used to log messages.
      * @return pointer to Logger based class.
      */
    static Logger *
    open_instance(const char *application = 0, bool append = false);

    /**
      * getInstance will return the most recent Logger based class that was created
      * so it can be used to log messages.
      * @return pointer to Logger based class.
      */
    static Logger *
    get_instance();

    /**
      * release the current logger instance.
      */
    static void
    release_instance();

    /**
      * set the logger output basepath.
      */
    static void
    SetBasePath(const char *baseapth);

    /**
      * set the logger output name.
      */
    static void
    SetName(const char *name);

    /**
      * Set the diagnostics level filter.
      */
    void
    SetLevel(LogLevel level);

    /**
      * Retrieve the current diagnostics level filter.
      */
    LogLevel
    GetLevel() const;

    /**
      * Enable/diable diagnostics to stdout, in addition to the active log stream.
      */
    void
    SetStdout(bool val);

    /**
      * Diagnostics stream retrieval, flushing any pending output.
      */
    std::ostream&
    Get(LogLevel level = LOG_INFO);

    /**
      * Flush any pending diagnostics output.
      */
    void
    Flush();

private:
    /**
      * Open/reopen the logger stream.
      */
    void
    OpenFile(const char *application, bool append);

    /**
      * Close the underlying logger stream.
      */
    void
    CloseFile();

    /**
      * Current timestamp.
      */
    const char *
    Timestamp(char *buffer, size_t buflen);

    /**
      * Level to string.
      */
    const char *
    Level(char *buffer, size_t buflen, LogLevel level);

private:
    /**
      * Thread specific stream.
      */
    struct thread_instance {
#if defined(__WATCOMC__)
        thread_instance() : text(buffer, sizeof(buffer)-2/*nl+nul*/) {
        }
        std::ostrstream text;
#else
        thread_instance() : text(buffer, sizeof(buffer)-2/*nl+nul*/) {
        }
        boost::interprocess::obufferstream text;
#endif
        char buffer[4 * 1024];
        LogLevel textlevel;
    };

    static void thread_instance_destroy(void *ptr);
    struct thread_instance& instance();

private:
    static CriticalSection lock_;
    static Logger *global_instance_;
    static char log_path_[MAX_PATH];
    static char log_name_[MAX_PATH];
    LogLevel level_;
    bool stdout_;
    std::ofstream file_;
};
  

template<enum LogLevel level>
std::ostream& LOG() {
    return Logger::get_instance()->Get(level);
}

#if defined(__WATCOMC__)
#define LOG_ENDL '\n'; Logger::get_instance()->Flush()
#else
inline std::ostream&
LOG_ENDL(std::ostream& out) {
    out << '\n'; Logger::get_instance()->Flush();
    return out;
}
#endif

// MFC/DLL Thread local storage
typedef void (*TlsDestroy_t)(void *);

bool  LoggerTlsSetValue(void *data, void (*destroy)(void *));
void *LoggerTlsGetValue(void);

}   // namespace Updater

#endif  //AUTOLOGGER_H_INCLUDED


