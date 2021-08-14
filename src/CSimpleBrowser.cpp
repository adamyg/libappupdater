//  $Id: CSimpleBrowser.cpp,v 1.10 2021/08/14 05:23:48 cvsuser Exp $
//
//  AutoUpdater: Browser widget.
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

#include "CSimpleBrowser.h"

#if defined(__WATCOMC__)
#include <alloca.h>

class bstr_t {
public:
	static BSTR CreateBSTR(const char *s) {
		const size_t length = strlen(s), bytes = (length + 1) * sizeof(wchar_t);
		wchar_t *wbuffer = (wchar_t *)alloca(bytes);

		::memset(wbuffer, 0, bytes);
		::MultiByteToWideChar(CP_ACP, 0, s, length, wbuffer, length + 1);
		return ::SysAllocString(wbuffer);
	}

	static BSTR CreateBSTR(const wchar_t *s) {
		return ::SysAllocString(s);
	}

	static void FreeBSTR(BSTR bstr) {
		::SysFreeString(bstr);
	}

public:
	bstr_t(const char *s) : bstr_(CreateBSTR(s)) { }
	bstr_t(const wchar_t *s) : bstr_(CreateBSTR(s)) { }
	~bstr_t() {
		FreeBSTR(bstr_);
	}
	operator BSTR() { return bstr_; };

private:
	BSTR bstr_;
};

class variant_t : public ::tagVARIANT {
public:
	variant_t() {
		::VariantInit(this);
	}
	variant_t(const wchar_t *s) {
		V_VT(this)   = VT_BSTR;
		V_BSTR(this) = bstr_t::CreateBSTR(s);
	}
	variant_t(const char *s) {
		V_VT(this)   = VT_BSTR;
		V_BSTR(this) = bstr_t::CreateBSTR(s);
	}
	variant_t(unsigned value) {
		V_VT(this)   = VT_I4;
		V_I4(this)   = value;
	}
	~variant_t() {
		::VariantClear(this);
	}
};

// local copies; OWC linker crashes otherwise, furthermore reduces the library size.
static const CLSID XCLSID_WebBrowser =      { 0x8856F961, 0x340A, 0x11D0, { 0xA9, 0x6B, 0x00, 0xC0, 0x4F, 0xD7, 0x05, 0xA2 } };

static const GUID XIID_IHTMLDocument2 =     { 0x332C4425, 0x26CB, 0x11D0, { 0xB4, 0x83, 0x00, 0xC0, 0x4F, 0xD9, 0x01, 0x19 } };
static const GUID XIID_IOleInPlaceObject =  { 0x00000113, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID XIID_IOleInPlaceSite =    { 0x00000119, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID XIID_IOleObject =         { 0x00000112, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID XIID_IUnknown =           { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID XIID_IWebBrowser2 =       { 0xD30C1661, 0xCDAF, 0x11D0, { 0x8A, 0x3E, 0x00, 0xC0, 0x4F, 0xC9, 0xE2, 0x6E } };

#define CLSID_WebBrowser	XCLSID_WebBrowser

#define IID_IHTMLDocument2	XIID_IHTMLDocument2
#define IID_IOleInPlaceObject	XIID_IOleInPlaceObject
#define IID_IOleInPlaceSite	XIID_IOleInPlaceSite
#define IID_IOleObject		XIID_IOleObject
#define IID_IUnknown		XIID_IUnknown
#define IID_IWebBrowser2	XIID_IWebBrowser2

#endif	//__WATCOMC__


CSimpleBrowser::CSimpleBrowser() : iComRefCount_(0),
	oleObject_(0), oleInPlaceObject_(0), webBrowser2_(0), parent_(0)
{
}


CSimpleBrowser::~CSimpleBrowser()
{
	if (webBrowser2_) {
		webBrowser2_->Release();
		webBrowser2_ = NULL;
		oleObject_->Release();
		oleObject_ = NULL;
	}
}


bool
CSimpleBrowser::CreateBrowser(HWND hParent, const RECT &rect, DWORD dwStyle)
{
	HRESULT hr;

	if (parent_) {
		return false;
	}

	parent_ = hParent;
	rect_ = rect;

	// Create instance.
	hr = ::OleCreate(CLSID_WebBrowser, IID_IOleObject, OLERENDER_DRAW, 0, this, this, (void**)&oleObject_);
	if (FAILED(hr)) {
		MessageBox(NULL, _T("Cannot create WebBrowser"), _T("Error"), MB_ICONERROR);
		parent_ = 0;
		return false;
	}

	hr = oleObject_->SetClientSite(this);
	hr = ::OleSetContainedObject(oleObject_, TRUE);

	// Activating the container.
	RECT t_rect;
	::SetRect(&t_rect, -1, -1, 1, 1);
	hr = oleObject_->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, this, -1, parent_, &t_rect);
	if (FAILED(hr)) {
		MessageBox(NULL, _T("WebBrowser: DoVerb failed"), _T("Error"), MB_ICONERROR);
		oleObject_->Release();
		parent_ = 0;
		return false;
	}

	// Prepare the browser itself.
		//hr = oleObject_->QueryInterface(&webBrowser2_);
	hr = oleObject_->QueryInterface(IID_IWebBrowser2, (void**)&webBrowser2_);
	if (FAILED(hr)) {
		MessageBox(NULL, _T("WebBrowser: QueryInterface failed"), _T("Error"), MB_ICONERROR);
		oleObject_->Release();
		parent_ = 0;
		return false;
	}

	::ShowWindow(GetControlWindow(), SW_SHOW);
	Navigate(_T("about:blank")); // prime document

	return true;
}


// Create in place of dialog control
bool
CSimpleBrowser::CreateFromControl(HWND hParent, UINT nID, DWORD dwStyle)
{
	HWND hDlgItem = ::GetDlgItem(hParent, nID);
	bool result = false;

	if (hDlgItem) {
		RECT rect = {0};
		::ShowScrollBar(hDlgItem, SB_BOTH, FALSE);
		::GetWindowRect(hDlgItem, &rect);
		::MapWindowPoints(0, hParent, (LPPOINT)&rect, 2);
		::SetRect(&rect, 0, 0, (rect.right - rect.left), (rect.bottom - rect.top)); // 0,0 relative
		result = CSimpleBrowser::CreateBrowser(hDlgItem, rect, dwStyle);
	}
	return result;
}


RECT
CSimpleBrowser::PixelToHiMetric(const RECT& _rc)
{
	static bool s_initialized = false;
	static int s_pixelsPerInchX, s_pixelsPerInchY;

	if (!s_initialized) {
		HDC hdc = ::GetDC(0);
		s_pixelsPerInchX = ::GetDeviceCaps(hdc, LOGPIXELSX);
		s_pixelsPerInchY = ::GetDeviceCaps(hdc, LOGPIXELSY);
		::ReleaseDC(0, hdc);
		s_initialized = true;
	}

	RECT rc = {0};
	rc.left   = ::MulDiv(2540, _rc.left, s_pixelsPerInchX);
	rc.top	  = ::MulDiv(2540, _rc.top, s_pixelsPerInchY);
	rc.right  = ::MulDiv(2540, _rc.right, s_pixelsPerInchX);
	rc.bottom = ::MulDiv(2540, _rc.bottom, s_pixelsPerInchY);
	return rc;
}


void
CSimpleBrowser::SetRect(const RECT& _rc)
{
	rect_ = _rc;

	RECT hiMetricRect = PixelToHiMetric(rect_);
	SIZEL sz = {0};
	sz.cx = hiMetricRect.right - hiMetricRect.left;
	sz.cy = hiMetricRect.bottom - hiMetricRect.top;
	oleObject_->SetExtent(DVASPECT_CONTENT, &sz);

	if (oleInPlaceObject_ != 0) {
		oleInPlaceObject_->SetObjectRects(&rect_, &rect_);
	}
}


void
CSimpleBrowser::GoBack()
{
	if (webBrowser2_) webBrowser2_->GoBack();
}


void
CSimpleBrowser::GoForward()
{
	if (webBrowser2_) webBrowser2_->GoForward();
}


void
CSimpleBrowser::Refresh()
{
	if (webBrowser2_) webBrowser2_->Refresh();
}


void
CSimpleBrowser::Navigate(const std::wstring &url)
{
	CSimpleBrowser::Navigate(url.c_str());
}


void
CSimpleBrowser::Navigate(const std::string &url)
{
	const int length = (int)url.length();
	if (length) {
		wchar_t *wstring = (wchar_t *)::calloc((length + 2) * 4, sizeof(wchar_t));
		int wlength;

		if ((wlength = ::MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
				    url.data(), length, wstring, length * 4)) > 0) {
			wstring[wlength] = 0;
			CSimpleBrowser::Navigate(wstring);
		}
		::free(wstring);
	}
}


void
CSimpleBrowser::Navigate(const wchar_t *url)
{
	bstr_t t_url(url);
	variant_t flags(navNoHistory | navNoReadFromCache | navNoWriteToCache);
		//variant_t target_frame_name("");
		//variant_t post_data("");
		//variant_t headers("");

	if (webBrowser2_) webBrowser2_->Navigate(t_url, &flags, 0, 0, 0);
}


/////////////////////////////////////////////////////////////////////////////////////////
//  Document
//

bool
CSimpleBrowser::Content(const std::wstring &content)
{
	IHTMLDocument2 *document = GetDocument();
	if (! document) {
		MessageBox(NULL, _T("WebBrowser: No active document"), _T("Error"), MB_ICONERROR);
		return false;
	}

	// construct text to be written to browser as SAFEARRAY

	SAFEARRAY *safe_array = SafeArrayCreateVector(VT_VARIANT,0,1);
	VARIANT *variant;

	SafeArrayAccessData(safe_array, (LPVOID *)&variant);

	variant->vt = VT_BSTR;
	variant->bstrVal = ::SysAllocString(content.c_str());

	SafeArrayUnaccessData(safe_array);

	// write SAFEARRAY to browser document

	document->write(safe_array);

	// cleanup

	document->Release();
	document = NULL;

	::SysFreeString(variant->bstrVal);
	variant->bstrVal = NULL;

	SafeArrayDestroy(safe_array);

	return true;
}


IHTMLDocument2 *
CSimpleBrowser::GetDocument()
{
	if (! webBrowser2_) {
		return NULL;
	}

	// get browser document's dispatch interface
	IDispatch *document_dispatch = NULL;
	HRESULT hr = webBrowser2_->get_Document(&document_dispatch);
	IHTMLDocument2 *document = NULL;

	if (SUCCEEDED(hr) && (document_dispatch != NULL)) {

		// get the actual document interface
		hr = document_dispatch->QueryInterface(IID_IHTMLDocument2, (void **)&document);

		// release dispatch interface
		document_dispatch->Release();
	}

	return document;
}


HWND
CSimpleBrowser::GetControlWindow()
{
	if (hWndControl_ != 0)
		return hWndControl_;

	if (oleInPlaceObject_ == 0)
		return 0;

	oleInPlaceObject_->GetWindow(&hWndControl_);
	return hWndControl_;
}


/////////////////////////////////////////////////////////////////////////////////////////
//  IUnknown implementation
//

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::QueryInterface(REFIID riid, void**ppvObject)
{
#if defined(__WATCOMC__)
	if (::IsEqualGUID(riid, IID_IUnknown)) {
#else
	if (riid == __uuidof(IUnknown)) {
#endif
		(*ppvObject) = static_cast<IOleClientSite*>(this);

#if defined(__WATCOMC__)
	} else if (::IsEqualGUID(riid, IID_IOleInPlaceSite)) {
#else
	} else if (riid == __uuidof(IOleInPlaceSite)) {
#endif
		(*ppvObject) = static_cast<IOleInPlaceSite*>(this);

	} else {
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE
CSimpleBrowser::AddRef(void)
{
	++iComRefCount_;
	return iComRefCount_;
}

ULONG STDMETHODCALLTYPE
CSimpleBrowser::Release(void)
{
	--iComRefCount_;
	return iComRefCount_;
}


/////////////////////////////////////////////////////////////////////////////////////////
//  IOleWindow implementation
//

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::GetWindow(__RPC__deref_out_opt HWND *phwnd)
{
	(*phwnd) = parent_;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::ContextSensitiveHelp(BOOL fEnterMode)
{
	return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////////////////
//  IOleInPlaceSite implementation
//

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::CanInPlaceActivate(void)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::OnInPlaceActivate(void)
{
	::OleLockRunning(oleObject_, TRUE, FALSE);
	oleObject_->QueryInterface(IID_IOleInPlaceObject, (void **)&oleInPlaceObject_);
	oleInPlaceObject_->SetObjectRects(&rect_, &rect_);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::OnUIActivate(void)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::GetWindowContext(__RPC__deref_out_opt IOleInPlaceFrame **ppFrame, __RPC__deref_out_opt IOleInPlaceUIWindow **ppDoc,
			__RPC__out LPRECT lprcPosRect,__RPC__out LPRECT lprcClipRect,__RPC__inout LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
	HWND hwnd = parent_;

	(*ppFrame) = NULL;
	(*ppDoc) = NULL;
	(*lprcPosRect).left   = rect_.left;
	(*lprcPosRect).top    = rect_.top;
	(*lprcPosRect).right  = rect_.right;
	(*lprcPosRect).bottom = rect_.bottom;
	*lprcClipRect = *lprcPosRect;

	lpFrameInfo->fMDIApp   = false;
	lpFrameInfo->hwndFrame = hwnd;
	lpFrameInfo->haccel    = NULL;
	lpFrameInfo->cAccelEntries = 0;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::Scroll(SIZE scrollExtant)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::OnUIDeactivate(BOOL fUndoable)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::OnInPlaceDeactivate(void)
{
	hWndControl_ = 0;
	oleInPlaceObject_ = 0;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::DiscardUndoState(void)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::DeactivateAndUndo(void)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::OnPosRectChange(__RPC__in LPCRECT lprcPosRect)
{
	return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////////////////
//  IOleClientSite implementation
//

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::SaveObject(void)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::GetMoniker(DWORD dwAssign,DWORD dwWhichMoniker,__RPC__deref_out_opt IMoniker **ppmk)
{
	if((dwAssign == OLEGETMONIKER_ONLYIFTHERE) && (dwWhichMoniker == OLEWHICHMK_CONTAINER))
		return E_FAIL;
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::GetContainer(__RPC__deref_out_opt IOleContainer **ppContainer)
{
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::ShowObject(void)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::OnShowWindow(BOOL fShow)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::RequestNewObjectLayout(void)
{
	return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////////////////
//  IStorage implementation
//

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::CreateStream(__RPC__in_string const OLECHAR *pwcsName,DWORD grfMode,DWORD reserved1,DWORD reserved2,__RPC__deref_out_opt IStream **ppstm)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::OpenStream(const OLECHAR *pwcsName,void *reserved1,DWORD grfMode,DWORD reserved2,IStream **ppstm)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::CreateStorage(__RPC__in_string const OLECHAR *pwcsName,DWORD grfMode,DWORD reserved1,DWORD reserved2,__RPC__deref_out_opt IStorage **ppstg)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::OpenStorage(__RPC__in_opt_string const OLECHAR *pwcsName,__RPC__in_opt IStorage *pstgPriority,DWORD grfMode,
		    __RPC__deref_opt_in_opt SNB snbExclude, DWORD reserved,__RPC__deref_out_opt IStorage **ppstg)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::CopyTo(DWORD ciidExclude,const IID *rgiidExclude,__RPC__in_opt SNB snbExclude,IStorage *pstgDest)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::MoveElementTo(__RPC__in_string const OLECHAR *pwcsName,__RPC__in_opt IStorage *pstgDest,__RPC__in_string const OLECHAR *pwcsNewName,DWORD grfFlags)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::Commit(DWORD grfCommitFlags)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::Revert(void)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::EnumElements(DWORD reserved1,void *reserved2,DWORD reserved3,IEnumSTATSTG **ppenum)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::DestroyElement(__RPC__in_string const OLECHAR *pwcsName)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::RenameElement(__RPC__in_string const OLECHAR *pwcsOldName,__RPC__in_string const OLECHAR *pwcsNewName)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::SetElementTimes(__RPC__in_opt_string const OLECHAR *pwcsName,__RPC__in_opt const FILETIME *pctime,
	__RPC__in_opt const FILETIME *patime, __RPC__in_opt const FILETIME *pmtime)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::SetClass(__RPC__in REFCLSID clsid)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::SetStateBits(DWORD grfStateBits,DWORD grfMask)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CSimpleBrowser::Stat(__RPC__out STATSTG *pstatstg,DWORD grfStatFlag)
{
	return E_NOTIMPL;
}
