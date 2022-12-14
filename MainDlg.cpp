// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MainDlg.h"	
#include <algorithm>

#define  MAX_FILESIZE			4*1024*1024			//可发送文件最大限制
#define  MAX_IMAGESIZE			4*1024*1024			//图片最大限制

CMainDlg::CMainDlg() : SHostWnd(_T("LAYOUT:XML_MAINWND"))
{
	m_bLayoutInited = FALSE;

	m_bSnapshotHideWindow = false;
	m_bResultEmpty = true;

	m_pAdapterSearchResult = NULL;
	m_pEmojiDlg = NULL;
	m_bEmotionShow = false;

	SNotifyCenter::getSingleton().addEvent(EVENTID(EventSnapshotFinish));
}

CMainDlg::~CMainDlg()
{
}

int CMainDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	SetMsgHandled(FALSE);
	return 0;
}

BOOL CMainDlg::OnInitDialog(HWND hWnd, LPARAM lParam)
{
	m_bLayoutInited = TRUE;

	if (!ImageProvider::IsExist(L"default_portrait"))
	{
		SAntialiasSkin* pSkin = new SAntialiasSkin();
		pSkin->SetRound(TRUE);

		if (pSkin->LoadFromFile(SApplication::getSingleton().GetAppDir()+L"\\default_res\\default_portrait.png"))
			ImageProvider::Insert(L"default_portrait", pSkin);
		else
			delete pSkin;
	}

	::DragAcceptFiles(m_hWnd, TRUE);
	::RegisterDragDrop(m_hWnd, GetDropTarget());

	SImageButton* pBtnMessage = FindChildByName2<SImageButton>(L"btn_message");
	SImageButton* pBtnContact = FindChildByName2<SImageButton>(L"btn_contact");
	SImageButton* pBtnFavorites = FindChildByName2<SImageButton>(L"btn_favorites");
	SASSERT(pBtnMessage);
	SASSERT(pBtnContact);
	SASSERT(pBtnFavorites);

	pBtnMessage->SetCheck(TRUE);
	pBtnContact->SetCheck(FALSE);
	pBtnFavorites->SetCheck(FALSE);

	SListView* pLasttalkList = FindChildByName2<SListView>(L"lv_list_lasttalk");
	SASSERT(pLasttalkList);
	pLasttalkList->EnableScrollBar(SSB_HORZ, FALSE);

	m_pAdapterLasttalk = new CAdapter_MessageList(pLasttalkList, this);
	pLasttalkList->SetAdapter(m_pAdapterLasttalk);
	m_pAdapterLasttalk->Release();

	SListView* pSearchResult = FindChildByName2<SListView>(L"lv_search_result");
	SASSERT(pSearchResult);
	pSearchResult->EnableScrollBar(SSB_HORZ, FALSE);

	m_pAdapterSearchResult = new CAdapter_SearchResult(pSearchResult, this);
	pSearchResult->SetAdapter(m_pAdapterSearchResult);
	m_pAdapterSearchResult->Release();

	STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
	SASSERT(pChatTab);
	//添加测试数据
	m_pAdapterLasttalk->AddItem(0, "file_helper");
	CGlobalUnits::GetInstance()->m_mapLasttalkTime.insert(std::make_pair("file_helper", ""));
	{//文件服务page
		SStringW sstrPage;
		sstrPage.Format(L"<page title='%s'><include src='layout:XML_PAGE_COMMON_FILEHELPER'/></page>", L"file_helper");
		pChatTab->InsertItem(sstrPage);

		SWindow* pPage = pChatTab->GetPage(L"file_helper", TRUE);
		SASSERT(pPage);
		SImRichEdit* pRecvRichedit = pPage->FindChildByName2<SImRichEdit>(L"recv_richedit");
		SImRichEdit* pSendRichedit = pPage->FindChildByName2<SImRichEdit>(L"send_richedit");
		SASSERT(pRecvRichedit);
		SASSERT(pSendRichedit);

		SUBSCRIBE(pSendRichedit, EVT_RE_OBJ, CMainDlg::OnSendRichEditObjEvent);
		SUBSCRIBE(pSendRichedit, EVT_RE_NOTIFY, CMainDlg::OnSendRichEditEditorChange);
		SUBSCRIBE(pSendRichedit, EVT_RE_QUERY_ACCEPT, CMainDlg::OnSendRichEditAcceptData);
		SUBSCRIBE(pSendRichedit, EVT_CTXMENU, CMainDlg::OnSendRichEditMenu);

		SUBSCRIBE(pRecvRichedit, EVT_RE_OBJ, CMainDlg::OnRecvRichEditObjEvent);
		SUBSCRIBE(pRecvRichedit, EVT_RE_SCROLLBAR, CMainDlg::OnRecvRichEditScrollEvent);
		SUBSCRIBE(pRecvRichedit, EVT_RE_QUERY_ACCEPT, CMainDlg::OnRecvRichEditAcceptData);
		SUBSCRIBE(pRecvRichedit, EVT_CTXMENU, CMainDlg::OnRecvRichEditMenu);

		AddFetchMoreBlock(pRecvRichedit);
	}
	m_pAdapterLasttalk->AddItem(3, "page_dyh");
	CGlobalUnits::GetInstance()->m_mapLasttalkTime.insert(std::make_pair("page_dyh", ""));
	{
		SStringW sstrPage;
		sstrPage.Format(L"<page title='%s'><include src='layout:XML_PAGE_DYH'/></page>", L"page_dyh");
		pChatTab->InsertItem(sstrPage);
	}
	m_pAdapterLasttalk->AddItem(4, "page_news");
	CGlobalUnits::GetInstance()->m_mapLasttalkTime.insert(std::make_pair("page_news", ""));
	{
		SStringW sstrPage;
		sstrPage.Format(L"<page title='%s'><include src='layout:XML_PAGE_NEWS'/></page>", L"page_news");
		pChatTab->InsertItem(sstrPage);
	}
	m_pAdapterLasttalk->AddItem(5, "page_gzh");
	CGlobalUnits::GetInstance()->m_mapLasttalkTime.insert(std::make_pair("page_gzh", ""));
	{
		SStringW sstrPage;
		sstrPage.Format(L"<page title='%s'><include src='layout:XML_PAGE_GZH'/></page>", L"page_gzh");
		pChatTab->InsertItem(sstrPage);
	}


	SStatic* pCurName = FindChildByName2<SStatic>(L"page_name");
	SASSERT(pCurName);
	pCurName->SetVisible(FALSE);

	SImageButton* pImgBtnMore = FindChildByName2<SImageButton>(L"btn_more");
	SASSERT(pImgBtnMore);
	pImgBtnMore->SetVisible(FALSE);


	STreeView * pTreeView = FindChildByName2<STreeView>("tv_Friend");
	if (pTreeView)
	{
		m_pTreeViewAdapter = new CContactTreeViewAdapter(this);
		pTreeView->SetAdapter(m_pTreeViewAdapter);
		m_pTreeViewAdapter->Release();
	}

	if (!m_pEmojiDlg)
	{
		m_pEmojiDlg = new CEmojiDlg(this);
		m_pEmojiDlg->Create(this->m_hWnd, 0,0,0,0);
		m_pEmojiDlg->SendMessage(WM_INITDIALOG);
	}

	return 0;
}
//TODO:消息映射
void CMainDlg::OnClose()
{
	SNativeWnd::DestroyWindow();
}

void CMainDlg::OnMaximize()
{
	SendMessage(WM_SYSCOMMAND, SC_MAXIMIZE);
}
void CMainDlg::OnRestore()
{
	SendMessage(WM_SYSCOMMAND, SC_RESTORE);
}
void CMainDlg::OnMinimize()
{
	SendMessage(WM_SYSCOMMAND, SC_MINIMIZE);
}

void CMainDlg::OnSize(UINT nType, CSize size)
{
	SetMsgHandled(FALSE);
	if (!m_bLayoutInited) return;
	
	SWindow *pBtnMax = FindChildByName(L"btn_max");
	SWindow *pBtnRestore = FindChildByName(L"btn_restore");
	if(!pBtnMax || !pBtnRestore) return;
	
	if (nType == SIZE_MAXIMIZED)
	{
		pBtnRestore->SetVisible(TRUE);
		pBtnMax->SetVisible(FALSE);
	}
	else if (nType == SIZE_RESTORED)
	{
		pBtnRestore->SetVisible(FALSE);
		pBtnMax->SetVisible(TRUE);
	}
}

void CMainDlg::OnLButtonDown(UINT nFlags, CPoint pt)
{
	SetMsgHandled(FALSE);

	SImageButton* pBtnEmotion = FindChildByName2<SImageButton>(L"btn_emotion");
	if (pBtnEmotion)
	{
		SOUI::CRect rcEmotion = pBtnEmotion->GetWindowRect();
		if (!PtInRect(& rcEmotion, pt))
		{
			if (m_pEmojiDlg)
			{
				m_pEmojiDlg->ShowWindow(SW_HIDE);
				m_bEmotionShow = false;
			}
		}
	}
}


//演示如何响应菜单事件
void CMainDlg::OnCommand(UINT uNotifyCode, int nID, HWND wndCtl)
{
	if (uNotifyCode == 0)
	{
		switch (nID)
		{
		case 6:
			PostMessage(WM_CLOSE);
			break;
		default:
			break;
		}
	}
}


void CMainDlg::OnBnClickMessage()
{
	SImageButton* pBtnMessage = FindChildByName2<SImageButton>(L"btn_message");
	SImageButton* pBtnContact = FindChildByName2<SImageButton>(L"btn_contact");
	SImageButton* pBtnFavorites = FindChildByName2<SImageButton>(L"btn_favorites");
	SASSERT(pBtnMessage);
	SASSERT(pBtnContact);
	SASSERT(pBtnFavorites);

	pBtnMessage->SetCheck(TRUE);
	pBtnContact->SetCheck(FALSE);
	pBtnFavorites->SetCheck(FALSE);

	STabCtrl* pLeftListTab = FindChildByName2<STabCtrl>(L"leftlist_tabctrl");
	SASSERT(pLeftListTab);
	pLeftListTab->SetCurSel(L"lasttalk_page", TRUE);

	if ("" != m_LasttalkCurSel.m_strID)
	{
		SStatic* pCurName = FindChildByName2<SStatic>(L"page_name");
		SASSERT(pCurName);
		pCurName->SetVisible(TRUE);
	}

	SImageButton* pImgBtnMore = FindChildByName2<SImageButton>(L"btn_more");
	SASSERT(pImgBtnMore);
	if (3 == m_LasttalkCurSel.m_nType ||
		4 == m_LasttalkCurSel.m_nType ||
		5 == m_LasttalkCurSel.m_nType)
	{
		pImgBtnMore->SetVisible(FALSE);
	}
	else
		pImgBtnMore->SetVisible(TRUE);

	if ("" != m_LasttalkCurSel.m_strID)
	{
		SStringW sstrID = S_CA2W(m_LasttalkCurSel.m_strID.c_str());
		STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
		SASSERT(pChatTab);
		pChatTab->SetCurSel(sstrID, TRUE);
	}

	SStatic* pCurName = FindChildByName2<SStatic>(L"page_name");
	SASSERT(pCurName);
	pCurName->SetVisible(TRUE);

	GetRoot()->Invalidate();
}

void CMainDlg::OnBnClickContact()
{
	SImageButton* pBtnMessage = FindChildByName2<SImageButton>(L"btn_message");
	SImageButton* pBtnContact = FindChildByName2<SImageButton>(L"btn_contact");
	SImageButton* pBtnFavorites = FindChildByName2<SImageButton>(L"btn_favorites");
	SASSERT(pBtnMessage);
	SASSERT(pBtnContact);
	SASSERT(pBtnFavorites);

	pBtnMessage->SetCheck(FALSE);
	pBtnContact->SetCheck(TRUE);
	pBtnFavorites->SetCheck(FALSE);

	STabCtrl* pLeftListTab = FindChildByName2<STabCtrl>(L"leftlist_tabctrl");
	SASSERT(pLeftListTab);
	pLeftListTab->SetCurSel(L"contact_page", TRUE);

	STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
	SASSERT(pChatTab);
	pChatTab->SetCurSel(0);

	SImageButton* pImgBtnMore = FindChildByName2<SImageButton>(L"btn_more");
	SASSERT(pImgBtnMore);
	pImgBtnMore->SetVisible(FALSE);

	SStatic* pCurName = FindChildByName2<SStatic>(L"page_name");
	SASSERT(pCurName);
	pCurName->SetVisible(FALSE);

	GetRoot()->Invalidate();
}

void CMainDlg::OnBnClickFavorites()
{
	SImageButton* pBtnMessage = FindChildByName2<SImageButton>(L"btn_message");
	SImageButton* pBtnContact = FindChildByName2<SImageButton>(L"btn_contact");
	SImageButton* pBtnFavorites = FindChildByName2<SImageButton>(L"btn_favorites");
	SASSERT(pBtnMessage);
	SASSERT(pBtnContact);
	SASSERT(pBtnFavorites);

	pBtnMessage->SetCheck(FALSE);
	pBtnContact->SetCheck(FALSE);
	pBtnFavorites->SetCheck(TRUE);

	STabCtrl* pLeftListTab = FindChildByName2<STabCtrl>(L"leftlist_tabctrl");
	SASSERT(pLeftListTab);
	pLeftListTab->SetCurSel(L"favorites_page", TRUE);

	STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
	SASSERT(pChatTab);
	pChatTab->SetCurSel(0);

	SImageButton* pImgBtnMore = FindChildByName2<SImageButton>(L"btn_more");
	SASSERT(pImgBtnMore);
	pImgBtnMore->SetVisible(FALSE);

	SStatic* pCurName = FindChildByName2<SStatic>(L"page_name");
	SASSERT(pCurName);
	pCurName->SetVisible(FALSE);

	GetRoot()->Invalidate();
}

void CMainDlg::OnBnClickMenu()
{
	//TODO:
}

void CMainDlg::OnBnClickSearchCancel()
{
	STabCtrl* pTab = FindChildByName2<STabCtrl>(L"leftlist_tabctrl");
	SASSERT(pTab);
	pTab->SetCurSel(m_nOldCurSel);

	SEdit *pEdit = FindChildByName2<SEdit>(L"search_edit");
	SASSERT(pEdit);
	pEdit->SetWindowText(L"");
	m_bResultEmpty = true;
}

void CMainDlg::MessageListItemClick(int nType, const std::string& strID)
{
	STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
	SASSERT(pChatTab);

	SStringW sstrID = S_CA2W(strID.c_str());
	pChatTab->SetCurSel(sstrID, TRUE);

	SStatic* pCurName = FindChildByName2<SStatic>(L"page_name");
	SASSERT(pCurName);

	SStringW sstrName = L"";
	switch (nType)
	{
	case 0://filehelper
		sstrName = L"文件传输助手";
		break;
	case 1://personal
		{
			std::string strName = "";
			PersonalsMap::iterator iter = CGlobalUnits::GetInstance()->m_mapPersonals.find(strID);
			if (iter != CGlobalUnits::GetInstance()->m_mapPersonals.end())
				strName = iter->second.m_strName;
			else
				strName = strID;

			sstrName = S_CA2W(strName.c_str());
		}
		break;
	case 2://group
		{
			std::string strName = "";
			GroupsMap::iterator iter = CGlobalUnits::GetInstance()->m_mapGroups.find(strID);
			if (iter != CGlobalUnits::GetInstance()->m_mapGroups.end())
				strName = iter->second.m_strGroupName;
			else
				strName = strID;

			sstrName = S_CA2W(strName.c_str());
		}
		break;
	case 3://订阅号
		sstrName = L"订阅号";
		break;
	case 4://新闻
		sstrName = L"新闻";
		break;
	case 5://公众号
		sstrName = L"公众号";
		break;
	default:
		break;
	}
	pCurName->SetVisible(TRUE);
	pCurName->SetWindowText(sstrName);
	pCurName->Invalidate();

	m_LasttalkCurSel.m_nType = nType;
	m_LasttalkCurSel.m_strID = strID;

	SImageButton* pImgBtnMore = FindChildByName2<SImageButton>(L"btn_more");
	SASSERT(pImgBtnMore);
	if (3 == m_LasttalkCurSel.m_nType ||
		4 == m_LasttalkCurSel.m_nType ||
		5 == m_LasttalkCurSel.m_nType)
	{
		pImgBtnMore->SetVisible(FALSE);
	}
	else
		pImgBtnMore->SetVisible(TRUE);
	
	GetRoot()->Invalidate();
}

void CMainDlg::MessageListItemRClick(int nType, const std::string& strID)
{
	MenuWrapper menu(L"menu_lasttalk_list", L"SMENU");
	menu.AddMenu(_T("置顶"), 1, TRUE, FALSE);
	menu.AddMenu(_T("消息免打扰"), 2, TRUE, FALSE);
	menu.AddMenu(_T("查看详情"), 3, TRUE, FALSE);
	menu.AddMenu(_T("怕不是个瓜皮"), 4, TRUE, FALSE);
	menu.AddSeperator();
	menu.AddMenu(_T("删除"), 5, TRUE, FALSE);

	int ret = 0;
	POINT pt;
	::GetCursorPos(&pt);

	ret = menu.ShowMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
	switch (ret)
	{
	case 1:
		break;
	case 2:
		break;
	default:
		break;
	}
}

void CMainDlg::ContactItemClick(int nGID, const std::string& strID)
{
	int i = 0;
}

void CMainDlg::ContactItemDBClick(int nGID, const std::string& strID)
{
	/*
	*	根据GID区分
	*	1、新朋友
	*	2、公众号
	*	3、订阅号
	*	4、群聊
	*	5、个人
	*/
	std::ostringstream os;
	if (1 == nGID || 2 == nGID || 3 == nGID)	return;    //不处理新朋友，公众号，订阅号的双击事件

	if (4 == nGID)		//group
		m_pAdapterLasttalk->AddItem(2, strID);
	else if (5 == nGID)	//personal
		m_pAdapterLasttalk->AddItem(1, strID);

	time_t local;
	time(&local);
	os.str("");
	os<<local;
	std::string strTimestamp = os.str();
	std::map<std::string, std::string>::iterator iterTime = CGlobalUnits::GetInstance()->m_mapLasttalkTime.find(strID);
	if (iterTime != CGlobalUnits::GetInstance()->m_mapLasttalkTime.end())
	{
		CGlobalUnits::GetInstance()->m_mapLasttalkTime.erase(strID);
		CGlobalUnits::GetInstance()->m_mapLasttalkTime.insert(std::make_pair(strID, strTimestamp));
	}
	else
		CGlobalUnits::GetInstance()->m_mapLasttalkTime.insert(std::make_pair(strID, strTimestamp));


	SStringW sstrID = S_CA2W(strID.c_str());
	STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
	SASSERT(pChatTab);
	SStringW sstrPage;	
	if (4 == nGID)		//group
		sstrPage.Format(L"<page title='%s'><include src='layout:XML_PAGE_COMMON_GROUP'/></page>", sstrID);
	else if (5 == nGID)	//personal
		sstrPage.Format(L"<page title='%s'><include src='layout:XML_PAGE_COMMON_PERSONAL'/></page>", sstrID);
	pChatTab->InsertItem(sstrPage);

	SWindow* pPage = pChatTab->GetPage(sstrID, TRUE);
	SASSERT(pPage);
	SImRichEdit* pRecvRichedit = pPage->FindChildByName2<SImRichEdit>(L"recv_richedit");
	SImRichEdit* pSendRichedit = pPage->FindChildByName2<SImRichEdit>(L"send_richedit");
	SASSERT(pRecvRichedit);
	SASSERT(pSendRichedit);

	SUBSCRIBE(pSendRichedit, EVT_RE_OBJ, CMainDlg::OnSendRichEditObjEvent);
	SUBSCRIBE(pSendRichedit, EVT_RE_NOTIFY, CMainDlg::OnSendRichEditEditorChange);
	SUBSCRIBE(pSendRichedit, EVT_RE_QUERY_ACCEPT, CMainDlg::OnSendRichEditAcceptData);
	SUBSCRIBE(pSendRichedit, EVT_CTXMENU, CMainDlg::OnSendRichEditMenu);

	SUBSCRIBE(pRecvRichedit, EVT_RE_OBJ, CMainDlg::OnRecvRichEditObjEvent);
	SUBSCRIBE(pRecvRichedit, EVT_RE_SCROLLBAR, CMainDlg::OnRecvRichEditScrollEvent);
	SUBSCRIBE(pRecvRichedit, EVT_RE_QUERY_ACCEPT, CMainDlg::OnRecvRichEditAcceptData);
	SUBSCRIBE(pRecvRichedit, EVT_CTXMENU, CMainDlg::OnRecvRichEditMenu);

	AddFetchMoreBlock(pRecvRichedit);

	if (4 == nGID)		//group
	{
		m_LasttalkCurSel.m_nType = 2;
		m_LasttalkCurSel.m_strID = strID;
	}
	else if (5 == nGID)	//personal
	{
		m_LasttalkCurSel.m_nType = 1;
		m_LasttalkCurSel.m_strID = strID;
	}
	OnBnClickMessage();
	MessageListItemClick(m_LasttalkCurSel.m_nType, m_LasttalkCurSel.m_strID = strID);

	//如果不想添加在列表后边，可选择添加到列表开头
	//m_pAdapterLasttalk->MoveItemToTop(strID);	

	//将滚动条滚动到刚添加的项
	m_pAdapterLasttalk->EnsureVisable(m_LasttalkCurSel.m_strID = strID);
	//设置将最新添加的项选中
	m_pAdapterLasttalk->SetCurSel(m_LasttalkCurSel.m_strID = strID);
}

void CMainDlg::ContactItemRClick(int nGID, const std::string& strID)
{
	MenuWrapper menu(L"menu_contact_list", L"SMENU");
	menu.AddMenu(_T("发送消息"), 1, TRUE, FALSE);
	menu.AddMenu(_T("分享该好友"), 2, TRUE, FALSE);
	menu.AddMenu(_T("你瞅啥"), 3, TRUE, FALSE);

	int ret = 0;
	POINT pt;
	::GetCursorPos(&pt);

	ret = menu.ShowMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
	switch (ret)
	{
	case 1:
		break;
	case 2:
		break;
	default:
		break;
	}
}

void CMainDlg::SearchResultItemDBClick(int nType, const std::string& strID)
{
	//4 group/5 personal
	if (nType == 1)
		ContactItemDBClick(5, strID);
	else if (nType == 2)
		ContactItemDBClick(4, strID);

	STabCtrl* pTab = FindChildByName2<STabCtrl>(L"leftlist_tabctrl");
	SASSERT(pTab);
	pTab->SetCurSel(m_nOldCurSel);

	SEdit *pEdit = FindChildByName2<SEdit>(L"search_edit");
	SASSERT(pEdit);
	pEdit->SetWindowText(L"");

	m_bResultEmpty = true;
}

void CMainDlg::EmotionTileViewItemClick(const std::string& strID)
{
	if (m_bEmotionShow)
	{
		m_pEmojiDlg->ShowWindow(SW_HIDE);
		m_bEmotionShow = false;
	}
	else
	{
		m_pEmojiDlg->ShowWindow(SW_SHOW);
		m_pEmojiDlg->SetNoSel();
		m_bEmotionShow = true;
	}

	std::map<std::string, std::string>::iterator iter = CGlobalUnits::GetInstance()->m_mapEmojisIndex.find(strID);
	if (iter != CGlobalUnits::GetInstance()->m_mapEmojisIndex.end())
	{
		std::string strEmotionName = iter->second;
		std::string strPath = "emojis\\" + strEmotionName;

		SStringW sstrPath = S_CA2W(strPath.c_str());
		SStringW sstrEmotion;
		sstrEmotion.Format(L"<RichEditContent>"
			L"<para break=\"0\" disable-layout=\"1\">"
			L"<img type=\"smiley_img\" path=\"%s\" id=\"zzz\" max-size=\"\" minsize=\"\" scaring=\"1\" cursor=\"hand\" />"
			L"</para>"
			L"</RichEditContent>", sstrPath);

		STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
		SASSERT(pChatTab);

		SStringW sstrID = S_CA2W(m_LasttalkCurSel.m_strID.c_str());
		SWindow* pPage = pChatTab->GetPage(sstrID, TRUE);
		SASSERT(pPage);
		SImRichEdit* pSendRichedit = pPage->FindChildByName2<SImRichEdit>(L"send_richedit");
		SASSERT(pSendRichedit);
		pSendRichedit->InsertContent(sstrEmotion, RECONTENT_CARET);
	}
}

void CMainDlg::OnBnClickSend()
{
	std::ostringstream os;
	STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
	SASSERT(pChatTab);

	SStringW sstrID = S_CA2W(m_LasttalkCurSel.m_strID.c_str());
	SWindow* pPage = pChatTab->GetPage(sstrID, TRUE);
	SASSERT(pPage);
	SImRichEdit* pRecvRichedit = pPage->FindChildByName2<SImRichEdit>(L"recv_richedit");
	SImRichEdit* pSendRichedit = pPage->FindChildByName2<SImRichEdit>(L"send_richedit");
	SASSERT(pRecvRichedit);
	SASSERT(pSendRichedit);

	CHARRANGE chr = {0, -1};
	SStringT strContent = pSendRichedit->GetSelectedContent(&chr);
	pugi::xml_document doc;
	if (!doc.load_buffer(strContent, strContent.GetLength() * sizeof(WCHAR)))
		return;
	strContent.Empty();

	std::vector<SStringW> vecContent;
	pugi::xml_node node = doc.child(L"RichEditContent").first_child();
	for (; node; node = node.next_sibling())
	{
		const wchar_t* pNodeName = node.name();
		if (wcscmp(RichEditText::GetClassName(), pNodeName) == 0)			//文本
		{
			SStringW sstrContent = node.text().get();
			SStringW sstrText;
			sstrText.Format(L"<text font-size=\"10\" font-face=\"微软雅黑\" color=\"#000000\"><![CDATA[%s]]></text>", sstrContent);
			vecContent.push_back(sstrText);
		}
		else if (wcscmp(RichEditImageOle::GetClassName(), pNodeName) == 0)	//图片
		{
			SStringW sstrImgPath = node.attribute(L"path").as_string();
			SStringW sstrImg;
			sstrImg.Format(L"<img subid=\"%s\" id=\"%s\" type=\"normal_img\" encoding=\"\" show-magnifier=\"1\" path=\"%s\"/>", L"", L"", sstrImgPath);
			vecContent.push_back(sstrImg);
		}
		else if (wcscmp(RichEditMetaFileOle::GetClassName(), pNodeName) == 0)	//文件
		{
			SStringW sstrFilePath = node.attribute(L"file").as_string();
			AddBackRightFileMessage(pRecvRichedit, sstrFilePath);

			time_t local;
			time(&local);
			os.str("");
			os<<local;
			std::string strTimestamp = os.str();
			std::map<std::string, std::string>::iterator iterTime = CGlobalUnits::GetInstance()->m_mapLasttalkTime.find(m_LasttalkCurSel.m_strID);
			if (iterTime != CGlobalUnits::GetInstance()->m_mapLasttalkTime.end())
			{
				CGlobalUnits::GetInstance()->m_mapLasttalkTime.erase(m_LasttalkCurSel.m_strID);
				CGlobalUnits::GetInstance()->m_mapLasttalkTime.insert(std::make_pair(m_LasttalkCurSel.m_strID, strTimestamp));
			}
			else
				CGlobalUnits::GetInstance()->m_mapLasttalkTime.insert(std::make_pair(m_LasttalkCurSel.m_strID, strTimestamp));
		}
		else
		{
			//其他未知消息类型
		}
	}

	if (!vecContent.empty())
	{
		AddBackRightGeneralMessage(pRecvRichedit, vecContent);

		time_t local;
		time(&local);
		os.str("");
		os<<local;
		std::string strTimestamp = os.str();
		std::map<std::string, std::string>::iterator iterTime = CGlobalUnits::GetInstance()->m_mapLasttalkTime.find(m_LasttalkCurSel.m_strID);
		if (iterTime != CGlobalUnits::GetInstance()->m_mapLasttalkTime.end())
		{
			CGlobalUnits::GetInstance()->m_mapLasttalkTime.erase(m_LasttalkCurSel.m_strID);
			CGlobalUnits::GetInstance()->m_mapLasttalkTime.insert(std::make_pair(m_LasttalkCurSel.m_strID, strTimestamp));
		}
		else
			CGlobalUnits::GetInstance()->m_mapLasttalkTime.insert(std::make_pair(m_LasttalkCurSel.m_strID, strTimestamp));
	}

	//刷新最近会话列表中的消息发送时间
	m_pAdapterLasttalk->UpdateData();

	pSendRichedit->Clear();
}

bool CMainDlg::OnSendRichEditAcceptData(SOUI::EventArgs* pEvt)
{
	EventQueryAccept * pev = (EventQueryAccept*)pEvt;
	if (pev->Conv->GetAvtiveFormat() == CF_HDROP)
	{
		::SetForegroundWindow(m_hWnd);
		RichFormatConv::DropFiles files = pev->Conv->GetDropFiles();
		DragDropFiles(files);
		return true;
	}
	return true;
}

bool CMainDlg::OnSendRichEditEditorChange(SOUI::EventArgs* pEvt)
{
	EventRENotify* pReNotify = (EventRENotify*)pEvt;
	if (pReNotify)
	{
		//处理输入框内容更改
	}
	return false;
}

bool CMainDlg::OnSendRichEditMenu(SOUI::EventArgs* pEvt)
{
	//发送框右键菜单事件响应
	SStringW sstrID = S_CA2W(m_LasttalkCurSel.m_strID.c_str());

	STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
	SASSERT(pChatTab);
	SWindow* pPage = pChatTab->GetPage(sstrID, TRUE);
	SASSERT(pPage);
	SImRichEdit* pSendRichEdit = pPage->FindChildByName2<SImRichEdit>(L"send_richedit");
	SASSERT(pSendRichEdit);

	CHARRANGE chrSel;
	pSendRichEdit->GetSel(&chrSel.cpMin, &chrSel.cpMax);
	RichEditOleBase* pOle = pSendRichEdit->GetSelectedOle();

	MenuWrapper menu(L"menu_send_richeidt", L"SMENU");
	BOOL enableCopy = (chrSel.cpMax - chrSel.cpMin) >= 1;
	menu.AddMenu(_T("复制"), 1, enableCopy, FALSE);
	menu.AddMenu(_T("剪切"), 2, enableCopy, FALSE);
	menu.AddMenu(_T("粘贴"), 3, TRUE, FALSE);

	int ret = 0;
	POINT pt;
	::GetCursorPos(&pt);

	ret = menu.ShowMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
	switch (ret)
	{
	case 1:
		pSendRichEdit->Copy();
		break;

	case 2:
		pSendRichEdit->Cut();
		break;

	case 3:
		pSendRichEdit->Paste();
		break;
	default:
		break;
	}

	return true;
}

bool CMainDlg::OnSendRichEditObjEvent(SOUI::EventArgs* pEvt)
{
	EventRichEditObj * pev = (EventRichEditObj*)pEvt;
	switch (pev->SubEventId)
	{
	case DBLCLICK_IMAGEOLE:
		{
			RichEditImageOle * pImageOle = sobj_cast<RichEditImageOle>(pev->RichObj);
			ShellExecute(NULL, _T("open"), pImageOle->GetImagePath(), NULL, NULL, SW_SHOW);
		}
		break;
	default:
		break;
	}
	return false;
}

bool CMainDlg::OnRecvRichEditAcceptData(SOUI::EventArgs *pEvt)
{
	EventQueryAccept* pev = (EventQueryAccept*)pEvt;
	if (pev)
	{
		//一般没处理需求
	}
	return true;
}

enum
{
	RECVMENU_SENDMSG = 0,
	RECVMENU_AUDIOCALL,
	RECVMENU_VIDEOCALL,
	RECVMENU_SHOWINFO,
	RECVMENU_AT,
	RECVMENU_REVOKE,
	RECVMENU_COPY,
	RECVMENU_COPYBUBBLE,
	RECVMENU_SAVEAS,
	RECVMENU_SELECTALL,
	RECVMENU_CLEAR,
	RECVMENU_OPENFILE,
	RECVMENU_OPENFILEDIR,
};

void CMainDlg::FillRClickAvatarMenu(MenuWrapper& menu, RichEditContent* pContent)
{
	menu.AddMenu(_T("发送消息"), RECVMENU_SENDMSG, TRUE, FALSE);
	menu.AddMenu(_T("语音通话"), RECVMENU_AUDIOCALL, FALSE, FALSE);
	menu.AddMenu(_T("视频通话"), RECVMENU_VIDEOCALL, FALSE, FALSE);
	menu.AddMenu(_T("查看资料"), RECVMENU_SHOWINFO, TRUE, FALSE);
	menu.AddMenu(_T("@TA"), RECVMENU_AT, TRUE, FALSE);
}

void CMainDlg::FillRClickImageMenu(MenuWrapper& menu, RichEditContent* pContent)
{
	if (pContent && pContent->GetAlign() == RichEditObj::ALIGN_RIGHT)
		menu.AddMenu(_T("撤回"), RECVMENU_REVOKE, FALSE, FALSE);

	menu.AddMenu(_T("复制"), RECVMENU_COPY, TRUE, FALSE);
	menu.AddMenu(_T("另存为"), RECVMENU_SAVEAS, TRUE, FALSE);
	menu.AddMenu(_T("全部选择"), RECVMENU_SELECTALL, TRUE, FALSE);
	menu.AddMenu(_T("清屏"), RECVMENU_CLEAR, TRUE, FALSE);
}

void CMainDlg::FillRClickFileMenu(MenuWrapper& menu, RichEditContent* pContent)
{
	if (pContent && pContent->GetAlign() == RichEditObj::ALIGN_RIGHT)
		menu.AddMenu(_T("撤回"), RECVMENU_REVOKE, FALSE, FALSE);

	menu.AddMenu(_T("打开"), RECVMENU_OPENFILE, TRUE, FALSE);
	menu.AddMenu(_T("打开文件夹"), RECVMENU_OPENFILEDIR, TRUE, FALSE);
	menu.AddMenu(_T("全部选择"), RECVMENU_SELECTALL, TRUE, FALSE);
	menu.AddMenu(_T("清屏"), RECVMENU_CLEAR, TRUE, FALSE);
}

void CMainDlg::FillRClickSelRegionMenu(MenuWrapper& menu, RichEditContent* pContent)
{
	if (pContent && pContent->GetAlign() == RichEditObj::ALIGN_RIGHT)
		menu.AddMenu(_T("撤回"), RECVMENU_REVOKE, FALSE, FALSE);

	menu.AddMenu(_T("复制"), RECVMENU_COPY, TRUE, FALSE);
	menu.AddMenu(_T("全部选择"), RECVMENU_SELECTALL, TRUE, FALSE);
	menu.AddMenu(_T("清屏"), RECVMENU_CLEAR, TRUE, FALSE);
}

void CMainDlg::FillRClickBubbleMenu(MenuWrapper& menu, RichEditContent* pContent)
{
	if (pContent && pContent->GetAlign() == RichEditObj::ALIGN_RIGHT)
		menu.AddMenu(_T("撤回"), RECVMENU_REVOKE, FALSE, FALSE);

	menu.AddMenu(_T("复制"), RECVMENU_COPYBUBBLE, TRUE, FALSE);
	menu.AddMenu(_T("全部选择"), RECVMENU_SELECTALL, TRUE, FALSE);
	menu.AddMenu(_T("清屏"), RECVMENU_CLEAR, TRUE, FALSE);
}

void CMainDlg::FillRClickNothingMenu(MenuWrapper& menu)
{
	menu.AddMenu(_T("全部选择"), RECVMENU_SELECTALL, TRUE, FALSE);
	menu.AddMenu(_T("清屏"), RECVMENU_CLEAR, TRUE, FALSE);
}

bool CMainDlg::OnRecvRichEditMenu(SOUI::EventArgs *pEvt)
{
	EventCtxMenu* pev = static_cast<EventCtxMenu*>(pEvt);
	if (pev)
	{
		STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
		SASSERT(pChatTab);

		SStringW sstrID = S_CA2W(m_LasttalkCurSel.m_strID.c_str());
		SWindow* pPage = pChatTab->GetPage(sstrID, TRUE);
		SImRichEdit* pRecvRichedit = pPage->FindChildByName2<SImRichEdit>(L"recv_richedit");
		SASSERT(pRecvRichedit);

		CHARRANGE selRange;
		pRecvRichedit->GetSel(&selRange.cpMin, &selRange.cpMax);
		int selectedCount = selRange.cpMax - selRange.cpMin;

		// 如果鼠标没有落在选中区域上，需要取消选中
		if (selectedCount > 0)
		{
			int cp = pRecvRichedit->CharFromPos(pev->pt);
			if (cp < selRange.cpMin || cp > selRange.cpMax)
			{
				pRecvRichedit->SetSel(cp, cp);
				selectedCount = 0;
			}
		}

		//对于右键在选择区域上的各种判断
		RichEditObj * pHitTestObj = pRecvRichedit->HitTest(pev->pt);
		RichEditObj * pObj = pHitTestObj;
		RichEditContent * pContent = sobj_cast<RichEditContent>(pObj);
		RichEditImageOle* pImageOle = NULL;
		RichEditFileOle* pFileOle = NULL;

		MenuWrapper menu(L"menu_recv_richeidt", L"SMENU");

		CHARRANGE          chr = { 0, -1 };
		IDataObject      * pobj;
		RichFormatConv     conv;
		SStringW           str;

		//弹出菜单
		int ret = 0;
		POINT pt;
		::GetCursorPos(&pt);

		if (pContent == NULL || pHitTestObj == NULL)
		{
			if (selectedCount > 0)
				FillRClickSelRegionMenu(menu, pContent);
			else
				FillRClickNothingMenu(menu);
			ret = menu.ShowMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
		}
		else if (pHitTestObj->IsClass(RichEditBkElement::GetClassName()) && pHitTestObj->GetData() == _T("avatar"))
		{
			FillRClickAvatarMenu(menu, pContent);
			ret = menu.ShowMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
		}
		else if (pHitTestObj->IsClass(RichEditImageOle::GetClassName()))
		{
			pImageOle = static_cast<RichEditImageOle*>(pHitTestObj);
			if (pImageOle->GetImageType() == L"normal_img")
				FillRClickImageMenu(menu, pContent);
			else
				FillRClickSelRegionMenu(menu, pContent);
			ret = menu.ShowMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
		}
		else if (pHitTestObj->IsClass(RichEditFileOle::GetClassName()))
		{
			pFileOle = static_cast<RichEditFileOle*>(pHitTestObj);
			FillRClickFileMenu(menu, pContent);
			ret = menu.ShowMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
		}
		else if (pHitTestObj->IsClass(RichEditReminderOle::GetClassName()))
		{
			FillRClickSelRegionMenu(menu, pContent);
			ret = menu.ShowMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
		}
		else if (pHitTestObj->IsClass(RichEditBkElement::GetClassName()) && pHitTestObj->GetData() == _T("bubble"))
		{
			if (selectedCount > 0)
				FillRClickSelRegionMenu(menu, pContent);
			else
				FillRClickBubbleMenu(menu, pContent);
			ret = menu.ShowMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
		}
		else
		{
			FillRClickNothingMenu(menu);
			ret = menu.ShowMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
		}

		//处理菜单消息
		switch (ret)
		{
		case RECVMENU_COPY:
			pRecvRichedit->Copy();
			break;
		case RECVMENU_SELECTALL:
			pRecvRichedit->SelectAll();
			break;
		case RECVMENU_SAVEAS:
			{
				if (pImageOle)
				{
					//图片另存为
					::MessageBox(m_hWnd, _T("点击了图片另存为菜单"), _T("提示"), MB_OK);
				}
			}
			break;
		case RECVMENU_OPENFILE:
			{
				if (pFileOle)
					ShellExecute(NULL, _T("open"), pFileOle->GetFilePath(), NULL, NULL, SW_SHOW);
			}
			break;
		case RECVMENU_OPENFILEDIR:
			{
				if (pFileOle)
				{
					SStringW param;
					param.Format(_T("/select,\"%s\""), pFileOle->GetFilePath());
					ShellExecute(NULL, _T("open"), _T("explorer.exe"), param, NULL, SW_SHOW);
				}
			}
			break;
		case RECVMENU_COPYBUBBLE:
			{
				if (pContent)
				{
					pObj = pContent->GetById(_T("msgbody"));
					chr = pObj->GetCharRange();
					str = pRecvRichedit->GetSelectedContent(&chr);
					conv.InitFromRichContent(str);
					conv.ToDataObject(&pobj);
					OleSetClipboard(pobj);
					OleFlushClipboard();
					pobj->Release();
				}
			}
			break;
		case RECVMENU_CLEAR:
			{
				pRecvRichedit->Clear();
				AddFetchMoreBlock(pRecvRichedit);
			}
			break;
		case RECVMENU_AUDIOCALL:
			::MessageBox(m_hWnd, _T("点击了语音通话菜单"), _T("提示"), MB_OK);
			break;
		case RECVMENU_VIDEOCALL:
			::MessageBox(m_hWnd, _T("点击了视频通话菜单"), _T("提示"), MB_OK);
			break;
		case RECVMENU_SHOWINFO:
			::MessageBox(m_hWnd, _T("点击了查看资料菜单"), _T("提示"), MB_OK);
			break;
		case RECVMENU_AT:
			::MessageBox(m_hWnd, _T("点击了AT菜单"), _T("提示"), MB_OK);
			break;
		case RECVMENU_REVOKE:
			::MessageBox(m_hWnd, _T("点击了消息撤回菜单"), _T("提示"), MB_OK);
			break;
		default:
			break;
		}
	}
	return true;
}

bool CMainDlg::OnRecvRichEditObjEvent(SOUI::EventArgs *pEvt)
{
	return true;
}

bool CMainDlg::OnRecvRichEditScrollEvent(SOUI::EventArgs *pEvt)
{
	return true;
}

void CMainDlg::DragDropFiles(RichFormatConv::DropFiles& files)
{
	SStringW sstrID = S_CA2W(m_LasttalkCurSel.m_strID.c_str());

	STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
	SASSERT(pChatTab);
	SWindow* pPage = pChatTab->GetPage(sstrID, TRUE);
	SASSERT(pPage);
	SImRichEdit* pSendRichEdit = pPage->FindChildByName2<SImRichEdit>(L"send_richedit");
	SASSERT(pSendRichEdit);

	RichFormatConv::DropFiles::iterator iter = files.begin();
	for (; iter != files.end(); ++iter)
	{
		SStringW strFile = *iter;
		int nFileSize;
		FILE* fp = _wfopen(strFile, L"rb");
		if (fp)
		{
			fseek(fp, 0L, SEEK_END);
			nFileSize = ftell(fp);
			rewind(fp);
			fclose(fp);
		}
		else
			return;

		//可对发送的文件大小做个限制
// 		if(nFileSize >= MAX_FILESIZE)
// 		{
// 			::MessageBox(this->m_hWnd, L"仅支持4M以下的文件，该文件大于4M", L"提示", MB_OK);
// 			return;
// 		}

		SStringW sstrContent;
		sstrContent.Format(L"<RichEditContent><metafile file=\"%s\" /></RichEditContent>", *iter);
		pSendRichEdit->InsertContent(sstrContent, RECONTENT_CARET);
	}

	::SetFocus(m_hWnd);
	pSendRichEdit->SetFocus();
}

void CMainDlg::OnBnClickChatEmotion()
{
	SetMsgHandled(FALSE);
	SImageButton* pBtn = FindChildByName2<SImageButton>(L"btn_emotion");
	SASSERT(pBtn);
	CRect rcEmotionBtn = pBtn->GetWindowRect();
	ClientToScreen(&rcEmotionBtn);
	::SetWindowPos(m_pEmojiDlg->m_hWnd, NULL, rcEmotionBtn.left - 10, rcEmotionBtn.top - 250, 0,0, SWP_NOSIZE);

	if (m_bEmotionShow)
	{
		m_pEmojiDlg->ShowWindow(SW_HIDE);
		m_bEmotionShow = false;
	}
	else
	{
		m_pEmojiDlg->ShowWindow(SW_SHOW);
		m_pEmojiDlg->SetNoSel();
		m_bEmotionShow = true;
	}
}

void CMainDlg::OnBnClickChatImage()
{
	SStringW strFile;
	CFileDialogEx openDlg(TRUE,_T("图片"),0,6,
		_T("图片文件\0*.gif;*.bmp;*.jpg;*.png\0\0"));
	if (openDlg.DoModal() == IDOK)
	{
		strFile = openDlg.m_szFileName;

		int nFileSize;
		FILE* fp = _wfopen(strFile, L"rb");
		if (fp)
		{
			fseek(fp, 0L, SEEK_END);
			nFileSize = ftell(fp);
			rewind(fp);
			fclose(fp);
		}
		else
			return;

		//发送图片限制大小
// 		if(nFileSize >= MAX_FILESIZE)
// 		{
// 			::MessageBox(this->m_hWnd, L"仅支持4M以下的图片，该图片大于4M", L"提示", MB_OK);
// 			return;
// 		}

		SStringW str;
		str.Format(L"<RichEditContent>"
			L"<para break=\"0\" disable-layout=\"1\">"
			L"<img type=\"normal_img\" path=\"%s\" id=\"zzz\" max-size=\"\" minsize=\"\" scaring=\"1\" cursor=\"hand\" />"
			L"</para>"
			L"</RichEditContent>", strFile);

		STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
		SASSERT(pChatTab);

		SStringW sstrID = S_CA2W(m_LasttalkCurSel.m_strID.c_str());

		SWindow* pPage = pChatTab->GetPage(sstrID, TRUE);
		SImRichEdit* pSendRichedit = pPage->FindChildByName2<SImRichEdit>(L"send_richedit");
		pSendRichedit->InsertContent(str, RECONTENT_CARET);
	}
}

void CMainDlg::OnBnClickChatFile()
{
	SStringW strFile;
	CFileDialogEx openDlg(TRUE,_T("文件"),0,6,
		_T("文件\0*.*\0\0"));
	if (openDlg.DoModal() == IDOK)
	{
		strFile = openDlg.m_szFileName;
		int nFileSize;
		FILE* fp = _wfopen(strFile, L"rb");
		if (fp)
		{
			fseek(fp, 0L, SEEK_END);
			nFileSize = ftell(fp);
			rewind(fp);
			fclose(fp);
		}
		else
			return;

		//发送图片限制大小
// 		if(nFileSize >= MAX_FILESIZE)
// 		{
// 			::MessageBox(this->m_hWnd, L"仅支持4M以下的文件，该文件大于4M", L"提示", MB_OK);
// 			return;
// 		}

		SStringW str;
		str.Format(L"<RichEditContent>"
			L"<metafile file=\"%s\" />"
			L"</RichEditContent>", strFile);

		STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
		SASSERT(pChatTab);

		SStringW sstrID = S_CA2W(m_LasttalkCurSel.m_strID.c_str());

		SWindow* pPage = pChatTab->GetPage(sstrID, TRUE);
		SImRichEdit* pSendRichedit = pPage->FindChildByName2<SImRichEdit>(L"send_richedit");
		pSendRichedit->InsertContent(str, RECONTENT_CARET);
	}
}

void CMainDlg::OnBnClickChatCapture()
{
	if (m_bSnapshotHideWindow)
	{
		this->ShowWindow(SW_HIDE);
	}

	CSnapshotDlg dlg;
	CWindowEnumer::EnumAllTopWindow();
	dlg.DoModal(NULL);
}

void CMainDlg::OnBnClickSettingCapture()
{
	//点击截图旁的箭头
	MenuWrapper menu(L"menu_snapshot_arrow", L"SMENU");
	menu.AddMenu(L"屏幕截图", 2001, TRUE, FALSE);
	menu.AddMenu(L"截图隐藏当前窗口", 2002, TRUE, FALSE);

	MenuItemWrapper* pItemCapture = menu.GetMenuItemById(2001);
	MenuItemWrapper* pItemHideWindow = menu.GetMenuItemById(2002);

	if (m_bSnapshotHideWindow)
		pItemHideWindow->SetCheck(TRUE);

	int ret = 0;
	POINT pt;
	::GetCursorPos(&pt);
	ret = menu.ShowMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd);
	switch (ret)
	{
	case 2001:
			OnBnClickChatCapture();
		break;

	case 2002:
		{
			if (m_bSnapshotHideWindow)
				m_bSnapshotHideWindow = false;
			else
				m_bSnapshotHideWindow = true;
		}
		break;

	default:
		break;
	}
}

void CMainDlg::OnBnClickChatHistory()
{
	//
}

void CMainDlg::AddBackRightGeneralMessage(SImRichEdit* pRecvRichEdit, const std::vector<SStringW>& vecContents)
{
	//统一使用右侧布局
	LPCWSTR pEmpty;
	pEmpty = L"<para id=\"msgbody\" margin=\"0,0,0,0\" break=\"1\" simulate-align=\"1\">"		
		L""
		L"</para>";

	//插入消息时间
	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	SStringW sstrTempTime;
	if (t->tm_hour > 12)
		sstrTempTime.Format(L"%02d月%02d日  下午%02d:%02d", t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
	else
		sstrTempTime.Format(L"%02d月%02d日  上午%02d:%02d", t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
	SStringW sstrTime;
	sstrTime.Format(L"<text font-size=\"8\" color=\"#808080\">%s</text>", sstrTempTime);

	SStringW sstrTimeContent;
	sstrTimeContent.Format(
		L"<RichEditContent type=\"ContentCenter\" >"
		L"<para margin=\"100,15,100,0\" align=\"center\" break=\"1\" >"
		L"%s"
		L"</para>"
		L"</RichEditContent>", sstrTime);
	pRecvRichEdit->InsertContent(sstrTimeContent, RECONTENT_LAST);

	SStringW sstrResend;
	sstrResend = L"<bkele id=\"resend\" name=\"BkEleSendFail\" data=\"resend\" right-skin=\"skin.richedit_resend\" right-pos=\"{-15,[-16,@12,@12\" cursor=\"hand\" interactive=\"1\"/>";

	SStringW sstrMsg;
	for (int i = 0; i < vecContents.size(); i++)
	{
		sstrMsg += vecContents[i];
	}

	SStringW sstrContent;
	sstrContent.Format(
		L"<RichEditContent  type=\"ContentRight\" align=\"right\" auto-layout=\"1\">"
		L"<para break=\"1\" align=\"left\" />"
		L"<bkele data=\"avatar\" id=\"%s\" skin=\"%s\" left-pos=\"0,]-6,@40,@40\" right-pos=\"-50,]-9,@40,@40\" cursor=\"hand\" interactive=\"1\"/>"
		L"<para id=\"msgbody\" margin=\"65,0,35,0\" break=\"1\" simulate-align=\"1\">"		
		L"%s"
		L"</para>"
		L"<bkele data=\"bubble\" left-skin=\"skin_left_bubble\" right-skin=\"skin_right_otherbubble\" left-pos=\"50,{-9,[10,[10\" right-pos=\"{-10,{-9,-55,[10\" />"
		L"%s"
		L"</RichEditContent>",
		L"default_portrait", L"default_portrait", sstrMsg, pEmpty);

	pRecvRichEdit->InsertContent(sstrContent, RECONTENT_LAST);
	pRecvRichEdit->ScrollToBottom();

	RichEditBkElement* pResendEleObj = sobj_cast<RichEditBkElement>(pRecvRichEdit->GetElementById(L"resend"));
	if (pResendEleObj)
	{
		pResendEleObj->SetVisible(FALSE);
		pRecvRichEdit->Invalidate();
	}

	AddBackLeftGeneralMessage(pRecvRichEdit, vecContents);
}

void CMainDlg::AddBackRightFileMessage(SImRichEdit* pRecvRichEdit, const SStringW& sstrFilePath)
{
	//统一使用右侧布局
	LPCWSTR pEmpty;
	pEmpty = L"<para id=\"msgbody\" margin=\"0,0,0,0\" break=\"1\" simulate-align=\"1\">"		
		L""
		L"</para>";

	//添加时间
	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	SStringW sstrTempTime;
	if (t->tm_hour > 12)
		sstrTempTime.Format(L"%02d月%02d日  下午%02d:%02d", t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
	else
		sstrTempTime.Format(L"%02d月%02d日  上午%02d:%02d", t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
	SStringW sstrTime;
	sstrTime.Format(L"<text font-size=\"8\" color=\"#808080\">%s</text>", sstrTempTime);

	SStringW sstrTimeContent;
	sstrTimeContent.Format(
		L"<RichEditContent type=\"ContentCenter\" >"
		L"<para margin=\"100,15,100,0\" align=\"center\" break=\"1\" >"
		L"%s"
		L"</para>"
		L"</RichEditContent>", sstrTime);
	pRecvRichEdit->InsertContent(sstrTimeContent, RECONTENT_LAST);

	SStringW sstrResend;
	sstrResend = L"<bkele id=\"resend\" name=\"BkEleSendFail\" data=\"resend\" right-skin=\"skin.richedit_resend\" right-pos=\"{-15,[-16,@12,@12\" cursor=\"hand\" interactive=\"1\"/>";

	std::string strFilePath = S_CW2A(sstrFilePath);
	//获取文件大小
	FILE* fp = fopen(strFilePath.c_str(), "rb");
	SASSERT(fp);
	int nFileSize = 0;
	fseek(fp, 0L, SEEK_END);
	nFileSize = ftell(fp);
	rewind(fp);
	fclose(fp);

	//获取文件后缀名
	std::string strFileSuffix;
	std::string::size_type pos = strFilePath.find_last_of(".");
	if (pos != std::string::npos)
	{
		strFileSuffix = strFilePath.substr(pos + 1);
	}

	SStringW sstrFileType = S_CA2W(strFileSuffix.c_str());

	SStringW sstrFile;
	int nVisableLinks = 0x0010 | 0x0020;
	sstrFile.Format(L"<file id=\"file_file\" file-size=\"%d\" file-state=\"上传成功\" file-suffix=\"%s\" file-path=\"%s\" links=\"%d\"/>",
		nFileSize,
		sstrFileType,
		sstrFilePath,
		nVisableLinks);

	SStringW sstrContent;
	sstrContent.Format(
		L"<RichEditContent  type=\"ContentRight\" align=\"right\" auto-layout=\"1\">"
		L"<para break=\"1\" align=\"left\" />"
		L"<bkele data=\"avatar\" id=\"%s\" skin=\"%s\" left-pos=\"0,]-6,@40,@40\" right-pos=\"-50,]-10,@40,@40\" cursor=\"hand\" interactive=\"1\"/>"
		L"<para data=\"file_file\" id=\"msgbody\" margin=\"65,0,45,0\" break=\"1\" simulate-align=\"1\">"		
		L"%s"
		L"</para>"
		L"<bkele data=\"bubble\" left-skin=\"skin_left_bubble\" right-skin=\"skin_right_otherbubble\" left-pos=\"50,{-9,[10,[10\" right-pos=\"{-10,{-9,-55,[10\" />"
		L"%s"
		L"</RichEditContent>",
		L"default_portrait", L"default_portrait", sstrFile, pEmpty);

	pRecvRichEdit->InsertContent(sstrContent, RECONTENT_LAST);
	pRecvRichEdit->ScrollToBottom();

	RichEditBkElement* pResendEleObj = sobj_cast<RichEditBkElement>(pRecvRichEdit->GetElementById(L"resend"));
	if (pResendEleObj)
	{
		pResendEleObj->SetVisible(FALSE);
		pRecvRichEdit->Invalidate();
	}

	AddBackLeftFileMessage(pRecvRichEdit, sstrFilePath);
}

void CMainDlg::AddBackLeftGeneralMessage(SImRichEdit* pRecvRichEdit, const std::vector<SStringW>& vecContents)
{
	LPCWSTR pEmpty;
	pEmpty = L"<para id=\"msgbody\" margin=\"0,0,0,0\" break=\"1\" simulate-align=\"1\">"		
		L""
		L"</para>";

	//插入消息时间
	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	SStringW sstrTempTime;
	if (t->tm_hour > 12)
		sstrTempTime.Format(L"%02d月%02d日  下午%02d:%02d", t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
	else
		sstrTempTime.Format(L"%02d月%02d日  上午%02d:%02d", t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
	SStringW sstrTime;
	sstrTime.Format(L"<text font-size=\"8\" color=\"#808080\">%s</text>", sstrTempTime);

	SStringW sstrTimeContent;
	sstrTimeContent.Format(
		L"<RichEditContent type=\"ContentCenter\" >"
		L"<para margin=\"100,15,100,0\" align=\"center\" break=\"1\" >"
		L"%s"
		L"</para>"
		L"</RichEditContent>", sstrTime);
	pRecvRichEdit->InsertContent(sstrTimeContent, RECONTENT_LAST);

	SStringW sstrMsg;
	for (int i = 0; i < vecContents.size(); i++)
	{
		sstrMsg += vecContents[i];
	}

	SStringW sstrContent;
	sstrContent.Format(
		L"<RichEditContent  type=\"ContentLeft\" align=\"left\">"
		L"<para break=\"1\" align=\"left\" />"
		L"<bkele data=\"avatar\" id=\"%s\" skin=\"%s\" pos=\"0,]-10,@40,@40\" cursor=\"hand\" interactive=\"1\"/>"
		L"<para id=\"msgbody\" margin=\"65,0,45,0\" break=\"1\" simulate-align=\"1\">"		
		L"%s"
		L"</para>"
		L"<bkele data=\"bubble\" left-skin=\"skin_left_bubble\" left-pos=\"50,{-9,[10,[10\" />"
		L"%s"
		L"</RichEditContent>",
		L"default_portrait", L"default_portrait", sstrMsg, pEmpty);

	pRecvRichEdit->InsertContent(sstrContent, RECONTENT_LAST);
	pRecvRichEdit->ScrollToBottom();
}

void CMainDlg::AddBackLeftFileMessage(SImRichEdit* pRecvRichEdit, const SStringW& sstrFilePath)
{
	LPCWSTR pEmpty;
	pEmpty = L"<para id=\"msgbody\" margin=\"0,0,0,0\" break=\"1\" simulate-align=\"1\">"		
		L""
		L"</para>";

	//添加时间
	time_t tt = time(NULL);
	tm* t = localtime(&tt);
	SStringW sstrTempTime;
	if (t->tm_hour > 12)
		sstrTempTime.Format(L"%02d月%02d日  下午%02d:%02d", t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
	else
		sstrTempTime.Format(L"%02d月%02d日  上午%02d:%02d", t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
	SStringW sstrTime;
	sstrTime.Format(L"<text font-size=\"8\" color=\"#808080\">%s</text>", sstrTempTime);

	SStringW sstrTimeContent;
	sstrTimeContent.Format(
		L"<RichEditContent type=\"ContentCenter\" >"
		L"<para margin=\"100,15,100,0\" align=\"center\" break=\"1\" >"
		L"%s"
		L"</para>"
		L"</RichEditContent>", sstrTime);
	pRecvRichEdit->InsertContent(sstrTimeContent, RECONTENT_LAST);

	std::string strFilePath = S_CW2A(sstrFilePath);
	//获取文件大小
	FILE* fp = fopen(strFilePath.c_str(), "rb");
	SASSERT(fp);
	int nFileSize = 0;
	fseek(fp, 0L, SEEK_END);
	nFileSize = ftell(fp);
	rewind(fp);
	fclose(fp);

	//获取文件后缀名
	std::string strFileSuffix;
	std::string::size_type pos = strFilePath.find_last_of(".");
	if (pos != std::string::npos)
	{
		strFileSuffix = strFilePath.substr(pos + 1);
	}

	SStringW sstrFileType = S_CA2W(strFileSuffix.c_str());

	SStringW sstrFile;
	int nVisableLinks = 0x0010 | 0x0020;
	sstrFile.Format(L"<file id=\"file_file\" file-size=\"%d\" file-state=\"上传成功\" file-suffix=\"%s\" file-path=\"%s\" links=\"%d\"/>",
		nFileSize,
		sstrFileType,
		sstrFilePath,
		nVisableLinks);

	SStringW sstrContent;
	sstrContent.Format(
		L"<RichEditContent  type=\"ContentLeft\" align=\"left\">"
		L"<para break=\"1\" align=\"left\" />"
		L"<bkele data=\"avatar\" id=\"%s\" skin=\"%s\" pos=\"0,]-10,@40,@40\" cursor=\"hand\" interactive=\"1\"/>"
		L"<para id=\"msgbody\" margin=\"65,0,45,0\" break=\"1\" simulate-align=\"1\">"			
		L"%s"
		L"</para>"
		L"<bkele data=\"bubble\" left-skin=\"skin_left_bubble\" left-pos=\"50,{-9,[10,[10\" />"
		L"%s"
		L"</RichEditContent>",
		L"default_portrait", L"default_portrait", sstrFile, pEmpty);

	pRecvRichEdit->InsertContent(sstrContent, RECONTENT_LAST);
	pRecvRichEdit->ScrollToBottom();
}

void CMainDlg::AddBackSysMessage(SImRichEdit* pRecvRichEdit, const SStringW& sstrContent)
{
	//
}

void CMainDlg::AddFetchMoreBlock(SImRichEdit* pRecvRichEdit)
{
	SStringW content = L"<RichEditContent type=\"ContentFetchMore\" align=\"center\">"
		L"<para name=\"para\" margin=\"0,5,0,0\" break=\"1\">"
		L"<fetchmore name=\"FetchMoreOle\" selectable=\"0\" />"
		L"</para>"
		L"</RichEditContent>";

	pRecvRichEdit->InsertContent(content);
}

bool CMainDlg::OnEditSearchSetFocus(EventArgs* pEvt)
{
	STabCtrl* pTab = FindChildByName2<STabCtrl>(L"leftlist_tabctrl");
	SASSERT(pTab);
	m_nOldCurSel = pTab->GetCurSel();
	pTab->SetCurSel(L"search_result", TRUE);

	return true;
}

bool CMainDlg::OnEditSearchKillFocus(EventArgs* pEvt)
{
	if (m_bResultEmpty)
	{
		STabCtrl* pTab = FindChildByName2<STabCtrl>(L"leftlist_tabctrl");
		SASSERT(pTab);
		pTab->SetCurSel(m_nOldCurSel);
	}

	return true;
}

bool CMainDlg::OnEditSearchChanged(EventArgs *e)
{
	EventRENotify *e2 = sobj_cast<EventRENotify>(e);
	if(e2->iNotify != EN_CHANGE) 
		return false;

	SEdit *pEdit = sobj_cast<SEdit>(e->sender);
	SStringW sstrInput = pEdit->GetWindowText();
	std::wstring wstrInput = sstrInput;

	std::vector<SEARCH_INFO> vecSearchResult;
	if (L"" != sstrInput)
	{
		m_bResultEmpty = false;
		if (!CGlobalUnits::GetInstance()->IsIncludeChinese(wstrInput))	//拼音搜索
		{
			SearchInfosMap::iterator iter = CGlobalUnits::GetInstance()->m_mapPinyinSearch.begin();
			for (; iter != CGlobalUnits::GetInstance()->m_mapPinyinSearch.end(); iter++)
			{
				std::wstring wstrName = iter->first;
				if (wstrName.find(wstrInput) != std::string::npos)
					vecSearchResult.push_back(SEARCH_INFO(iter->second.m_nType, iter->second.m_strID));
			}
			//去重
			std::sort(vecSearchResult.begin(), vecSearchResult.end());
			vecSearchResult.erase(unique(vecSearchResult.begin(), vecSearchResult.end()), vecSearchResult.end());
		}
		else	//汉字搜索
		{
			SearchInfosMap::iterator iter = CGlobalUnits::GetInstance()->m_mapChineseSearch.begin();
			for (; iter != CGlobalUnits::GetInstance()->m_mapChineseSearch.end(); iter++)
			{
				std::wstring wstrName = iter->first;
				if (wstrName.find(wstrInput) != std::string::npos)
					vecSearchResult.push_back(SEARCH_INFO(iter->second.m_nType, iter->second.m_strID));
			}
			//去重
			sort(vecSearchResult.begin(), vecSearchResult.end());
			vecSearchResult.erase(unique(vecSearchResult.begin(), vecSearchResult.end()), vecSearchResult.end());
		}

		m_pAdapterSearchResult->DeleteAllItem();
		for (int i = 0; i < vecSearchResult.size(); i++)
		{
			m_pAdapterSearchResult->AddItem(vecSearchResult[i].m_nType, vecSearchResult[i].m_strID);
		}
	}
	else
	{
		m_pAdapterSearchResult->DeleteAllItem();
		m_bResultEmpty = true;
	}

	return true;
}

bool CMainDlg::OnEventSnapshotFinish(EventArgs* e)
{
	if (m_bSnapshotHideWindow)
		this->ShowWindow(SW_SHOWNORMAL);
	
	STabCtrl* pChatTab = FindChildByName2<STabCtrl>(L"chattab");
	SASSERT(pChatTab);

	SStringW sstrID = S_CA2W(m_LasttalkCurSel.m_strID.c_str());
	if (0 == m_LasttalkCurSel.m_nType || 1 == m_LasttalkCurSel.m_nType || 2 == m_LasttalkCurSel.m_nType)
	{
		SWindow* pPage = pChatTab->GetPage(sstrID, TRUE);
		SImRichEdit* pSendRichedit = pPage->FindChildByName2<SImRichEdit>(L"send_richedit");
		SASSERT(pSendRichedit);

		pSendRichedit->Paste();
		pSendRichedit->SetFocus();
	}
	return true;
}