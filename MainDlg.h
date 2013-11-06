// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include <atlframe.h>
#include "resource.h"
#include "AboutDlg.h"
#include "BrowserView.h"
#include <atlmisc.h>

#include <IWeibo.hxx>
#include <IWeiboMethod.hxx>
#include <util/threading/Lock.hxx>
#include <IWeiboDef.hxx>
#include <IWeiboMethod.hxx>
#include <ParsingObject.hxx>
#include "ParsingDefine.hxx"
#include "ParsingDataStruct.h"
#include <util/threading/Lock.hxx>
using namespace weibo;


#define APP_KEY "397065771"
#define APP_SECRET "8b1a204835edfbde7e933a48b24cd79b"
#define REDIRECT_URL "http://www.baidu.com"

class CMainDlg : public CDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>,
		public CMessageFilter, public CIdleHandler,public CWinDataExchange<CMainDlg>
{
public:

	CEdit	m_newWeibo;
	WTL::CEdit	m_logInfo;

	string m_strUid;

	BEGIN_DDX_MAP(CMainDlg)
		DDX_CONTROL_HANDLE(IDC_EDIT_INFO, m_logInfo)
		DDX_CONTROL_HANDLE(IDC_EDIT_NEW_WEIBO, m_newWeibo)
	END_DDX_MAP();
	
	CBrowserView *pView;

	boost::shared_ptr<weibo::IWeibo> mWeiboPtr;

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return CWindow::IsDialogMessage(pMsg);
	}

	virtual BOOL OnIdle()
	{
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		COMMAND_HANDLER(IDC_BTN_GET_NEW_WEIBO, BN_CLICKED, OnBnClickedBtnGetNewWeibo)
		COMMAND_HANDLER(IDC_BTN_UNREAD, BN_CLICKED, OnBnClickedBtnUnread)
		COMMAND_HANDLER(IDC_BTN_SEND, BN_CLICKED, OnBnClickedBtnSend)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER(IDOK,OnOK)
		MESSAGE_HANDLER(WM_OAUTH_SUCCESS,OnAuthSuccess)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// center the dialog on the screen
		CenterWindow();

		CString strUrl = _T("https://api.weibo.com/oauth2/authorize?client_id=397065771&response_type=code&redirect_uri=www.baidu.com");

		pView = new CBrowserView;
		RECT rt;
		rt.left = 100;
		rt.bottom=100;

		GetWindowRect(&rt);

		rt.right += 350;
		rt.bottom += 200;

		pView->Create(m_hWnd, rt, strUrl, WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL);

		// set icons
		HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
		SetIcon(hIcon, TRUE);
		HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		SetIcon(hIconSmall, FALSE);

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		UIAddChildWindowContainer(m_hWnd);

		DoDataExchange(false);

		mWeiboPtr = weibo::WeiboFactory::getWeibo();

		mWeiboPtr->startup();
		mWeiboPtr->setOption(weibo::WOPT_CONSUMER, APP_KEY, APP_SECRET);

		std::string url("https://api.weibo.com/oauth2/authorize?client_id=");
		url += APP_KEY;
		url += "&redirect_uri=";
		url += REDIRECT_URL;
		url += "&response_type=code";

		mWeiboPtr->OnDelegateComplated += std::make_pair(this, &CMainDlg::onWeiboRespComplated);
		mWeiboPtr->OnDelegateErrored += std::make_pair(this, &CMainDlg::onWeiboRespErrored);
		mWeiboPtr->OnDelegateWillRelease += std::make_pair(this, &CMainDlg::onWeiboRespStoped);

		//mWeiboPtr->getMethod()->oauth2("leileigood888@sina.com","23213352321335");

		return TRUE;
	}


	void onWeiboRespComplated(unsigned int optionId, const char* httpHeader, weibo::ParsingObject* result, const weibo::UserTaskInfo* pTask)
	{

		onResponseProcess(optionId, result, 0, 0, true);
	}

	void onWeiboRespErrored(unsigned int optionId, const int errCode, const int errSubCode, weibo::ParsingObject* result, const weibo::UserTaskInfo* pTask)
	{

		onResponseProcess(optionId, result, errCode, errSubCode, false);
	}

	void onWeiboRespStoped(unsigned int optionId, const weibo::UserTaskInfo* pTask)
	{

		// Do noting!
	}


	void onResponseProcess(unsigned int optionId, weibo::ParsingObject* resultObj, const int errCode, const int errSubCode, bool isComplated)
	{


		if (resultObj)
		{
			ParsingObject* tempObject = new ParsingObject(*resultObj);
			ParsingObjectPtr objPtr(tempObject);

			
			// The special event.
			switch(optionId)
			{
			case WBOPT_OAUTH2_ACCESS_TOKEN:
				{
					USES_CONVERSION;
					ParsingOauthRet ret;
					ret.doParse(objPtr);
					
					m_strUid = ret.uid;
					//std::string access_token = objPtr->getSubStringByKey("access_token");

					mWeiboPtr->setOption(WOPT_ACCESS_TOKEN, ret.access_token.c_str());
					CString strAdd(A2W(ret.access_token.c_str()));
					CString strBefore = "\r\naccess_token=";
					strBefore += strAdd;
					m_logInfo.AppendText(strBefore);
				}
				break;
			case WBOPT_POST_STATUSES_UPDATE:
				{
					m_logInfo.AppendText(_T("\r\n微博发送成功!"));
				}
				break;
			case WBOPT_GET_REMIND_UNREAD_COUNT:
				{
					USES_CONVERSION;
					string strCountUnRead = objPtr->getSubStringByKey("status");
					CString strAdd(A2W(strCountUnRead.c_str()));
					CString strBefore = "\r\n当前所有未读微博数：";
					strBefore += strAdd;
					m_logInfo.AppendText(strBefore);
				}
				break;
			case WBOPT_GET_STATUSES_FRIENDS_TIMELINE:
				{
					USES_CONVERSION;
					ParsingObjectPtr pAllNewWeibo = objPtr->getSubObjectByKey("statuses");
					for (int i = 0 ; i < pAllNewWeibo->getSubCounts() ; i ++)
					{
						
					
						ParsingObjectPtr pWeibo = pAllNewWeibo->getSubObjectByIndex(i);
						string strText = pWeibo->getSubStringByKey("text");
						ParsingObjectPtr pUser = pWeibo->getSubObjectByKey("user");
						string strUser = "";
						if (pUser)
						{
							strUser = pUser->getSubStringByKey("screen_name");
						}
						else
						{
							continue;
						}
						CString strHead = _T("\r\n--------------------------\r\n");
					
						strUser += "\r\n" + strText;

						DWORD dwNum = MultiByteToWideChar(CP_UTF8,NULL,strUser.c_str(),-1,NULL,0);
						WCHAR *psText;
						psText = new WCHAR[dwNum];
						if(!psText)
						{
							delete []psText;
						}
						MultiByteToWideChar (CP_UTF8,NULL,strUser.c_str(),-1,psText,dwNum);
						
						m_logInfo.AppendText(strHead);

						m_logInfo.AppendText(psText);
						
						m_logInfo.AppendText(strHead);

						delete []psText;
					}
				}
				break;
				//case WBOPT_GET_ACCOUNT_GET_UID:
				//	{
				//		ParsingResultPtr pret = boost::dynamic_pointer_cast<ParsingResult>(result->pasringPtr_);
				//		if (pret)
				//		{
				//			mMYID = pret->asAString("uid");
				//		}
				//	}
				//	break;

			default:
				break;
			}
		}
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{

		if (pView)
		{
			pView->DestroyWindow();
		}

		// unregister message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
		pLoop->RemoveIdleHandler(this);


		mWeiboPtr->OnDelegateComplated -= std::make_pair(this, &CMainDlg::onWeiboRespComplated);
		mWeiboPtr->OnDelegateErrored -= std::make_pair(this, &CMainDlg::onWeiboRespErrored);
		mWeiboPtr->OnDelegateWillRelease -= std::make_pair(this, &CMainDlg::onWeiboRespStoped);
		mWeiboPtr->shutdown();

		return 0;
	}

	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if (pView)
		{
			pView->ShowWindow(SW_SHOW);
		}

		// TODO: Add validation code 
		//CloseDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CloseDialog(wID);
		return 0;
	}

	void CloseDialog(int nVal)
	{
		DestroyWindow();
		::PostQuitMessage(nVal);
	}

	LRESULT OnAuthSuccess(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		USES_CONVERSION;
		LPCTSTR lpstrAddress = (LPCTSTR)lParam;

		CString str(lpstrAddress);
		
		int pos  = -1;
		pos = str.Find('=');
		
		CString retnCode = str.Right(str.GetLength() - pos - 1);

		if (pView)
		{
			pView->ShowWindow(SW_HIDE);
		}


		CString strAddText = "\r\n获取code成功,code=" + retnCode;

		m_logInfo.AppendText(strAddText.GetBuffer(strAddText.GetLength()));

		strAddText.ReleaseBuffer();


		mWeiboPtr->getMethod()->oauth2Code(W2A(retnCode.GetBuffer(retnCode.GetLength())), REDIRECT_URL , NULL);
		retnCode.ReleaseBuffer();

		return 0;
	}
	LRESULT OnBnClickedBtnSend(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{

		USES_CONVERSION;

		CString str;
		m_newWeibo.GetWindowText(str.GetBuffer(128),128);

		
		DWORD dwNum = WideCharToMultiByte(CP_UTF8,NULL,str.GetBuffer(str.GetLength()),-1,NULL,0,NULL,FALSE);
		char *psText;
		psText = new char[dwNum];
		if(!psText)
		{
			delete []psText;
		}
		WideCharToMultiByte (CP_UTF8,NULL,str.GetBuffer(str.GetLength()),-1,psText,dwNum,NULL,FALSE);
		


		mWeiboPtr->getMethod()->postStatusesUpdate(psText, NULL, NULL);

		delete []psText;
		str.ReleaseBuffer();

		CString logInfo = "\r\n发送微博测试，内容：";
		logInfo += str;

		m_logInfo.AppendText(logInfo);

		m_newWeibo.SetWindowText(_T(""));


		return 0;
	}
	LRESULT OnBnClickedBtnUnread(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		mWeiboPtr->getMethod()->getRemindUnreadCount(m_strUid.c_str());

		return 0;
	}
	LRESULT OnBnClickedBtnGetNewWeibo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
	
		mWeiboPtr->getMethod()->getStatusesFriendTimeline();

		return 0;
	}
};
