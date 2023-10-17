#ifndef AUTODIALOG_H_INCLUDED
#define AUTODIALOG_H_INCLUDED
//  $Id: AutoDialog.h,v 1.9 2023/10/17 12:33:57 cvsuser Exp $
//
//  AutoUpdater: dialog interface.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2023, Adam Young
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
#include <assert.h>

#include "IAutoUpdaterUI.h"
#include "AutoUpdater.h"
#include "AutoString.h"

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


/////////////////////////////////////////////////////////////////////////////////////////
//  AutoDialogUI
//

class AutoDialogUI : public IAutoUpdaterUI {
public:
    AutoDialogUI();
    virtual ~AutoDialogUI();

    virtual PromptResponse PromptDialog(AutoUpdater &owner);
    virtual int     InstallDialog(AutoUpdater &owner);
    virtual void    UptoDateDialog(AutoUpdater &owner);

    virtual void    WarningMessage(const char *message);
    virtual void    ErrorMessage(const char *message);

    virtual void    ProgressStart(AutoUpdater &owner, HWND parent, bool indeterminate = false, const char *msg = 0);
    virtual void    ProgressUpdate(int percentage, int total = 0);
    virtual bool    ProgressCancelled();
    virtual bool    ProgressStop();

private:
    void *          progress_;                  // progress implementation.
};


/////////////////////////////////////////////////////////////////////////////////////////
//  ASize
//

class ASize : public SIZE {
public:
    ASize()                                     { cx = 0, cy = 0; }
    ASize(int _cx, int _cy)                     { cx = _cx, cy = _cy; }
    ASize(SIZE size)                            { *this = size; }
};


/////////////////////////////////////////////////////////////////////////////////////////
//  APoint
//

class APoint : public POINT {
public:
    APoint()                                    { x = 0; y = 0; }
    APoint(int X, int Y)                        { x = X; y = Y; }
};


/////////////////////////////////////////////////////////////////////////////////////////
//  ARect
//

class ARect : public RECT {
public:
    ARect()                                     { left = top = right = bottom = 0; }
    ARect(int l, int t, int r, int b)           { left = l; top = t; right = r; bottom = b; }

    void SetRect(int l, int t, int r, int b)    { left = l; top = t; right = r; bottom = b; }

    BOOL OffsetRect(int dx, int dy)             { return ::OffsetRect(this, dx, dy); }
    BOOL OffsetRect(POINT pt)                   { return ::OffsetRect(this, pt.x, pt.y); }
    BOOL OffsetRect(SIZE size)                  { return ::OffsetRect(this, size.cx, size.cy); }

    int Height() const                          { return bottom - top; }
    int Width() const                           { return right - left; }
    ASize Size()                                { return ASize(right - left, bottom - top); }
    POINT CenterPoint() const                   { return APoint((left + right) / 2, (top + bottom) / 2); }
    POINT TopLeft() const                       { return APoint(left, top); }
    POINT BottomRight() const                   { return APoint(right, bottom); }
};


/////////////////////////////////////////////////////////////////////////////////////////
//  AFont
//

class AFont {
    AFont(const AFont &);
    AFont& operator=(const AFont &);

public:
    AFont();
    AFont(int height, const char *name, HWND hWnd = 0);
    AFont(int height, const wchar_t *name, HWND hWnd = 0);
    ~AFont();

    bool CreatePointFont(int height, const char *name, HWND hWnd = 0);
    bool CreatePointFont(int height, const wchar_t *name, HWND hWnd = 0);
    bool SetWindowFont(HWND hWnd);
    bool SetDlgItemFont(HWND hWnd, int nItem);

private:
    HFONT           font_;
};


/////////////////////////////////////////////////////////////////////////////////////////
//  AutoDialog
//

class AutoDialog {
    AutoDialog(const AutoDialog &);
    AutoDialog& operator=(const AutoDialog &);

public:
    AutoDialog(int idd, AutoUpdater &owner, HWND hParent = 0);
    virtual ~AutoDialog();

public:
    INT_PTR DoModal();
    void EndDialog(UINT return_code);

    HWND GetSafeHwnd() const {
        return (this ? d_hWnd : 0);
    }

    BOOL IsWindow() const {
        return ::IsWindow(d_hWnd);
    }

    BOOL IsIconic() const {
        assert(IsWindow());
        return ::IsIconic(d_hWnd);
    }

    void SetIcon(HICON hIcon, bool big) {
        assert(IsWindow());
        ::SendMessage(d_hWnd, WM_SETICON, big ? ICON_BIG : ICON_SMALL, (LPARAM)hIcon);
    }

    void GetClientRect(RECT &rect) {
        assert(IsWindow());
        ::GetClientRect(d_hWnd, &rect);
    }

    BOOL ScreenToClient(POINT &point) const {
        assert(IsWindow());
        return ::ScreenToClient(d_hWnd, &point);
    }

    BOOL ScreenToClient(RECT& rect) const {
        assert(IsWindow());
        return (::MapWindowPoints(0, d_hWnd, (LPPOINT)&rect, 2) != 0);
    }

    DWORD GetStyle() const {
        assert(IsWindow());
        return (d_hWnd ? ::GetWindowLong(d_hWnd, GWL_EXSTYLE) : 0);
    }

    void GetDlgItem(int item, HWND *pHWnd) {
        assert(IsWindow());
        *pHWnd = ::GetDlgItem(d_hWnd, item);
    }

    LRESULT SendMessageA(UINT Msg, WPARAM wParam, LPARAM lParam) {
        assert(IsWindow());
        return ::SendMessageA(d_hWnd, Msg, wParam, lParam);
    }

    LRESULT SendMessageW(UINT Msg, WPARAM wParam, LPARAM lParam) {
        assert(IsWindow());
        return ::SendMessageW(d_hWnd, Msg, wParam, lParam);
    }

protected:
    static INT_PTR CALLBACK dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    virtual BOOL    OnInitDialog();
    virtual BOOL    OnGetMinMaxInfo(MINMAXINFO *mmi);
    virtual HCURSOR OnQueryDragIcon();
    virtual BOOL    OnCommand(WPARAM wParam, LPARAM lParam);
    virtual void    OnSysCommand(WPARAM wParam, LPARAM lParam);
    virtual BOOL    OnCtlColorEdit(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnCtlColorStatic(WPARAM wParam, LPARAM lParam);
    virtual void    OnPaint();
    virtual void    OnSize(UINT type, int cx, int cy);
    virtual void    OnClose();
    virtual void    OnDestroy();

    virtual void    OnCANCEL();
    virtual void    OnOK();

protected:
    const int       d_idd;
    AutoUpdater &   d_owner;
    HWND            d_hWnd;
    HWND            d_hParentWnd;
};


/////////////////////////////////////////////////////////////////////////////////////////
//  AButton
//

class AButton {
    AButton(const AButton &);
    AButton& operator=(const AButton &);

public:
    AButton(int idd, AutoDialog &parent) :
        idd_(idd), parent_(&parent), hParent_(0) {
    }
    AButton(int idd, HWND hParent = 0) :
        idd_(idd), parent_(0), hParent_(hParent) {
    }
    void SetParentHandle(HWND hParent) {
        hParent_ = hParent;
    }
    HWND GetParentHandle() const {
        if (0 == hParent_ && parent_) hParent_ = parent_->GetSafeHwnd();
        return hParent_;
    }
    HWND GetItemHandle() const {
        return ::GetDlgItem(GetParentHandle(), idd_);
    }
    void ShowWindow(int state) {
        if (HWND hDlgHnd = GetItemHandle()) {
            ::ShowWindow(hDlgHnd, state);
        }
    }
    void EnableWindow(bool state) {
        ShowWindow(state ? SW_NORMAL : SW_SHOW);
    }
    bool IsWindowEnabled() const {
        return ::IsWindowEnabled(GetItemHandle()) ? true : false;
    }
    unsigned GetCheck() const {
        // BST_CHECKED, BST_INDETERMINATE or BST_UNCHECKED
        return ::IsDlgButtonChecked(GetParentHandle(), idd_);
    }
    bool SetCheck(unsigned state) {
        // BST_CHECKED, BST_INDETERMINATE or BST_UNCHECKED
        return ::CheckDlgButton(GetParentHandle(), idd_, state) ? true : false;
    }
    const int idd_;
    AutoDialog *parent_;
    mutable HWND hParent_;
};


/////////////////////////////////////////////////////////////////////////////////////////
//  AEdit
//

class AEdit {
    AEdit(const AEdit &);
    AEdit& operator=(const AEdit &);

public:
    AEdit(int idd, AutoDialog &parent) :
        idd_(idd), parent_(&parent), hParent_(0) {
    }
    AEdit(int idd, HWND hParent = 0) :
        idd_(idd), parent_(0), hParent_(hParent) {
    }
    void SetParentHandle(HWND hParent) {
        hParent_ = hParent;
    }
    HWND GetParentHandle() const {
        if (0 == hParent_ && parent_) hParent_ = parent_->GetSafeHwnd();
        return hParent_;
    }
    void ShowWindow(int state) {
        if (HWND hParent = GetParentHandle()) {
            ::ShowWindow(::GetDlgItem(hParent, idd_), state);
        }
    }
    void SetFont(AFont &font) {
        if (HWND hParent = GetParentHandle()) {
            font.SetDlgItemFont(GetParentHandle(), idd_);
        }
    }
    void SetFont(AFont *font) {
        if (font) AEdit::SetFont(*font);
    }
    void SetText(const AString &s) {
        if (HWND hParent = GetParentHandle()) {
            if (!s.empty()) ::SetDlgItemTextW(hParent_, idd_, s.c_str());
        }
    }
    void SetText(const char *s) {
        if (HWND hParent = GetParentHandle()) {
            if (s) ::SetDlgItemTextA(hParent_, idd_, s);
        }
    }
    void SetText(const wchar_t *s) {
        if (HWND hParent = GetParentHandle()) {
            if (s) ::SetDlgItemTextW(hParent, idd_, s);
        }
    }
    void SetWindowTextA(const char *s) { 
        AEdit::SetText(s); 
    }
    void SetWindowTextW(const wchar_t *s) {
        AEdit::SetText(s);
    }
    const int idd_;
    AutoDialog *parent_;
    mutable HWND hParent_;
};

#endif //AUTODIALOG_H_INCLUDED
