#ifndef AUTOMISC_H_INCLUDED
#define AUTOMISC_H_INCLUDED
//  $Id: AutoMisc.h,v 1.6 2021/08/14 15:38:10 cvsuser Exp $
//
//  AutoUpdater: misc definitions and functionality
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
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

#include <string>
#include <tchar.h>

    //#define UPDATE_NORMSIZE 100
    //#define UPDATE_BOLDSIZE 110
    //#define UPDATE_NORMFONT _T("Arial")
    //#define UPDATE_BOLDFONT _T("Arial Bold")
    //#define UPDATE_NORMFONT _T("Lucida Sans Unicode")
    //#define UPDATE_BOLDFONT _T("Lucida Sans Unicode Bold")
    //#define UPDATE_NORMFONT _T("Verdana")
    //#define UPDATE_BOLDFONT _T("Verdana Bold")
    //#define UPDATE_NORMFONT _T("Trebuchet MS")
    //#define UPDATE_BOLDFONT _T("Trebuchet MS Bold")
    //#define UPDATE_NORMFONT _T("Tahoma")
    //#define UPDATE_BOLDFONT _T("Tahoma Bold")
    //#define UPDATE_NORMFONT _T("Sans-serif")
    //#define UPDATE_BOLDFONT _T("Sans-serif Bold")

#define UPDATE_NORMSIZE 110
#define UPDATE_BOLDSIZE 125
    //#define UPDATE_NORMFONT _T("Calibri")
    //#define UPDATE_BOLDFONT _T("Calibri Bold")
    //#define UPDATE_NORMFONT _T("Segoe UI")
    //#define UPDATE_BOLDFONT _T("Segoe UI Bold")
#define UPDATE_NORMFONT _T("Cambria")
#define UPDATE_BOLDFONT _T("Cambria Bold")


namespace Updater {

/////////////////////////////////////////////////////////////////////////////////////////
//  CriticalSection object
//

class CriticalSection {
    CriticalSection(const CriticalSection &rsh);
    CriticalSection& operator=(const CriticalSection &rsh);
public:
    class Guard {
        Guard(const Guard &rsh);
        Guard& operator=(const Guard &rsh);
    public:
        Guard(CriticalSection& cs) : m_cs(cs)
            { cs.Enter(); }
        ~Guard()
            { m_cs.Leave(); }

    private:
        CriticalSection& m_cs;
    };

public:
    CriticalSection()
        { ::InitializeCriticalSection(&m_cs); }
    ~CriticalSection()
        { ::DeleteCriticalSection(&m_cs); }

    void Enter()
        { ::EnterCriticalSection(&m_cs); }
    void Leave()
        { ::LeaveCriticalSection(&m_cs); }

private:
    CRITICAL_SECTION m_cs;
};


/////////////////////////////////////////////////////////////////////////////////////////
//  Event
//

class WaitableEvent {
    WaitableEvent(const WaitableEvent &rsh);
    WaitableEvent& operator=(const WaitableEvent &rsh);
public:
    WaitableEvent() : event_(INVALID_HANDLE_VALUE) { }
    ~WaitableEvent() { 
        Close();
    }

    bool Create(bool manual_reset = true) {
        if (INVALID_HANDLE_VALUE == event_ &&
                INVALID_HANDLE_VALUE != (event_ = ::CreateEventW(NULL, manual_reset, FALSE, NULL))) {
            return true;
        }
        return false;
    }

    bool IsOpen() const {
        return (INVALID_HANDLE_VALUE != event_);
    }

    int Wait(unsigned milliseconds = 0) {
        if (INVALID_HANDLE_VALUE != event_) {
            if (::WaitForMultipleObjects(1, &event_, FALSE, milliseconds) == WAIT_OBJECT_0) {
                return 1;
            }
            return 0;
        }
        return -1; //error
    }

    void Trigger() {
        if (INVALID_HANDLE_VALUE != event_) {
            ::SetEvent(event_);
        }
    }

    void Reset() {
        if (INVALID_HANDLE_VALUE != event_) {
            ::ResetEvent(event_);
        }
    }

    void Close() {
        if (INVALID_HANDLE_VALUE != event_) {
            ::CloseHandle(event_);
            event_ = INVALID_HANDLE_VALUE;
        }
    }

private:
    HANDLE event_;
};


/////////////////////////////////////////////////////////////////////////////////////////
//  Thread
//

typedef unsigned int (__cdecl *ThreadFunction)(void *);

class Thread {
    Thread(const Thread &rhs);
    Thread& operator=(const Thread &rhs);

private:
    Thread(ThreadFunction function, void *parameter)
        : handle_(0), function_(function), parameter_(parameter), auto_delete_(false) {
    }

public:
    static Thread *Begin(ThreadFunction function, void *parameter) {
        Thread *self = new(std::nothrow) Thread(function, parameter);
        if (self) {
            HANDLE handle =
                ::CreateThread(NULL, 0, thread_function, self, CREATE_SUSPENDED, NULL);
            if (handle) {
                self->handle_ = handle;
                return self;
            }
        }
        delete self;
        return NULL;
    }

public:
    ~Thread() {
        if (handle_) ::CloseHandle(handle_);
    }
    void SetAutoDelete() { 
        auto_delete_ = true;
    }
    void ResumeThread() {
        ::ResumeThread(handle_);
    }
    bool IsRunning() const { 
        return (::WaitForSingleObject(handle_, 0) == WAIT_TIMEOUT); 
    }

private:
    static DWORD WINAPI thread_function(LPVOID lpParameter) {
        Thread *self = (Thread *)(lpParameter);
        if (self->function_) {
            (self->function_)(self->parameter_);
        }
        if (self->auto_delete_) {
            delete(self);
        }
        return 0;
    }

public:
    HANDLE handle_;
    ThreadFunction function_;
    void *parameter_;
    bool auto_delete_;
};

}   // namespace Updater

#endif  //AUTOMISC_H_INCLUDED
