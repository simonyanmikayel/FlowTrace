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

  dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL;
  if (!gSettings.GetInfoHiden())
    dwStyle |= WS_VISIBLE;
  m_wndInfo.Create(m_wndHorzSplitter, rcDefault, NULL, dwStyle, 0);

  m_wndListView.Create(m_wndVertSplitter, rcDefault, NULL,
    WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
    LVS_REPORT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_OWNERDATA | LVS_NOCOLUMNHEADER,
    0);

#ifdef NATIVE_TREE
  m_wndTreeView.Create(m_wndVertSplitter, rcDefault, NULL,
    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
    TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_CHECKBOXES, //TVS_SINGLEEXPAND
    WS_EX_CLIENTEDGE);
  m_wndTreeView.SetImageList(m_wndTreeView.m_hTypeImageList);
#else
  m_wndTreeView.Create(m_wndVertSplitter, rcDefault, NULL,
    WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
    LVS_REPORT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_OWNERDATA | LVS_NOCOLUMNHEADER,
    LVS_EX_FULLROWSELECT);
#endif

  m_wndHorzSplitter.SetSplitterPanes(m_wndVertSplitter, m_wndInfo);
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
}

LRESULT CFlowTraceView::OnLvnEndScroll(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
  m_wndListView.UpdateCaret();
  return 0;
}

void CFlowTraceView::ShowInfo(TCHAR* szText)
{
  m_wndInfo.SetWindowText(szText);
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
  m_selectedNode = NULL;
}

LRESULT CFlowTraceView::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
  bHandled = FALSE;
#ifdef NATIVE_TREE
  LPNMHDR lpnmh = (LPNMHDR)lParam;
  if (lpnmh->hwndFrom == m_wndTreeView)
  {
    switch (lpnmh->code)  // let us filter notifications
    {
    case TVN_KEYDOWN:  // tree has keyboard focus and user pressed a key
    {
      LPNMTVKEYDOWN ptvkd = (LPNMTVKEYDOWN)lParam;
      if (ptvkd->wVKey == ' ')
      {
        HTREEITEM ht = m_wndTreeView.GetSelectedItem();
        ::PostMessage(hwndMain, WM_UPDATE_FILTER, (WPARAM)ht, (LPARAM)getTreeNode(ht));
      }
    }
    break;
    case NM_CLICK:  // user clicked on a tree
    {
      TVHITTESTINFO ht = { 0 };

      DWORD dwpos = GetMessagePos();

      // include <windowsx.h> and <windows.h> header files
      ht.pt.x = GET_X_LPARAM(dwpos);
      ht.pt.y = GET_Y_LPARAM(dwpos);
      ::MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

      TreeView_HitTest(lpnmh->hwndFrom, &ht);

      if (TVHT_ONITEMSTATEICON & ht.flags)
      {
        TVITEM tvItem;

        tvItem.mask = TVIF_HANDLE | TVIF_STATE;
        tvItem.hItem = (HTREEITEM)ht.hItem;
        tvItem.stateMask = TVIS_STATEIMAGEMASK;
                
        if (m_wndTreeView.GetItem(&tvItem))
        {
          HTREEITEM ht = tvItem.hItem;
          ::PostMessage(hwndMain, WM_UPDATE_FILTER, (WPARAM)ht, (LPARAM)getTreeNode(ht));
        }

      }
    }
    default:
      break;
    }
  }
#endif
  return 0;
}

#ifdef NATIVE_TREE
LRESULT CFlowTraceView::OnTvnGetDispInfo(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
  NMTVDISPINFO* pDispInfo = (NMTVDISPINFO*)pnmh;
  TVITEM* pItem = &(pDispInfo)->item;
  LOG_NODE* pNode = (LOG_NODE*)pItem->lParam;
  LOG_DATA*  pData = pNode->data;

  if (pItem->mask & TVIF_TEXT)
  {
    pItem->pszText = pNode->getTreeText();
  }

  if (pItem->mask & TVIF_IMAGE || pItem->mask & TVIF_SELECTEDIMAGE)
  {
    pItem->iSelectedImage = pItem->iImage = pNode->getTreeImage();
  }
  if (pItem->mask & TVIF_CHILDREN)
  {
    ATLASSERT(FALSE);
    pItem->cChildren = pNode->lastChild ? 1 : 0;
  }
  return 1;
}
#endif

LRESULT CFlowTraceView::OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
  if (pnmh->hwndFrom == m_wndTreeView)
  {
#ifdef NATIVE_TREE
    LPNMTVCUSTOMDRAW pNMTVCD = (LPNMTVCUSTOMDRAW)pnmh;
    //return CDRF_DODEFAULT;
    if (pNMTVCD == NULL)
    {
      return -1;
    }
    switch (pNMTVCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
      return CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
      return CDRF_NOTIFYPOSTPAINT;
    case CDDS_ITEMPOSTPAINT:
    {
      HTREEITEM hItem = (HTREEITEM)pNMTVCD->nmcd.dwItemSpec;
      LOG_NODE* pNode = getTreeNode(hItem);
      if (pNode == m_selectedNode)
      {
        RECT rc;
        if (TreeView_GetItemRect(pnmh->hwndFrom, hItem, &rc, TRUE))
        {
          HDC hdc = pNMTVCD->nmcd.hdc;
          CBrush brush;
          brush.CreateSolidBrush(RGB(0, 0, 255));
          FrameRect(hdc, &rc, brush);
        }
      }

    }
    return CDRF_DODEFAULT;
    }
#else
    LPNMLVCUSTOMDRAW pNMLVCD = (LPNMLVCUSTOMDRAW)pnmh;
    switch (pNMLVCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
      return CDRF_NOTIFYSUBITEMDRAW;          // ask for subitem notifications.
    case CDDS_ITEMPREPAINT:
      //m_wndTreeView.DrawItem(pNMLVCD->nmcd.dwItemSpec, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
      return CDRF_NOTIFYSUBITEMDRAW;
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: // recd when CDRF_NOTIFYSUBITEMDRAW is returned in
    {                                    // response to CDDS_ITEMPREPAINT.
      m_wndTreeView.DrawSubItem(pNMLVCD->nmcd.dwItemSpec, pNMLVCD->iSubItem, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
      return CDRF_SKIPDEFAULT;
    }
    break;
    }
    return CDRF_DODEFAULT;
#endif
  }
  else if (pnmh->hwndFrom == m_wndListView)
  {
    LPNMLVCUSTOMDRAW pNMLVCD = (LPNMLVCUSTOMDRAW)pnmh;
    switch (pNMLVCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
      return CDRF_NOTIFYSUBITEMDRAW;          // ask for subitem notifications.
    case CDDS_ITEMPREPAINT:
      m_wndListView.DrawItem(pNMLVCD->nmcd.dwItemSpec, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
      return CDRF_NOTIFYSUBITEMDRAW;
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: // recd when CDRF_NOTIFYSUBITEMDRAW is returned in
    {                                    // response to CDDS_ITEMPREPAINT.
      m_wndListView.DrawSubItem(pNMLVCD->nmcd.dwItemSpec, pNMLVCD->iSubItem, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
      return CDRF_SKIPDEFAULT;
    }
    break;
    }
  }
  return CDRF_DODEFAULT;
}
#ifdef NATIVE_TREE
LOG_NODE* CFlowTraceView::getTreeNode(HTREEITEM hItem)
{
  TVITEM tvi = { 0 };
  tvi.hItem = hItem;
  m_wndTreeView.GetItem(&tvi);
  return (LOG_NODE*)tvi.lParam;
}
#endif
void CFlowTraceView::SyncViews()
{
#ifdef NATIVE_TREE
  if (GetFocus() == m_wndTreeView)
  {
    CTreeItem item = m_wndTreeView.GetSelectedItem();
    if (item.m_hTreeItem)
    {
      LOG_NODE* pNode = getTreeNode(item.m_hTreeItem);
      if (pNode)
      {
        m_selectedNode = pNode;
        m_wndListView.ShowFirstSyncronised(true);
      }
    }
  }
  else 
  if (GetFocus() == m_wndListView)
  {
    LOG_NODE* pNode = listNodes->getNode(m_wndListView.getSelectionItem());
    if (pNode)
    {
      pNode = pNode->getSyncNode();
      if (pNode) 
      {
        m_selectedNode = pNode;
        m_wndTreeView.EnsureVisible(pNode->htreeitem);
      }
    }
  }
  m_wndListView.Invalidate();
  m_wndTreeView.Invalidate();
#else
  if (GetFocus() == m_wndTreeView)
  {
    LOG_NODE* pNode = m_wndTreeView.GetSelectedNode();
    if (pNode)
    {
      m_selectedNode = pNode;
      m_wndListView.ShowFirstSyncronised(true);
    }
  }
  else
    if (GetFocus() == m_wndListView)
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
  m_wndListView.Invalidate();
  m_wndTreeView.Invalidate();
#endif
}

void CFlowTraceView::ShowStackView(bool show)
{
  //m_wndInfo.ShowWindow(show ? SW_SHOW : SW_HIDE);
  //SetChildPos(0, 0, false);
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
