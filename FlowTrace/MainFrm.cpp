// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "FlowTraceView.h"
#include "MainFrm.h"
#include "Settings.h"
#include "Archive.h"
#include "DlgSettings.h"
#include "DlgDetailes.h"
#include "DlgProgress.h"
#include "SearchInfo.h"
#include "DlgModules.h"
#include "DlgFilters.h"
#include "LogReceiver.h"

HWND       hwndMain;
WNDPROC oldEditProc;
LRESULT CALLBACK subEditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

CMainFrame::CMainFrame() :
      m_list(m_view.list())
    , m_tree(m_view.tree())
    , m_backTrace(m_view.backTrace())
{
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
        return TRUE;

    return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
    UIEnable(ID_SEARCH_PREV, searchInfo.total); // && (searchInfo.cur > 0)
    UIEnable(ID_SEARCH_NEXT, searchInfo.total); // && (searchInfo.cur + 1 < searchInfo.total)
    UIEnable(ID_SEARCH_FIRST, searchInfo.total); //  && (searchInfo.cur > 0)
    UIEnable(ID_SEARCH_LAST, searchInfo.total); //  && (searchInfo.cur + 1 != searchInfo.total)
    UIEnable(ID_SEARCH_REFRESH, 1);
    UIEnable(ID_SEARCH_CLEAR, 1);
    UIEnable(ID_VIEW_PAUSERECORDING, gLogReceiver.working());
    UIEnable(ID_VIEW_STARTRECORDIG, !gLogReceiver.working());
    UIEnable(ID_EDIT_SELECTALL, !gArchive.IsEmpty());
    UIEnable(ID_EDIT_FIND32798, !gArchive.IsEmpty());
    UIEnable(ID_EDIT_COPY, m_list.HasSelection() || ::GetFocus() == m_tree.m_hWnd);
    UISetCheck(ID_VIEW_SHOW_HIDE_TREE, gSettings.GetTreeViewHiden() == 0);
    UISetCheck(ID_VIEW_SHOW_HIDE_STACK, !gSettings.GetInfoHiden());
    UISetCheck(ID_VIEW_SHOW_HIDE_FLOW_TRACES, gSettings.GetFlowTracesHiden() == 0);
    UIUpdateToolBar();
    return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    hwndMain = m_hWnd;
    m_lostIcon = LoadIconA(NULL, MAKEINTRESOURCE(32515));
    // create command bar window
    HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
    // attach menu
    m_CmdBar.AttachMenu(GetMenu());
    // load command bar images
    m_CmdBar.LoadImages(IDR_MAINFRAME);
    // remove old menu
    SetMenu(NULL);


    CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
    AddSimpleReBarBand(hWndCmdBar);

    HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
    m_searchbar = hWndToolBar;

    AddSimpleReBarBand(hWndToolBar, NULL, FALSE);
    SizeSimpleReBarBands();
    if (1)
    {
        ////////////////
        TBBUTTONINFO tbi;
        tbi.cbSize = sizeof(TBBUTTONINFO);
        tbi.dwMask = TBIF_SIZE | TBIF_STATE | TBIF_STYLE;

        // Make sure the underlying button is disabled
        tbi.fsState = 0;
        // BTNS_SHOWTEXT will allow the button size to be altered
        tbi.fsStyle = BTNS_SHOWTEXT;
        tbi.cx = static_cast<WORD>(25 * ::GetSystemMetrics(SM_CXSMICON));

        m_searchbar.SetButtonInfo(ID_SEARCH_COMBO, &tbi);

        // Get the button rect
        CRect rcCombo, rcButton0, rcSearcResult;
        m_searchbar.GetItemRect(0, rcButton0);

        rcSearcResult = rcButton0;
        rcSearcResult.top += 4;
        rcSearcResult.right = rcSearcResult.left + 4 * GetSystemMetrics(SM_CXSMICON) - 4;
        m_searchResult.Create(m_hWnd, rcSearcResult, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_RIGHT);// | WS_BORDER
        m_searchResult.SetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT), FALSE);
        m_searchResult.SetParent(hWndToolBar);

        rcCombo = rcButton0;
        rcCombo.left += rcSearcResult.Width() + 4;

        // create search bar combo
        DWORD dwComboStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL;

        m_cb.Create(m_hWnd, rcCombo, NULL, dwComboStyle);
        m_cb.SetParent(hWndToolBar);

        // set 25 lines visible in combo list
        m_cb.GetComboCtrl().ResizeClient(rcCombo.Width(), rcCombo.Height() + 25 * m_cb.GetItemHeight(0));
        m_searchbox = m_cb.GetComboCtrl();
        m_searchedit = m_cb.GetEditCtrl();
        oldEditProc = (WNDPROC)m_searchedit.SetWindowLongPtr(GWLP_WNDPROC, (LONG_PTR)subEditProc);
        searchInfo.hwndEdit = m_searchedit;

        //LoadSearchList();

        //m_searchedit.SetCueBannerText(L"Search...");

        // The combobox might not be centred vertically, and we won't know the
        // height until it has been created.  Get the size now and see if it
        // needs to be moved.
        CRect rectToolBar;
        CRect rectCombo;
        m_searchbar.GetClientRect(&rectToolBar);
        m_cb.GetWindowRect(rectCombo);

        // Get the different between the heights of the toolbar and
        // the combobox
        int nDiff = rectToolBar.Height() - rectCombo.Height();
        // If there is a difference, then move the combobox
        if (nDiff > 1)
        {
            m_searchbar.ScreenToClient(&rectCombo);
            m_cb.MoveWindow(rectCombo.left, rectCombo.top + (nDiff / 2), rectCombo.Width(), rectCombo.Height());
        }
        ////////////////  
    }

	//if (!gSettings.GetUseAdb())
		m_searchbar.HideButton(ID_VIEW_RESETLOG);

    SizeSimpleReBarBands();

    CreateSimpleStatusBar();
#ifdef _STATUS_BAR_PARTS    
    int status_parts[] = { 200, 400, 600, -1 };
    ::SendMessage(m_hWndStatusBar, SB_SETPARTS, _countof(status_parts), (LPARAM)&status_parts);
#endif
    m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);

    UIAddToolBar(hWndToolBar);
    UISetCheck(ID_VIEW_TOOLBAR, 1);
    UISetCheck(ID_VIEW_STATUS_BAR, 1);

    // register object for message filtering and idle updates
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);

    m_view.ShowStackView(!gSettings.GetInfoHiden());
    m_view.ShowTreeView(!gSettings.GetTreeViewHiden());
    m_view.ShowStackView(!gSettings.GetInfoHiden());

    SetTitle();

    return 0;
}

LRESULT CMainFrame::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;
    NMHDR* lpNMHDR = (NMHDR*)lParam;
    if (lpNMHDR->hwndFrom == m_cb.m_hWnd)
    {
        if (lpNMHDR->code == CBEN_BEGINEDIT)
        {
            //stdlog("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
            while (m_cb.GetCount())
                m_cb.DeleteItem(0);
            LoadSearchList();
            bHandled = TRUE;
        }
    }
    return 0;
}

void CMainFrame::LoadSearchList()
{
    CHAR* searchList = gSettings.GetSearchList();
    char* next_token = NULL;
    char* p = strtok_s(searchList, "\n", &next_token);
    COMBOBOXEXITEM item = { 0 };
    item.mask = CBEIF_TEXT;
    const int MAX_SEARCH_ITEM = 40;
    while (p && item.iItem < MAX_SEARCH_ITEM) {
        item.pszText = p;
        m_cb.InsertItem(&item);
        item.iItem++;
        p = strtok_s(NULL, "\n", &next_token);
    }
}

void CMainFrame::SaveSearchList()
{
    CString strSearch;
    for (int i = 0; i < m_cb.GetCount(); i++)
    {
        CString str;
        m_cb.GetLBText(i, str);
        if (!str.IsEmpty())
        {
            strSearch += str;
            strSearch += "\n";
        }
    }
    gSettings.SetSearchList(strSearch.GetBuffer());
}

LRESULT CMainFrame::OnActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    //DlgProgress dlg(ID_FILE_IMPORT, "D:\\Temp\\t1.txt"); dlg.DoModal();
    static bool activated = false;
    if (!activated && wParam != WA_INACTIVE)
    {
		activated = true;
		gSettings.RestoreWindPos(m_hWnd);
		StartLogging(true);
		//DlgModules dlg;
        //dlg.DoModal();
    }

    return 0;
}

LRESULT CMainFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    StopLogging(true, true);
    gSettings.SetVertSplitterPos(m_view.GetVertSplitterPosPct());
    gSettings.SetHorzSplitterPos(m_view.GetHorzSplitterPosPct());
    gSettings.SetStackSplitterPos(m_backTrace.GetStackSplitterPosPct());
    gSettings.SaveWindPos(m_hWnd);
    bHandled = FALSE;

    return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    // unregister message filtering and idle updates
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->RemoveMessageFilter(this);
    pLoop->RemoveIdleHandler(this);

    bHandled = FALSE;
    return 1;
}


void CMainFrame::RefreshLog(bool showAll)
{
	//gArchive.updateNodes();
    gArchive.getListedNodes()->updateList(gSettings.GetFlowTracesHiden());
    m_tree.RefreshTree(showAll);
    m_list.RefreshList(false);
}

LRESULT CMainFrame::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (wParam == TIMER_DATA_REFRESH)
    {
#ifdef _AUTO_TEST
		static boolean autoDone = false;
		if (!autoDone && wParam != WA_INACTIVE)
		{
			autoDone = true;
			DlgProgress dlg(ID_FILE_IMPORT, "auto"); dlg.DoModal();
			return 0;
		}
#endif
		if (gLogReceiver.working())
        {
            RefreshLog(false);
        }
    }
    return 0;
}

void CMainFrame::StartLogging(bool reset)
{
    SetTimer(TIMER_DATA_REFRESH, TIMER_DATA_REFRESH_INTERVAL);
    gLogReceiver.start(reset);
}

void CMainFrame::StopLogging(bool bClearArcive, bool closing)
{
    KillTimer(TIMER_DATA_REFRESH);
    gLogReceiver.stop();
    if (bClearArcive)
    {
        gArchive.clearArchive(closing);
    }
    else
    {
		gArchive.onPaused();
        RefreshLog(true);
    }
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    PostMessage(WM_CLOSE);
    return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    static BOOL bVisible = TRUE;	// initially visible
    bVisible = !bVisible;
    CReBarCtrl rebar = m_hWndToolBar;
    int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
    rebar.ShowBand(nBandIndex, bVisible);
    UISetCheck(ID_VIEW_TOOLBAR, bVisible);
    UpdateLayout();
    return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
    ::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
    UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
    UpdateLayout();
    return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CAboutDlg dlg;
    dlg.DoModal();
    return 0;
}

LRESULT CMainFrame::OnViewModules(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    DlgModules dlg;
    dlg.DoModal();
    return 0;
}

LRESULT CMainFrame::OnViewFilters(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	DlgFilters dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnViewDetailes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    DlgDetailes dlg;
    if (IDOK == dlg.DoModal())
        m_view.ApplySettings(false);
    return 0;
}

void CMainFrame::SetTitle()
{
    CHAR pBuf[256];
    _sntprintf_s(pBuf, _countof(pBuf), _countof(pBuf) - 1, TEXT("%d - %s"), gSettings.GetUdpPort(), TEXT("FlowTrace"));
    SetWindowText(pBuf);
}

LRESULT CMainFrame::OnViewSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    DWORD udpPort = gSettings.GetUdpPort();
    DlgSettings dlg;
    if (IDOK == dlg.DoModal())
    {
        if (udpPort != gSettings.GetUdpPort())
        {
            StopLogging(false);
            StartLogging(true);
            SetTitle();
        }
        m_view.ApplySettings(true);
    }
    return 0;
}

LRESULT CMainFrame::OnFileSave(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    DlgProgress dlg(wID, NULL);
    dlg.DoModal();
    return 0;
}

LRESULT CMainFrame::OnFileImport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    DlgProgress dlg(wID, NULL);
    dlg.DoModal();
    return 0;
}

LRESULT CMainFrame::OnShowHideTreeView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    gSettings.SetTreeViewHiden(!gSettings.GetTreeViewHiden());
    m_view.ShowTreeView(!gSettings.GetTreeViewHiden());
    return 0;
}

LRESULT CMainFrame::OnShowHideStack(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  gSettings.SetInfoHiden(!gSettings.GetInfoHiden());
  m_view.ShowStackView(!gSettings.GetInfoHiden());
  return 0;
}

LRESULT CMainFrame::OnRunExternalCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	const char* pExternalCmd = nullptr;
	if (ID_VIEW_RUN_EXTERNAL_CMD_1 == wID)
		pExternalCmd = gSettings.GetExternalCmd_1();
	else if (ID_VIEW_RUN_EXTERNAL_CMD_2 == wID)
		pExternalCmd = gSettings.GetExternalCmd_2();
	else if (ID_VIEW_RUN_EXTERNAL_CMD_3 == wID)
		pExternalCmd = gSettings.GetExternalCmd_3();
	if (pExternalCmd && *pExternalCmd)
	{
		char cmd[MAX_PATH];
		strncpy_s(cmd, pExternalCmd, sizeof(cmd));
		cmd[sizeof(cmd) - 1] = 0;
		
		char *next_token;
		char* cmdToken = strtok_s(cmd, ";", &next_token);
		while (cmdToken != nullptr)
		{
			if (cmdToken[1] == 0 && (cmdToken[0] == 'x' || cmdToken[0] == 'X'))
			{
				ClearLog(true, true); 
			}
			else
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;
				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				if (CreateProcessA(cmdToken, "", NULL, NULL, FALSE,
					NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
				{
					WaitForSingleObject(pi.hProcess, INFINITE);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
			}
			cmdToken = strtok_s(nullptr, ";", &next_token);
		}

	}
	return 0;
}

LRESULT CMainFrame::OnBookmarks(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_list.OnBookmarks(wID);
    return 0;
}

LRESULT CMainFrame::onShowMsg(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  CHAR* buf = (CHAR*)wParam;
  MessageBox(buf, TEXT("Flow Trace Error"), MB_OK | MB_ICONEXCLAMATION);
  delete buf;
  return true;
}

LRESULT CMainFrame::onUpdateBackTrace(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
  LOG_NODE* pNode = (LOG_NODE*)lParam;
  m_view.ShowBackTrace(pNode);
  return 0;
}

LRESULT CMainFrame::onUpdateTree(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    m_tree.RedrawAll();
    return 0;
}

LRESULT CMainFrame::onUpdateFilter(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    //bool filterChanged = false;
    //LOG_NODE* pNode = (LOG_NODE*)lParam;
    //if (pNode && (pNode->isApp() || pNode->isThread()))
    //{
    //    int cheked = pNode->checked;
    //    if (pNode->hiden != !cheked)
    //    {
    //        pNode->hiden = !cheked;
    //        filterChanged = true;
    //    }
    //}
    //else
    //{
    //    filterChanged = true;
    //}
    //if (filterChanged)
    {
        //m_searchedit.SetWindowText(_T(""));
        m_searchResult.SetWindowText(_T(""));
        searchInfo.ClearSearchResults();
        gArchive.getListedNodes()->applyFilter(gSettings.GetFlowTracesHiden());
        m_list.RefreshList(true);
        m_tree.RedrawAll();
    }
    return 0;
}

LRESULT CMainFrame::OnShowHideFlowTraces(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    gSettings.SetFlowTracesHiden(!gSettings.GetFlowTracesHiden());
    ::SendMessage(hwndMain, WM_UPDATE_FILTER, 0, 0);
    return 0;
}

LRESULT CMainFrame::OnStartRecording(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
#ifdef _AUTO_TEST
	DlgProgress dlg(ID_FILE_IMPORT, "auto"); dlg.DoModal();
#endif
	StartLogging(true);
    return 0;
}

LRESULT CMainFrame::OnPauseRecording(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
#ifdef _AUTO_TEST
	DlgProgress dlg(ID_FILE_IMPORT, "auto"); dlg.DoModal();
#endif
	StopLogging(false);
    return 0;
}

LRESULT CMainFrame::onCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (::GetFocus() == m_tree.m_hWnd)
        m_tree.CopySelection();
    else if (::GetFocus() == m_list.m_hWnd)
        m_list.CopySelection();
    else if (::GetFocus() == m_backTrace.m_wndCallFuncView.m_hWnd)
      m_backTrace.m_wndCallFuncView.CopySelection();
    else if (::GetFocus() == m_backTrace.m_wndCallStackView.m_hWnd)
      m_backTrace.m_wndCallStackView.CopySelection();
    else if (::GetFocus() == m_searchedit.m_hWnd)
      m_searchedit.Copy();
    
    return 0;
}

LRESULT CMainFrame::OnImportTask(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (wParam == 0)
	{
		if (!lParam)
			ClearLog(false);
	}
    else
    {
        RefreshLog(true);
    }

    return 0;
}

LRESULT CMainFrame::onSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_list.SelectAll();
    return 0;
}

void CMainFrame::SyncViews()
{
    m_view.SyncViews();
}

void CMainFrame::RedrawViews()
{
	m_list.Invalidate(FALSE);
	m_tree.Invalidate(FALSE);
}

LRESULT CMainFrame::OnSyncViews(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SyncViews();
    return 0;
}

LRESULT CMainFrame::OnEditFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_searchedit.SetFocus();
    return 0;
}

LRESULT CMainFrame::OnShowOnlyThisApp(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	FilterNode(wID);
	return 0;
}
LRESULT CMainFrame::OnShowOnlyThisThread(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	FilterNode(wID);
	return 0;
}
LRESULT CMainFrame::OnShowAllApps(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	FilterNode(wID);
	return 0;
}
void CMainFrame::FilterNode(WORD wID)
{
	LOG_NODE* pNode = m_view.SyncViews();
	if (!pNode)
		return;
	
	if (wID == ID_TREE_SHOW_THIS_THREAD)
	{
		pNode = pNode->getTrhread();
	}
	else if (wID == ID_TREE_SHOW_THIS_APP)
	{
		pNode = pNode->getApp();
	}
	else if (wID == ID_TREE_SHOW_ALL)
	{
		pNode = pNode->getRoot();
	}

	if (!pNode)
		return;

	bool checkChanged = pNode->ShowOnlyThis();
	if (checkChanged)
	{
		::PostMessage(hwndMain, WM_UPDATE_FILTER, 0, 0);
		m_tree.RedrawAll();
	}
}




LRESULT CMainFrame::OnClearLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ClearLog(true, true);
    return 0;
}

LRESULT CMainFrame::OnResetLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ClearLog(true, true);
	return 0;
}

void CMainFrame::ClearLog(bool bRestart, bool reset)
{
    StopLogging(true, false);
    m_view.ClearLog();
    searchInfo.ClearSearchResults();
    if (bRestart)
        StartLogging(reset);
}

LRESULT CALLBACK subEditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            ::PostMessage(hwndMain, WM_COMMAND, ID_SEARCH_REFRESH_ON_EMTER, 0);
            return CallWindowProc(oldEditProc, wnd, msg, wParam, lParam);
            //return 0; //if you don't want to pass it further to def thread
            //break;  //If not your key, skip to default:
        }
    default:
        return CallWindowProc(oldEditProc, wnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CMainFrame::OnSearchSettings(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
    switch (wID)
    {
    case ID_SEARCH_MATCH_CASE:
        searchInfo.bMatchCase = !searchInfo.bMatchCase;
        m_searchbar.CheckButton(ID_SEARCH_MATCH_CASE, searchInfo.bMatchCase);
        break;

    case ID_SEARCH_MATCH_WHOLE_WORD:
        searchInfo.bMatchWholeWord = !searchInfo.bMatchWholeWord;
        m_searchbar.CheckButton(ID_SEARCH_MATCH_WHOLE_WORD, searchInfo.bMatchWholeWord);
        break;
    }
    PostMessage(WM_COMMAND, ID_SEARCH_REFRESH, 0);
    //OnSearch(0, wID, 0, bHandled);
    return 0;
}

LRESULT CMainFrame::OnSearchClear(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_searchedit.SetWindowText(_T(""));
    m_searchedit.SetFocus();
    return 0;
}

LRESULT CMainFrame::OnSearchNavigate(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SearchNavigate(wID);
    return 0;
}

LRESULT CMainFrame::OnSearchRefreshOnEnter(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SearchRefresh(ID_SEARCH_LAST); //ID_SEARCH_LAST
    return 0;
}

LRESULT CMainFrame::OnSearchRefresh(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SearchRefresh(ID_SEARCH_REFRESH);
    return 0;
}

void CMainFrame::SearchRefresh(WORD wID)
{
    //Update Serch
    CHAR szText[256];
    ::GetWindowText(searchInfo.hwndEdit, szText, _countof(szText) - 1);
    szText[_countof(szText) - 1] = 0;
    searchInfo.setSerachText(szText);

    searchInfo.ClearSearchResults();

    if (szText[0])
    {
        int i = m_cb.FindStringExact(0, szText);
        if (i >= 0)
            m_cb.DeleteString(i);
        COMBOBOXEXITEM item = { 0 };
        item.mask = CBEIF_TEXT;
        item.pszText = szText;
        item.iItem = 0;
        m_cb.InsertItem(&item);
        m_cb.SetCurSel(0);
        SaveSearchList();

        ::SetWindowText(searchInfo.hwndEdit, szText);


        //do search
        DWORD lineCount = gArchive.getListedNodes()->Count();
        int countInLast = 0;
        for (DWORD i = 0; i < lineCount; i++)
        {
            LOG_NODE* pNode = gArchive.getListedNodes()->getNode(i);
            CHAR* p = m_list.getText(i);
            if (p[0])
            {
                pNode->lineSearchPos = 0;
                while (p = searchInfo.find(p))
                {
                    if (searchInfo.firstLine == 0 && searchInfo.total == 0)
                    {
                        searchInfo.firstLine = i;
                    }
                    if (pNode->lineSearchPos == 0)
                    {
                        countInLast = 0;
                    }
                    searchInfo.lastLine = i;
                    searchInfo.total++;
                    if (pNode->lineSearchPos == 0)
                        pNode->lineSearchPos = searchInfo.total;
                    countInLast++;
                    p += searchInfo.cbText;
                }
            }
        }
        searchInfo.curLine = searchInfo.lastLine;
        searchInfo.posInCur = countInLast - 1;
    }

    SearchNavigate(wID);
    m_list.SetFocus();
}

void CMainFrame::SearchNavigate(WORD wID)
{
    //if (wID != ID_SEARCH_REFRESH)
    //    m_list.SelLogSelection();
    int item = m_list.getSelectionItem();
    int column = m_list.getSelectionColumn();
    int logLen;
    bool found = false;
    if (searchInfo.total)
    {
        if (wID == ID_SEARCH_PREV)
        {
            if (item == searchInfo.curLine && searchInfo.posInCur > 0)
            {
                searchInfo.posInCur--;
                found = true;
            }
            else //if (searchInfo.curLine > 0)
            {
                for (int i = item; i >= 0; i--)
                {
                    LOG_NODE* pNode = gArchive.getListedNodes()->getNode(i);
                    ATLASSERT(pNode);
                    if (pNode && pNode->isInfo())
                    {
                        CHAR* p = m_list.getText(i);
                        if (p[0])
                        {
                            if (pNode->lineSearchPos)
                            {
                                searchInfo.curLine = i;
                                int curCount = searchInfo.calcCountIn(p);
                                searchInfo.posInCur = curCount - 1;
                                if (i == item)
                                {
                                    searchInfo.posInCur = -1;
                                    char* log = m_list.getText(searchInfo.curLine, &logLen);
                                    if (column > 0 && column <= logLen)
                                    {
                                        char* p = log;
                                        while (NULL != (p = searchInfo.find(p)) && (p - log) < column - (int)searchInfo.cbText)
                                        {
                                            p += searchInfo.cbText;
                                            searchInfo.posInCur++;
                                        }
                                        if (searchInfo.posInCur < 0)
                                        {
                                            continue;
                                        }
                                    }
                                }
                                found = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        else if (wID == ID_SEARCH_NEXT)
        {
            CHAR* p = m_list.getText(searchInfo.curLine);
            int curCount = searchInfo.calcCountIn(p);
            if (item == searchInfo.curLine && searchInfo.posInCur < curCount - 1)
            {
                searchInfo.posInCur++;
                found = true;
            }
            else //if (searchInfo.curLine != searchInfo.lastLine)
            {
                for (int i = item; i <= searchInfo.lastLine; i++)
                {
                    LOG_NODE* pNode = gArchive.getListedNodes()->getNode(i);
                    ATLASSERT(pNode);
                    if (pNode && pNode->isInfo())
                    {
                        if (pNode->lineSearchPos)
                        {
                            searchInfo.curLine = i;
                            searchInfo.posInCur = 0;
                            if (i == item)
                            {
                                char* log = m_list.getText(searchInfo.curLine, &logLen);
                                if (column > 0 && column <= logLen)
                                {
                                    char* p = log;
                                    while (NULL != (p = searchInfo.find(p)) && (p - log) < column)
                                    {
                                        p += searchInfo.cbText;
                                        searchInfo.posInCur++;
                                    }
                                    if (searchInfo.posInCur >= curCount)
                                    {
                                        continue;
                                    }
                                }
                            }
                            found = true;
                            break;
                        }
                    }
                }
            }
        }

        if (searchInfo.posInCur < 0)
        {
            searchInfo.posInCur = 0;
        }
        if (wID == ID_SEARCH_FIRST || (!found && wID == ID_SEARCH_PREV) || (searchInfo.curLine < searchInfo.firstLine))
        {
            searchInfo.curLine = searchInfo.firstLine;
            searchInfo.posInCur = 0;
        }
        else if (wID == ID_SEARCH_LAST || (!found && wID == ID_SEARCH_NEXT) || (searchInfo.curLine > searchInfo.lastLine))
        {
            searchInfo.curLine = searchInfo.lastLine;
            CHAR* p = m_list.getText(searchInfo.lastLine);
            int curCount = searchInfo.calcCountIn(p);
            searchInfo.posInCur = curCount - 1;
        }

        if (wID != ID_SEARCH_REFRESH)
        {
            char* log = m_list.getText(searchInfo.curLine, &logLen);
            char* p = log;
            int searchPos = 0;
            while (NULL != (p = searchInfo.find(p)) && searchInfo.posInCur > searchPos)
            {
                p += searchInfo.cbText;
                searchPos++;
            }

            if (p)
            {
              m_list.MoveSelectionEx(searchInfo.curLine, int((p - log) + searchInfo.cbText), false);
              m_list.EnsureTextVisible(searchInfo.curLine, int(p - log), int(p - log + searchInfo.cbText));
            }
        }
    }

    LOG_NODE* pNode = gArchive.getListedNodes()->getNode(searchInfo.curLine);
    if (pNode)
    {
        CHAR szText[32];
        _stprintf_s(szText, _countof(szText), _T("%d of %d"), searchInfo.total ? pNode->lineSearchPos + searchInfo.posInCur : 0, searchInfo.total);
        m_searchResult.SetWindowText(szText);
    }

    m_list.Redraw(-1, -1);
}

void CMainFrame::UpdateStatusBar()
{
    static CHAR pBuf[1024];
    static DWORD prev_lost = 0;

#ifdef _STATUS_BAR_PARTS    
    _sntprintf_s(pBuf, _countof(pBuf), _countof(pBuf) - 1, TEXT("Log: %s"), Helpers::str_format_int_grouped(m_tree.GetRecCount()));
    ::SendMessage(m_hWndStatusBar, SB_SETTEXT, 0, (LPARAM)pBuf);
    _sntprintf_s(pBuf, _countof(pBuf), _countof(pBuf) - 1, TEXT("Mem: %s"), Helpers::str_format_int_grouped((LONG_PTR)(gArchive.UsedMemory())));
    ::SendMessage(m_hWndStatusBar, SB_SETTEXT, 1, (LPARAM)pBuf);
    _sntprintf_s(pBuf, _countof(pBuf), _countof(pBuf) - 1, TEXT("Listed: %s"), Helpers::str_format_int_grouped(m_list.GetRecCount()));
    ::SendMessage(m_hWndStatusBar, SB_SETTEXT, 2, (LPARAM)pBuf);
    _sntprintf_s(pBuf, _countof(pBuf), _countof(pBuf) - 1, TEXT("Ln: %s"), Helpers::str_format_int_grouped(m_list.getSelectionItem() + 1));
    ::SendMessage(m_hWndStatusBar, SB_SETTEXT, 3, (LPARAM)pBuf);
#else
    int cb = 0;
    DWORD lost = gArchive.getLost();
	if (lost)
		cb += _sntprintf_s(pBuf + cb, _countof(pBuf) - cb, _countof(pBuf) - cb - 1, TEXT("LOST: %d   "), lost);
	cb += _sntprintf_s(pBuf + cb, _countof(pBuf) - cb, _countof(pBuf) - cb - 1, TEXT("Count: %s"), Helpers::str_format_int_grouped(gArchive.getNodeCount()));
    cb += _sntprintf_s(pBuf + cb, _countof(pBuf) - cb, _countof(pBuf) - cb - 1, TEXT("   Memory: %s mb"), Helpers::str_format_int_grouped((LONG_PTR)(gArchive.UsedMemory()/1000000)));
    cb += _sntprintf_s(pBuf + cb, _countof(pBuf) - cb, _countof(pBuf) - cb - 1, TEXT("   Listed: %s"), Helpers::str_format_int_grouped(m_list.GetRecCount()));
    cb += _sntprintf_s(pBuf + cb, _countof(pBuf) - cb, _countof(pBuf) - cb - 1, TEXT("   Line: %s"), Helpers::str_format_int_grouped(m_list.getSelectionItem() + 1));
    ::PostMessage(m_hWndStatusBar, SB_SETTEXT, 0, (LPARAM)pBuf);

    if (lost != prev_lost)
    {
        if (lost && !prev_lost)
            ::PostMessage(m_hWndStatusBar, SB_SETICON, 0, (LPARAM)m_lostIcon);
        else if (!lost)
            ::PostMessage(m_hWndStatusBar, SB_SETICON, 0, (LPARAM)0);
        prev_lost = lost;
    }
#endif
}