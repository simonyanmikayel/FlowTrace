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
#include "ServerThread.h"
#include "SettingsDlg.h"
#include "DetailesDlg.h"
#include "ProgressDlg.h"
#include "SearchInfo.h"
#include "SnapshotsDlg.h"

HWND       hwndMain;
HWND hWndStatusBar;
WNDPROC oldEditProc;
LRESULT CALLBACK subEditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

CMainFrame::CMainFrame()
    : m_pServerThread(NULL)
    , m_list(m_view.list())
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
    UIEnable(ID_VIEW_PAUSERECORDING, m_pServerThread != NULL);
    UIEnable(ID_VIEW_STARTRECORDIG, m_pServerThread == NULL);
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

        // set 15 lines visible in combo list
        m_cb.GetComboCtrl().ResizeClient(rcCombo.Width(), rcCombo.Height() + 15 * m_cb.GetItemHeight(0));
        m_searchbox = m_cb.GetComboCtrl();
        m_searchedit = m_cb.GetEditCtrl();
        oldEditProc = (WNDPROC)m_searchedit.SetWindowLongPtr(GWLP_WNDPROC, (LONG_PTR)subEditProc);
        searchInfo.hwndEdit = m_searchedit;

        TCHAR* searchList = gSettings.GetSearchList();
        char *p = strtok(searchList, "\n");
        COMBOBOXEXITEM item = { 0 };
        item.mask = CBEIF_TEXT;
        while (p) {
            item.pszText = p;
            m_cb.InsertItem(&item);
            item.iItem++;

            p = strtok(NULL, "\n");
        }

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

    SizeSimpleReBarBands();

    CreateSimpleStatusBar();
    hWndStatusBar = m_hWndStatusBar;
    int status_parts[] = { 200, 400, -1 };
    ::SendMessage(m_hWndStatusBar, SB_SETPARTS, sizeof(status_parts) / sizeof(status_parts[0]), (LPARAM)&status_parts);

    m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);

    UIAddToolBar(hWndToolBar);
    UISetCheck(ID_VIEW_TOOLBAR, 1);
    UISetCheck(ID_VIEW_STATUS_BAR, 1);

    // register object for message filtering and idle updates
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);

    gSettings.RestoreWindPos(m_hWnd);

    StartLogging();

    //CProgressDlg dlg(ID_FILE_IMPORT, "D:\\Temp\\t1.txt"); dlg.DoModal();
#ifdef _AUTO_TEST
    CProgressDlg dlg(ID_FILE_IMPORT, "auto"); dlg.DoModal();
#endif
    m_view.ShowStackView(!gSettings.GetInfoHiden());
    m_view.ShowTreeView(!gSettings.GetTreeViewHiden());
    m_view.ShowStackView(!gSettings.GetInfoHiden());

    return 0;
}

LRESULT CMainFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    StopLogging(true);
    gSettings.SetVertSplitterPos(m_view.GetVertSplitterPosPct());
    gSettings.SetHorzSplitterPos(m_view.GetHorzSplitterPosPct());
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
    listNodes->updateList(gSettings.GetFlowTracesHiden());
    m_tree.RefreshTree(showAll);
    m_list.RefreshList(false);
}

LRESULT CMainFrame::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (wParam == TIMER_DATA_REFRESH)
    {
        if (m_pServerThread)
        {
            if (m_pServerThread->IsWorking())
            {
                RefreshLog(false);
            }
            else
            {
                StopLogging(false);
                Helpers::ErrMessageBox(TEXT("Recording stoped"));
            }
        }
    }
    return 0;
}

void CMainFrame::StartLogging()
{
    SetTimer(TIMER_DATA_REFRESH, TIMER_DATA_REFRESH_INTERVAL);
    m_pServerThread = new ServerThread();
    m_pServerThread->StartWork();
}

void CMainFrame::StopLogging(bool bClearArcive)
{
    KillTimer(TIMER_DATA_REFRESH);
    if (m_pServerThread)
        m_pServerThread->StopWork();
    delete m_pServerThread;
    m_pServerThread = NULL;
    if (bClearArcive)
    {
        gArchive.clearArchive();
    }
    else
    {
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

LRESULT CMainFrame::OnViewDetailes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CColumnsDlg dlg;
    if (IDOK == dlg.DoModal())
        m_view.ApplySettings(false);
    return 0;
}

LRESULT CMainFrame::OnViewSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    DWORD udpPort = gSettings.GetUdpPort();
    DWORD resolveAddr = gSettings.GetResolveAddr();
    CSettingsDlg dlg;
    if (IDOK == dlg.DoModal())
    {
        if (udpPort != gSettings.GetUdpPort())
        {
            StopLogging(false);
            StartLogging();
        }
        if (gSettings.GetResolveAddr() && !resolveAddr)
          gArchive.resolveAddrAsync();
        m_view.ApplySettings(true);
    }
    return 0;
}

LRESULT CMainFrame::OnFileSave(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CProgressDlg dlg(wID, NULL);
    dlg.DoModal();
    return 0;
}

LRESULT CMainFrame::OnFileImport(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CProgressDlg dlg(wID, NULL);
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

LRESULT CMainFrame::OnBookmarks(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_list.OnBookmarks(wID);
    return 0;
}

LRESULT CMainFrame::onShowMsg(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  TCHAR* buf = (TCHAR*)wParam;
  MessageBox(buf, TEXT("Flow Trace Error"), MB_OK | MB_ICONEXCLAMATION);
  delete buf;
  return true;
}

LRESULT CMainFrame::onUpdateBackTrace(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
  LOG_NODE* pNode = (LOG_NODE*)lParam;
  m_view.ShowBackTrace(NULL, pNode, gArchive.getArchiveNumber());
  return 0;
}
LRESULT CMainFrame::onUpdateFilter(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    bool filterChanged = false;
    LOG_NODE* pNode = (LOG_NODE*)lParam;
    if (pNode && (pNode->isApp() || pNode->isProc()))
    {
        int cheked = pNode->checked;
        if (pNode->hiden != !cheked)
        {
            pNode->hiden = !cheked;
            filterChanged = true;
        }
    }
    else
    {
        filterChanged = true;
    }

    if (filterChanged)
    {
        //m_searchedit.SetWindowText(_T(""));
        m_searchResult.SetWindowText(_T(""));
        searchInfo.ClearSearchResults();
        listNodes->applyFilter(gSettings.GetFlowTracesHiden());
        m_list.RefreshList(true);
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
    StartLogging();
    return 0;
}

LRESULT CMainFrame::OnPauseRecording(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
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

LRESULT CMainFrame::OnImportTask(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (wParam == 0)
        ClearLog(false);
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

LRESULT CMainFrame::OnSyncViews(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_view.SyncViews();
    return 0;
}

LRESULT CMainFrame::OnEditFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_searchedit.SetFocus();
    return 0;
}

LRESULT CMainFrame::OnTakeSnamshot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SnapshotsDlg dlg;
    if (IDOK == dlg.DoModal())
        ::SendMessage(hwndMain, WM_UPDATE_FILTER, 0, 0);
    return 0;
}

LRESULT CMainFrame::OnStartNewSnamshot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SnapshotsDlg dlg;
    if (dlg.AddSnapshot("", true))
        ::SendMessage(hwndMain, WM_UPDATE_FILTER, 0, 0);
    return 0;
}

LRESULT CMainFrame::OnClearLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ClearLog(true);
    return 0;
}

void CMainFrame::ClearLog(bool bRestart)
{
    StopLogging(true);
    m_view.ClearLog();
    searchInfo.ClearSearchResults();
    if (bRestart)
        StartLogging();
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
            //return 0; //if you don't want to pass it further to def proc
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
    TCHAR szText[256];
    ::GetWindowText(searchInfo.hwndEdit, szText, sizeof(szText) / sizeof(szText[0]) - 1);
    szText[sizeof(szText) / sizeof(szText[0]) - 1] = 0;
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
        if (m_cb.GetCount() > 15) {
            m_cb.DeleteItem(15);
            m_cb.SetCurSel(0);
        }

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
        ::SetWindowText(searchInfo.hwndEdit, szText);


        //do search
        DWORD lineCount = listNodes->Count();
        int countInLast = 0;
        for (DWORD i = 0; i < lineCount; i++)
        {
            LOG_NODE* pNode = listNodes->getNode(i);
            TCHAR* p = m_list.getText(i);
            if (p[0])
            {
                pNode->hasSearchResult = 0;
                while (p = searchInfo.find(p))
                {
                    if (searchInfo.firstLine == 0 && searchInfo.total == 0)
                    {
                        searchInfo.firstLine = i;
                    }
                    if (pNode->hasSearchResult == 0)
                    {
                        countInLast = 0;
                    }
                    searchInfo.lastLine = i;
                    searchInfo.total++;
                    pNode->hasSearchResult = 1;
                    countInLast++;
                    p += searchInfo.cbText;
                }
            }
        }
        searchInfo.curLine = searchInfo.lastLine;
        searchInfo.posInCur = countInLast - 1;
        searchInfo.cur = searchInfo.total - 1;
    }

    SearchNavigate(wID);
    m_list.SetFocus();
}

void CMainFrame::SearchNavigate(WORD wID)
{
    if (wID != ID_SEARCH_REFRESH)
        m_list.SelLogSelection();
    int item = m_list.IsLogSelection() ? m_list.getSelectionItem() : searchInfo.curLine;
    bool found = false;
    if (searchInfo.total)
    {
        if (wID == ID_SEARCH_PREV)
        {
            if (item == searchInfo.curLine && searchInfo.posInCur > 0)
            {
                searchInfo.posInCur--;
                searchInfo.cur--;
                found = true;
            }
            else //if (searchInfo.curLine > 0)
            {
                for (int i = item - 1; i >= 0; i--)
                    //for (int i = searchInfo.curLine - 1; i >= 0; i--)
                {
                    LOG_NODE* pNode = listNodes->getNode(i);
                    TCHAR* p = m_list.getText(i);
                    if (p[0])
                    {
                        if (pNode->hasSearchResult)
                        {
                            searchInfo.curLine = i;
                            searchInfo.cur--;
                            int curCount = searchInfo.calcCountIn(p);
                            searchInfo.posInCur = curCount - 1;
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
        else if (wID == ID_SEARCH_NEXT)
        {
            TCHAR* p = m_list.getText(searchInfo.curLine);
            int curCount = searchInfo.calcCountIn(p);
            if (item == searchInfo.curLine && searchInfo.posInCur < curCount - 1)
            {
                searchInfo.posInCur++;
                searchInfo.cur++;
                found = true;
            }
            else //if (searchInfo.curLine != searchInfo.lastLine)
            {
                for (int i = item + 1; i <= searchInfo.lastLine; i++)
                    //for (int i = searchInfo.curLine + 1; i <= searchInfo.lastLine; i++)
                {
                    LOG_NODE* pNode = listNodes->getNode(i);
                    if (pNode->isInfo())
                    {
                        INFO_DATA* pLogData = (INFO_DATA*)pNode->data;
                        if (pNode->hasSearchResult)
                        {
                            searchInfo.curLine = i;
                            searchInfo.cur++;
                            searchInfo.posInCur = 0;
                            found = true;
                            break;
                        }
                    }
                }
            }
        }

        if (wID == ID_SEARCH_FIRST || (!found && wID == ID_SEARCH_PREV))
        {
            searchInfo.curLine = searchInfo.firstLine;
            searchInfo.cur = 0;
            searchInfo.posInCur = 0;
        }
        else if (wID == ID_SEARCH_LAST || (!found && wID == ID_SEARCH_NEXT))
        {
            searchInfo.curLine = searchInfo.lastLine;
            searchInfo.cur = searchInfo.total - 1;
            TCHAR* p = m_list.getText(searchInfo.lastLine);
            int curCount = searchInfo.calcCountIn(p);
            searchInfo.posInCur = curCount - 1;
        }

        if (wID != ID_SEARCH_REFRESH)
        {
            int logLen;
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
                //if (m_list.IsLogSelection())
                {
                    m_list.MoveSelectionEx(searchInfo.curLine, (p - log) + searchInfo.cbText, false);
                }
                m_list.EnsureTextVisible(searchInfo.curLine, p - log, p - log + searchInfo.cbText);
            }
        }
    }

    TCHAR szText[32];
    _stprintf_s(szText, _T("%d of %d"), searchInfo.total ? searchInfo.cur + 1 : 0, searchInfo.total);
    m_searchResult.SetWindowText(szText);

    m_list.Redraw(-1, -1);
}
