// FlowTraceView.cpp : implementation of the CFlowTraceView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "Settings.h"
#include "Helpers.h"
#include "FlowTraceView.h"

CFlowTraceView::CFlowTraceView()
  : m_wndListView(this)
  , m_wndTreeView(this)
  , m_wndBackTraceView(this)
  , m_selectedNode(NULL)
{

}

BOOL CFlowTraceView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

LRESULT CFlowTraceView::OnCreate(LPCREATESTRUCT lpcs)
{
  DWORD dwStyle;
  ModifyStyle(0, WS_CLIPCHILDREN);
  ModifyStyleEx(WS_EX_CLIENTEDGE, WS_EX_TRANSPARENT);

  //m_wndLog.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW);

  m_wndHorzSplitter.m_bVertical = false;
  m_wndHorzSplitter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, SPLIT_PROPORTIONAL);
  m_wndVertSplitter.Create(m_wndHorzSplitter, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, SPLIT_PROPORTIONAL);

  dwStyle = WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
    LVS_REPORT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_OWNERDATA | LVS_NOCOLUMNHEADER;
#if(0)
  dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN;
#endif
  if (!gSettings.GetInfoHiden())
    dwStyle |= WS_VISIBLE;
  m_wndBackTraceView.Create(m_wndHorzSplitter, rcDefault, NULL, dwStyle, 0);
#ifdef _USE_RICH_EDIT_FOR_BACK_TRACE
  m_wndBackTraceView.SetEventMask(m_wndBackTraceView.GetEventMask() | ENM_SELCHANGE | ENM_LINK);
  m_wndBackTraceView.SetAutoURLDetect(TRUE);
  m_wndBackTraceView.SetUndoLimit(0);
  m_wndBackTraceView.SetReadOnly();
#endif

  m_wndListView.Create(m_wndVertSplitter, rcDefault, NULL,
    WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
    LVS_REPORT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_OWNERDATA | LVS_NOCOLUMNHEADER,
    0);

  m_wndTreeView.Create(m_wndVertSplitter, rcDefault, NULL,
    WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
    LVS_REPORT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_OWNERDATA | LVS_NOCOLUMNHEADER,
    LVS_EX_FULLROWSELECT);

  m_wndHorzSplitter.SetSplitterPanes(m_wndVertSplitter, m_wndBackTraceView);
  m_wndHorzSplitter.SetSplitterPosPct(max(10, min(90, gSettings.GetHorzSplitterPos())), false);

  m_wndVertSplitter.SetSplitterPanes(m_wndTreeView, m_wndListView);
  m_wndVertSplitter.SetSplitterPosPct(max(10, min(90, gSettings.GetVertSplitterPos())), false);

  ApplySettings(true);

  return 0; // windows sets focus to first control
}

LRESULT CFlowTraceView::OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
  return 1; // handled
}

void CFlowTraceView::ApplySettings(bool fontChanged)
{
  m_wndTreeView.ApplySettings(fontChanged);
  m_wndListView.ApplySettings(fontChanged);
  if (fontChanged)
    m_wndBackTraceView.SetFont(gSettings.GetFont());
}

LRESULT CFlowTraceView::OnLvnEndScroll(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
  m_wndListView.UpdateCaret();
  return 0;
}

void CFlowTraceView::ShowBackTrace(LOG_NODE* pSelectedNode, bool bNested, LOG_NODE* pUpdatedNode, DWORD archiveNumber)
{
  static DWORD curArchiveNumber = 0;
  static LOG_NODE* pCurNode = 0;

  if (archiveNumber != INFINITE && curArchiveNumber != archiveNumber)
  {
    return;
  }
  if (pUpdatedNode != NULL && pCurNode != pUpdatedNode)
  {
    return;
  }

  if (pSelectedNode == 0 && pUpdatedNode == 0)
  {
    return;
  }

  pCurNode = pSelectedNode ? pSelectedNode : pUpdatedNode;
  curArchiveNumber = gArchive.getArchiveNumber();

  if (pUpdatedNode == NULL)
  {
    if (pSelectedNode->PendingToResolveAddr())
    {
      gArchive.resolveAddr(pSelectedNode, bNested);
    }
  }

  m_wndBackTraceView.UpdateTrace(pCurNode, bNested);
}

void CFlowTraceView::SetChildPos(int cx, int cy)
{ 
  if (cx == 0 || cy == 0)
  {
    WINDOWPLACEMENT wndpl;
    GetWindowPlacement(&wndpl);
    cx = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
    cy = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
  }
  if (cx && cy)
  {
    m_wndHorzSplitter.MoveWindow(0, 0, cx, cy, TRUE);
    //RECT rc;
    //m_wndHorzSplitter.GetClientRect(&rc);
    //m_wndVertSplitter.MoveWindow(0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    //stdlog("cx: %d cy: %d rc1: %d %d\n", cx, cy, rc.right - rc.left, rc.bottom - rc.top);
  }
}

LRESULT CFlowTraceView::OnPositionChanging(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  WINDOWPOS*	pWinPos = reinterpret_cast<WINDOWPOS*>(lParam);
  if (!(pWinPos->flags & SWP_NOSIZE))
  {
    SetChildPos(pWinPos->cx, pWinPos->cy);
  }
  return 1;
}

void CFlowTraceView::ClearLog()
{
  m_wndListView.Clear();
  m_wndTreeView.Clear();
  m_wndBackTraceView.ClearTrace();
  m_selectedNode = NULL;
}

LRESULT CFlowTraceView::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
  bHandled = FALSE;
  return 0;
}

LRESULT CFlowTraceView::OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
  if (pnmh->hwndFrom == m_wndTreeView)
  {
    LPNMLVCUSTOMDRAW pNMLVCD = (LPNMLVCUSTOMDRAW)pnmh;
    switch (pNMLVCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
      return CDRF_NOTIFYSUBITEMDRAW;          // ask for subitem notifications.
    case CDDS_ITEMPREPAINT:
      return CDRF_NOTIFYSUBITEMDRAW;
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: // recd when CDRF_NOTIFYSUBITEMDRAW is returned in
    {                                    // response to CDDS_ITEMPREPAINT.
      m_wndTreeView.DrawSubItem(pNMLVCD->nmcd.dwItemSpec, pNMLVCD->iSubItem, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
      return CDRF_SKIPDEFAULT;
    }
    break;
    }
    return CDRF_DODEFAULT;
  }
  else if (pnmh->hwndFrom == m_wndListView)
  {
    LPNMLVCUSTOMDRAW pNMLVCD = (LPNMLVCUSTOMDRAW)pnmh;
    switch (pNMLVCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
      return CDRF_NOTIFYSUBITEMDRAW;          // ask for subitem notifications.
    case CDDS_ITEMPREPAINT:
      m_wndListView.ItemPrePaint(pNMLVCD->nmcd.dwItemSpec, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
      return CDRF_NOTIFYSUBITEMDRAW;
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: // recd when CDRF_NOTIFYSUBITEMDRAW is returned in
    {                                    // response to CDDS_ITEMPREPAINT.
      m_wndListView.DrawSubItem(pNMLVCD->nmcd.dwItemSpec, pNMLVCD->iSubItem, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
      return CDRF_SKIPDEFAULT;
    }
    break;
    }
    return CDRF_DODEFAULT;
  }
  else if (pnmh->hwndFrom == m_wndBackTraceView)
  {
    LPNMLVCUSTOMDRAW pNMLVCD = (LPNMLVCUSTOMDRAW)pnmh;
    switch (pNMLVCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
      return CDRF_NOTIFYSUBITEMDRAW;          // ask for subitem notifications.
    case CDDS_ITEMPREPAINT:
      m_wndBackTraceView.ItemPrePaint(pNMLVCD->nmcd.dwItemSpec, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
      return CDRF_NOTIFYSUBITEMDRAW;
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: // recd when CDRF_NOTIFYSUBITEMDRAW is returned in
    {                                    // response to CDDS_ITEMPREPAINT.
      m_wndBackTraceView.DrawSubItem(pNMLVCD->nmcd.dwItemSpec, pNMLVCD->iSubItem, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
      return CDRF_SKIPDEFAULT;
    }
    break;
  }
    return CDRF_DODEFAULT;
  }
  return CDRF_DODEFAULT;
}

void CFlowTraceView::ShowCallStack()
{
  if (gSettings.GetInfoHiden())
    ShowStackView(true);

  LOG_NODE* pNode = NULL;
  HWND hwnd = GetFocus();
  if (hwnd == m_wndTreeView)
  {
    pNode = m_wndTreeView.GetSelectedNode();
  }
  else if (hwnd == m_wndListView)
  {
    int iItem = m_wndListView.getSelectionItem();
    pNode = listNodes->getNode(iItem);
  }
  ShowBackTrace(pNode);
}

void CFlowTraceView::ShowInEclipse(LOG_NODE* pNode)
{
  if (pNode && (*gSettings.GetEclipsePath()))
  {
    pNode = pNode->getSyncNode();
    if (pNode && pNode->p_addr_info)
    {
      STARTUPINFO si;
      PROCESS_INFORMATION pi;
      ZeroMemory(&si, sizeof(si));
      si.cb = sizeof(si);
      //D:\Programs\eclipse\eclipse-cpp-neon-M4a-win32-x86_64\eclipsec.exe -name Eclipse --launcher.openFile X:\prj\c\c\ctap\kernel\CTAPparameters\src\CTAP_parameters.c:50
      const int max_cmd = 2 * MAX_PATH;
      char cmd[max_cmd + 1];
      char* src = pNode->p_addr_info->src;
      char* szLinuxHome = gSettings.GetLinuxHome();
      if (strstr(src, szLinuxHome))
        src = strstr(src, szLinuxHome) + strlen(szLinuxHome);
      _sntprintf(cmd, max_cmd, " -name Eclipse --launcher.openFile %s%s:%d", gSettings.GetMapOnWin(), src, pNode->p_addr_info->line);
      CreateProcess(gSettings.GetEclipsePath(), cmd, NULL, NULL, FALSE,
        NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
    }
  }
}

void CFlowTraceView::SyncTree(LOG_NODE* pNode)
{
  if (pNode)
  {
    pNode = pNode->getSyncNode();
    if (pNode)
    {
      m_selectedNode = pNode;
      m_wndTreeView.EnsureNodeVisible(pNode, false);
      m_wndListView.ShowFirstSyncronised(true);

      m_wndListView.Invalidate();
      m_wndTreeView.Invalidate();
    }
  }
}
void CFlowTraceView::SyncViews()
{
  HWND hwnd = GetFocus();
  if (hwnd == m_wndTreeView)
  {
    LOG_NODE* pNode = m_wndTreeView.GetSelectedNode();
    if (pNode)
    {
      m_selectedNode = pNode;
      m_wndListView.ShowFirstSyncronised(true);
    }
  }
  else if (hwnd == m_wndListView)
  {
    int iItem = m_wndListView.getSelectionItem();
    LOG_NODE* pNode = listNodes->getNode(iItem);
    if (pNode)
    {
      pNode = pNode->getSyncNode();
      if (pNode)
      {
        m_selectedNode = pNode;
        m_wndTreeView.EnsureNodeVisible(pNode, false);
        m_wndListView.ShowFirstSyncronised(true);
      }
    }
  }
  else if (hwnd == m_wndBackTraceView)
  {
    LOG_NODE* pNode = m_wndBackTraceView.GetSelectedNode();
    if (pNode)
    {
      SyncTree(pNode);
    }
  }
  m_wndListView.Invalidate();
  m_wndTreeView.Invalidate();
}

void CFlowTraceView::ShowStackView(bool show)
{
  //m_wndBackTraceView.ShowWindow(show ? SW_SHOW : SW_HIDE);
  //SetChildPos(0, 0, false);
  gSettings.SetInfoHiden(!show);
  if (show)
  {
    m_wndHorzSplitter.SetSinglePaneMode(SPLIT_PANE_NONE);
  }
  else
  {
    m_wndHorzSplitter.SetSinglePaneMode(SPLIT_PANE_TOP);
  }
}

void CFlowTraceView::ShowTreeView(bool show)
{
  if (show)
  {
    m_wndVertSplitter.SetSinglePaneMode(SPLIT_PANE_NONE);
  }
  else
  {
    m_wndVertSplitter.SetSinglePaneMode(SPLIT_PANE_RIGHT);
  }
}
