#pragma once


#include <exdispid.h>

const int _nDispatchID = 1;

class CLoginDialog:public CAxDialogImpl<CLoginDialog>,
	public CMessageFilter, public CIdleHandler,
	public IDispEventSimpleImpl<_nDispatchID, CLoginDialog, &DIID_DWebBrowserEvents2>
{
public:
	DECLARE_WND_SUPERCLASS(_T("LoginDialog"), CAxWindow::GetWndClassName())
public:
	CLoginDialog(void);
	~CLoginDialog(void);

	static _ATL_FUNC_INFO DocumentComplete2_Info;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if((pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
			(pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST))
			return FALSE;

		BOOL bRet = FALSE;
		// give HTML page a chance to translate this message
		if(pMsg->hwnd == m_hWnd || IsChild(pMsg->hwnd))
			bRet = (BOOL)SendMessage(WM_FORWARDMSG, 0, (LPARAM)pMsg);

		return bRet;
	}

	virtual void OnFinalMessage(HWND /*hWnd*/)
	{
		delete this;
	}

	BEGIN_SINK_MAP(CLoginDialog)
		SINK_ENTRY_INFO(_nDispatchID, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE, OnEventDocumentComplete, &DocumentComplete2_Info)
	END_SINK_MAP()


	void __stdcall OnEventDocumentComplete(IDispatch* /*pDisp*/, VARIANT* URL)
	{
		// Send message to the main frame
		ATLASSERT(V_VT(URL) == VT_BSTR);
		USES_CONVERSION;
	}


	BEGIN_MSG_MAP(CLoginDialog)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);

		// Connect events
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl(IID_IWebBrowser2, (void**)&spWebBrowser2);
		if(SUCCEEDED(hRet))
		{
			if(FAILED(DispEventAdvise(spWebBrowser2, &DIID_DWebBrowserEvents2)))
				ATLASSERT(FALSE);
		}

		// Set host flag to indicate that we handle themes
		CComPtr<IAxWinAmbientDispatch> spHost;
		hRet = QueryHost(IID_IAxWinAmbientDispatch, (void**)&spHost);
		if(SUCCEEDED(hRet))
		{
			const DWORD _DOCHOSTUIFLAG_THEME = 0x40000;
			hRet = spHost->put_DocHostFlags(DOCHOSTUIFLAG_NO3DBORDER | _DOCHOSTUIFLAG_THEME);
			ATLASSERT(SUCCEEDED(hRet));
		}

		return lRet;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		// Disconnect events
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl(IID_IWebBrowser2, (void**)&spWebBrowser2);
		if(SUCCEEDED(hRet))
			DispEventUnadvise(spWebBrowser2, &DIID_DWebBrowserEvents2);

		bHandled=FALSE;
		return 1;
	}

};

__declspec(selectany) _ATL_FUNC_INFO CLoginDialog::DocumentComplete2_Info = { CC_STDCALL, VT_EMPTY, 2, { VT_DISPATCH, VT_BYREF | VT_VARIANT } };
