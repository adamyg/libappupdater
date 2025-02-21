#pragma once
//  $Id: CSimpleBrowser.h,v 1.12 2025/02/21 19:03:23 cvsuser Exp $
//
//  AutoUpdater: Browser widget.
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

#include <string>
#include <tchar.h>

#if defined(_MSC_VER)
#include <comdef.h>
#else
#include <ole2.h>
#endif
#include <Exdisp.h>

#if defined(__WATCOMC__)
#pragma disable_message(146) /* unexpected storage class specifier found */
#endif
#include <Mshtml.h>

#if !defined(WINDOWS_MEAN_AND_LEAN)
#define  WINDOWS_MEAN_AND_LEAN
#endif
#include <Windows.h>

#if defined(_MSC_VER) && (_MSC_VER <= 1600)
#define _Outptr_
#define _Reserved_ 
#define _In_reads_opt_(x)
#endif
#if defined(__WATCOMC__)
#define override
#define _In_reads_opt_(x)
#define _In_opt_
#define _In_z_
#define _In_
#define _Outptr_
#define _Reserved_ 
#define __RPC__deref_opt_in_opt
#define __RPC__deref_out_opt
#define __RPC__in
#define __RPC__in_opt
#define __RPC__in_opt_string
#define __RPC__in_string
#define __RPC__inout
#define __RPC__out
#endif

class CSimpleBrowser :
	public IOleClientSite, public IOleInPlaceSite, public IStorage
{
public:
	CSimpleBrowser();
	virtual ~CSimpleBrowser();

	// Construction/management

	bool CreateBrowser(HWND hParent, const RECT &rect, DWORD dwStyle = 0);
	bool CreateFromControl(HWND hParent, UINT nID, DWORD dwStyle = 0);

	RECT PixelToHiMetric(const RECT& _rc);

	virtual void SetRect(const RECT& _rc);

	// Control

	void GoBack();
	void GoForward();
	void Refresh();

	void Navigate(const std::wstring &url);

	void Navigate(const std::string &url);

	void Navigate(const wchar_t *url);

	// Document

	bool Content(const std::wstring &text);

	IHTMLDocument2 *GetDocument();

	HWND GetControlWindow();

	/////////////////////////////////////////////////////////////////////////////////////////
	//  IUnknown

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
		void**ppvObject) override;

	virtual ULONG STDMETHODCALLTYPE AddRef(void);

	virtual ULONG STDMETHODCALLTYPE Release(void);

	/////////////////////////////////////////////////////////////////////////////////////////
	//  IOleWindow

	virtual HRESULT STDMETHODCALLTYPE GetWindow(
		__RPC__deref_out_opt HWND *phwnd) override;

	virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(
		BOOL fEnterMode) override;

	/////////////////////////////////////////////////////////////////////////////////////////
	//  IOleClientSite

	virtual HRESULT STDMETHODCALLTYPE SaveObject( void);

	virtual HRESULT STDMETHODCALLTYPE GetMoniker(
	    /* [in] */ DWORD dwAssign,
	    /* [in] */ DWORD dwWhichMoniker,
	    /* [out] */ __RPC__deref_out_opt IMoniker **ppmk);

	virtual HRESULT STDMETHODCALLTYPE GetContainer(
	    /* [out] */ __RPC__deref_out_opt IOleContainer **ppContainer);

	virtual HRESULT STDMETHODCALLTYPE ShowObject( void);

	virtual HRESULT STDMETHODCALLTYPE OnShowWindow(
	    /* [in] */ BOOL fShow);

	virtual HRESULT STDMETHODCALLTYPE RequestNewObjectLayout( void);

	/////////////////////////////////////////////////////////////////////////////////////////
	//  IOleInPlaceSite

	virtual HRESULT STDMETHODCALLTYPE CanInPlaceActivate( void);

	virtual HRESULT STDMETHODCALLTYPE OnInPlaceActivate(void);

	virtual HRESULT STDMETHODCALLTYPE OnUIActivate( void);

	virtual HRESULT STDMETHODCALLTYPE GetWindowContext(
	    /* [out] */ __RPC__deref_out_opt IOleInPlaceFrame **ppFrame,
	    /* [out] */ __RPC__deref_out_opt IOleInPlaceUIWindow **ppDoc,
	    /* [out] */ __RPC__out LPRECT lprcPosRect,
	    /* [out] */ __RPC__out LPRECT lprcClipRect,
	    /* [out][in] */ __RPC__inout LPOLEINPLACEFRAMEINFO lpFrameInfo);

	virtual HRESULT STDMETHODCALLTYPE Scroll(
	    /* [in] */ SIZE scrollExtant);

	virtual HRESULT STDMETHODCALLTYPE OnUIDeactivate(
	    /* [in] */ BOOL fUndoable);

	virtual HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate( void);

	virtual HRESULT STDMETHODCALLTYPE DiscardUndoState( void);

	virtual HRESULT STDMETHODCALLTYPE DeactivateAndUndo( void);

	virtual HRESULT STDMETHODCALLTYPE OnPosRectChange(
	    /* [in] */ __RPC__in LPCRECT lprcPosRect);

	/////////////////////////////////////////////////////////////////////////////////////////
	//  IStorage

	virtual HRESULT STDMETHODCALLTYPE CreateStream(
	    /* [string][in] */ __RPC__in_string const OLECHAR *pwcsName,
	    /* [in] */ DWORD grfMode,
	    /* [in] */ DWORD reserved1,
	    /* [in] */ DWORD reserved2,
	    /* [out] */ __RPC__deref_out_opt IStream **ppstm);

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE OpenStream(
	    /* [annotation][string][in] */
	    _In_z_  const OLECHAR *pwcsName,
	    /* [annotation][unique][in] */
	    _Reserved_ void *reserved1,
	    /* [in] */ DWORD grfMode,
	    /* [in] */ DWORD reserved2,
	    /* [annotation][out] */
	    _Outptr_  IStream **ppstm);

	virtual HRESULT STDMETHODCALLTYPE CreateStorage(
	    /* [string][in] */ __RPC__in_string const OLECHAR *pwcsName,
	    /* [in] */ DWORD grfMode,
	    /* [in] */ DWORD reserved1,
	    /* [in] */ DWORD reserved2,
	    /* [out] */ __RPC__deref_out_opt IStorage **ppstg);

	virtual HRESULT STDMETHODCALLTYPE OpenStorage(
	    /* [string][unique][in] */ __RPC__in_opt_string const OLECHAR *pwcsName,
	    /* [unique][in] */ __RPC__in_opt IStorage *pstgPriority,
	    /* [in] */ DWORD grfMode,
	    /* [unique][in] */ __RPC__deref_opt_in_opt SNB snbExclude,
	    /* [in] */ DWORD reserved,
	    /* [out] */ __RPC__deref_out_opt IStorage **ppstg);

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo(
	    /* [in] */ DWORD ciidExclude,
	    /* [annotation][size_is][unique][in] */
	    _In_reads_opt_(ciidExclude)  const IID *rgiidExclude,
	    /* [annotation][unique][in] */
	    _In_opt_  SNB snbExclude,
	    /* [annotation][unique][in] */
	    _In_  IStorage *pstgDest);

	virtual HRESULT STDMETHODCALLTYPE MoveElementTo(
	    /* [string][in] */ __RPC__in_string const OLECHAR *pwcsName,
	    /* [unique][in] */ __RPC__in_opt IStorage *pstgDest,
	    /* [string][in] */ __RPC__in_string const OLECHAR *pwcsNewName,
	    /* [in] */ DWORD grfFlags);

	virtual HRESULT STDMETHODCALLTYPE Commit(
	    /* [in] */ DWORD grfCommitFlags);

	virtual HRESULT STDMETHODCALLTYPE Revert( void);

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE EnumElements(
	    /* [annotation][in] */
	    _Reserved_ DWORD reserved1,
	    /* [annotation][size_is][unique][in] */
	    _Reserved_ void *reserved2,
	    /* [annotation][in] */
	    _Reserved_ DWORD reserved3,
	    /* [annotation][out] */
	    _Outptr_  IEnumSTATSTG **ppenum);

	virtual HRESULT STDMETHODCALLTYPE DestroyElement(
	    /* [string][in] */ __RPC__in_string const OLECHAR *pwcsName);

	virtual HRESULT STDMETHODCALLTYPE RenameElement(
	    /* [string][in] */ __RPC__in_string const OLECHAR *pwcsOldName,
	    /* [string][in] */ __RPC__in_string const OLECHAR *pwcsNewName);

	virtual HRESULT STDMETHODCALLTYPE SetElementTimes(
	    /* [string][unique][in] */ __RPC__in_opt_string const OLECHAR *pwcsName,
	    /* [unique][in] */ __RPC__in_opt const FILETIME *pctime,
	    /* [unique][in] */ __RPC__in_opt const FILETIME *patime,
	    /* [unique][in] */ __RPC__in_opt const FILETIME *pmtime);

	virtual HRESULT STDMETHODCALLTYPE SetClass(
	    /* [in] */ __RPC__in REFCLSID clsid);

	virtual HRESULT STDMETHODCALLTYPE SetStateBits(
	    /* [in] */ DWORD grfStateBits,
	    /* [in] */ DWORD grfMask);

	virtual HRESULT STDMETHODCALLTYPE Stat(
	    /* [out] */ __RPC__out STATSTG *pstatstg,
	    /* [in] */ DWORD grfStatFlag);

protected:
	LONG iComRefCount_;
	IOleObject* oleObject_;
	IOleInPlaceObject* oleInPlaceObject_;
	IWebBrowser2* webBrowser2_;
	RECT rect_;
	HWND parent_;
	HWND hWndControl_;
};

