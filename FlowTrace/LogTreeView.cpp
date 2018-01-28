#include "stdafx.h"
#include "Resource.h"
#include "LogTreeView.h"
#include "FlowTraceView.h"
#include "Settings.h"
#include "ProgressDlg.h"

enum STATE_IMAGE{ STATE_IMAGE_COLAPSED, STATE_IMAGE_EXPANDED, STATE_IMAGE_CHECKED, STATE_IMAGE_UNCHECKE };
static const int min_colWidth = 16;

CLogTreeView::CLogTreeView(CFlowTraceView* pView)
  : m_pView(pView)
  , m_recCount(0)
#ifndef NATIVE_TREE
  , m_pSelectedNode(0)
  , m_rowHeight(1)
#endif
{
  m_hTypeImageList = ImageList_Create(16, 16, ILC_MASK, 1, 0);
  ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_ROOT))); //0
  ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_APP))); //1
  ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_PROC))); //2
  ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_PAIRED))); //3
  ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_ENTER))); //4
  ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_EXIT))); //5  

#ifndef NATIVE_TREE
  m_colWidth = min_colWidth;
  m_Initialised = false;
  m_hdc = CreateCompatibleDC(NULL);// CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);

  LOGBRUSH LogBrush;
  LogBrush.lbColor = RGB(0, 0, 0);
  LogBrush.lbStyle = PS_SOLID;
  hDotPen = ExtCreatePen(PS_COSMETIC | PS_ALTERNATE, 1, &LogBrush, 0, NULL);  

  m_hStateImageList = ImageList_Create(16, 16, ILC_MASK, 1, 0);
  ImageList_AddIcon(m_hStateImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_NODE_COLAPSED))); //0 STATE_IMAGE_COLAPSED
  ImageList_AddIcon(m_hStateImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_NODE_EXPANDED))); //1 STATE_IMAGE_EXPANDED
  ImageList_AddIcon(m_hStateImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_NODE_CHECKED))); //2 STATE_IMAGE_CHECKED
  ImageList_AddIcon(m_hStateImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_NODE_UNCHECKE))); //3 STATE_IMAGE_UNCHECKE
#endif
}


CLogTreeView::~CLogTreeView()
{
#ifndef NATIVE_TREE
  DeleteDC(m_hdc); 
#endif
}

void CLogTreeView::ApplySettings(bool fontChanged)
{
  if (fontChanged)
  {
    SetFont(gSettings.GetFont());
#ifndef NATIVE_TREE
    SelectObject(m_hdc, gSettings.GetFont());
#endif
  }
  Invalidate();
}

void CLogTreeView::Clear()
{
  m_recCount = 0;
#ifndef NATIVE_TREE
  SetItemCountEx(0, 0);
  m_colWidth = min_colWidth;
  SetColumnWidth(0, m_colWidth);
#else
  DeleteAllItems();
#endif
  ::SendMessage(hWndStatusBar, SB_SETTEXT, 0, (LPARAM)"");
}

LRESULT CLogTreeView::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
  bHandled = TRUE;

  if (GetFocus() != *this)
    SetFocus();

  int xPos = GET_X_LPARAM(lParam);
  int yPos = GET_Y_LPARAM(lParam);

  LOG_NODE* pNode = NULL;
#ifdef NATIVE_TREE
  DWORD dwpos = GetMessagePos();
  TVHITTESTINFO ht = { 0 };
  ht.pt.x = GET_X_LPARAM(dwpos);
  ht.pt.y = GET_Y_LPARAM(dwpos);
  ::MapWindowPoints(HWND_DESKTOP, m_hWnd, &ht.pt, 1);
  HitTest(&ht);
  if (TVHT_ONITEM & ht.flags)
  {
    TVITEM tvItem;
    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = ht.hItem;
    if (GetItem(&tvItem))
    {
      TVITEM tvi = { 0 };
      tvi.hItem = tvItem.hItem;
      GetItem(&tvi);
      pNode = (LOG_NODE*)tvi.lParam;
    }
}
#else
  int iItem = ItemByPos(yPos);
  if (iItem >= 0)
  {
    pNode = getTreeNode(iItem);
  }
#endif

  if (pNode)
  {
    if (m_pSelectedNode != pNode)
    {
      if (m_pSelectedNode != NULL)
      {
        int iCurSelected = m_pSelectedNode->GetPosInTree();
        RedrawItems(iCurSelected, iCurSelected);
      }
      SetSelectedNode(pNode);
      RedrawItems(iItem, iItem);
    }

    DWORD dwFlags;
    POINT pt = { xPos, yPos };
    ClientToScreen(&pt);
    HMENU hMenu = CreatePopupMenu();
    dwFlags = MF_BYPOSITION | MF_STRING;
    if (!pNode->isFlow())
      dwFlags |= MF_DISABLED;
    //InsertMenu(hMenu, 0, dwFlags, ID_TREE_COPY_CALL_PATH, _T("Copy call stack"));
    dwFlags = MF_BYPOSITION | MF_STRING;
    InsertMenu(hMenu, 0, dwFlags, ID_TREE_COPY, _T("Copy"));
    dwFlags = MF_BYPOSITION | MF_STRING;
    if (!pNode->lastChild)
      dwFlags |= MF_DISABLED;
    InsertMenu(hMenu, 0, dwFlags, ID_TREE_COLLAPSE_ALL, _T("Collapse All"));
    dwFlags = MF_BYPOSITION | MF_STRING;
    if (!pNode->lastChild)
      dwFlags |= MF_DISABLED;
    InsertMenu(hMenu, 0, dwFlags, ID_TREE_EXPAND_ALL, _T("Expand All"));
    dwFlags = MF_BYPOSITION | MF_STRING;
    UINT nRet = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, 0);
    DestroyMenu(hMenu);
    //stdlog("%u\n", GetTickCount());
    if (nRet == ID_TREE_EXPAND_ALL)
    {
      CollapseExpandAll(pNode, true);
    }
    else if (nRet == ID_TREE_COLLAPSE_ALL)
    {
      CollapseExpandAll(pNode, false);
    }
    else if (nRet == ID_TREE_COPY)
    {
      ::SendMessage(hwndMain, WM_COMMAND, ID_EDIT_COPY, 0);
    }
    //stdlog("%u\n", GetTickCount());
  }
  return 0;
}
#ifdef NATIVE_TREE
void CLogTreeView::CollapseExpandAll(LOG_NODE* pNode, bool expand)
{
  Expand(pNode->htreeitem, expand ? TVE_EXPAND : TVE_COLLAPSE);
  pNode = pNode->lastChild;
  while (pNode)
  {
    Expand(pNode->htreeitem, expand ? TVE_EXPAND : TVE_COLLAPSE);
    CollapseExpandAll(pNode, expand);
    pNode = pNode->prevSibling;
  }
}
#else
void CLogTreeView::CollapseExpandAll(LOG_NODE* pNode, bool expand)
{
  pNode->CollapseExpandAll(expand);
  SetItemCount(rootNode->GetExpandCount() + 1);
  RedrawItems(0, rootNode->GetExpandCount());
}
#endif

void CLogTreeView::CopySelection()
{
  if (!m_pSelectedNode)
    return;
  int cbText;
  char* szText;
  szText = m_pSelectedNode->getTreeText(&cbText, false);
  Helpers::CopyToClipboard(m_hWnd, szText, cbText);
}

void CLogTreeView::RefreshTree(bool showAll)
{
  DWORD newCount = gArchive.getCount();

  if (newCount <= m_recCount)
  {
    ATLASSERT(newCount == m_recCount);
    return;
  }

//  if (!showAll)
//  {
//#define TREE_UPDATE_CHUNK 10000
//    if ((newCount - m_recCount) > TREE_UPDATE_CHUNK)
//      newCount = m_recCount + TREE_UPDATE_CHUNK;
//  }

#ifdef NATIVE_TREE

  if (rootNode->htreeitem == 0)
  {
    TVINSERTSTRUCT tvs = { 0 };
    tvs.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    tvs.hInsertAfter = TVI_ROOT;
    tvs.item.lParam = (LPARAM)rootNode;
    tvs.item.iImage = tvs.item.iSelectedImage = 0;
    tvs.item.cChildren = 1;
    tvs.item.pszText = rootNode->getTreeText();
    rootNode->htreeitem = InsertItem(&tvs);
    //remove check box 
    TVITEM tvi;
    tvi.hItem = rootNode->htreeitem;
    tvi.mask = TVIF_STATE;
    tvi.stateMask = TVIS_STATEIMAGEMASK;
    tvi.state = 0;
    SetItem(&tvi);
  }
  else
  {
    TVITEM tvi = { 0 };
    tvi.mask = TVIF_TEXT;
    tvi.hItem = rootNode->htreeitem;
    tvi.pszText = rootNode->getTreeText();
    SetItem(&tvi);
  }

  //add new logs
  for (DWORD i = m_recCount; i < newCount; i++)
  {
    LOG_NODE* pNode = gArchive.getNode(i);
    LOG_DATA*  pData = pNode->data;

    TVINSERTSTRUCT tvs = { 0 };
    tvs.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    if (!pData->isTrace() && pNode->parent)
    {
      tvs.item.pszText = LPSTR_TEXTCALLBACK;
      tvs.item.iSelectedImage = tvs.item.iImage = I_IMAGECALLBACK;
      tvs.item.lParam = (LPARAM)pNode;

      tvs.hParent = pNode->parent->htreeitem;
      tvs.hInsertAfter = TVI_LAST;
      pNode->htreeitem = InsertItem(&tvs);

      if ((pNode->isApp() || pNode->isProc()))
      {
        SetCheckState(pNode->htreeitem, TRUE);
      }
      else
      {
        //remove check box if itis flow node
        TVITEM tvi;
        tvi.hItem = pNode->htreeitem;
        tvi.mask = TVIF_STATE;
        tvi.stateMask = TVIS_STATEIMAGEMASK;
        tvi.state = 0;
        SetItem(&tvi);
      }
    }
  }
#else
  if (!m_Initialised)
    return;

  int prevExpanded = rootNode->GetExpandCount();
  int firstAffected = -1;
  //int firstRow = prevExpanded ? 0 : GetTopIndex();
  //int lastRow = m_size.cy / m_rowHeight + 1;
  rootNode->CalcLines();

  //add new logs
  for (DWORD i = m_recCount; i < newCount; i++)
  {
    LOG_NODE* pNode = gArchive.getNode(i);
    
    if (!pNode->isTrace())
    {
      LOG_NODE* p = pNode->isTreeNode() ? pNode : ((FLOW_NODE*)pNode)->getPeer();
      //if(p) stdlog("pathExpanded = %d line = %d\n", p->parent->pathExpanded, p->parent->line);
      if (p && p->parent->pathExpanded)
      {
        if (p->parent->GetExpandCount() > 0 && !p->parent->hasNodeBox)
        {
          p->parent->hasNodeBox = 1;
          if (firstAffected == -1 || firstAffected > p->parent->line)
            firstAffected = p->parent->line;
        }
        if (p->parent->expanded)
        {
          if (firstAffected == -1 || firstAffected > p->line)
            firstAffected = p->line;
        }
      }
    }
  }

  if (m_recCount != 0)
  {
    if (prevExpanded != rootNode->GetExpandCount())
    {
      SetItemCountEx(rootNode->GetExpandCount() + 1, LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);//LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL
    }
    if (firstAffected > -1)
    {
      RedrawItems(firstAffected, rootNode->GetExpandCount());
    }
  }

#endif
  if (m_recCount == 0)
    SetItemCountEx(1, LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);//LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL

  m_recCount = newCount;

  static TCHAR pBuf[128];
  _sntprintf(pBuf, sizeof(pBuf) - 1, TEXT("Log: %s"), Helpers::str_format_int_grouped(m_recCount));
  ::SendMessage(hWndStatusBar, SB_SETTEXT, 0, (LPARAM)pBuf);
}

#ifndef NATIVE_TREE

LOG_NODE* CLogTreeView::getTreeNode(int iItem, int* pOffset)
{
  int offset;
  LOG_NODE* pNode;
  bool did_again = false;

//#ifdef _DEBUG
//  if (iItem == 5)
//    iItem = iItem;
//#endif
  //stdlog("getTreeNode-> %d\n", GetTickCount());

again:
  offset = 0;
  pNode = rootNode;
  ATLASSERT(rootNode->expanded || iItem == 0);

  while (iItem != pNode->line)
  {
    //stdlog("\tline %d offset = %d %s\n", pNode->line, offset, pNode->getTreeText());
    //if (pNode->nextSibling && (pNode->line + pNode->GetExpandCount()) < iItem)
    if (pNode->nextSibling && pNode->nextSibling->line <= iItem)
    {
      while (pNode->nextChank && pNode->nextChank->line <= iItem)
      {
        //stdlog("\t1 index = %d line %d offset = %d %s\n", pNode->index, pNode->line, offset, pNode->getTreeText());
        pNode = pNode->nextChank;
        //stdlog("\t\t1 index = %d line %d offset = %d %s\n", pNode->index, pNode->line, offset, pNode->getTreeText());
      }
      while (pNode->nextSibling && pNode->nextSibling->line <= iItem)
      {
        //stdlog("\t2 index = %d line %d offset = %d %s\n", pNode->index, pNode->line, offset, pNode->getTreeText());
        pNode = pNode->nextSibling;
        //stdlog("\t\t2 index = %d line %d offset = %d %s\n", pNode->index, pNode->line, offset, pNode->getTreeText());
      }
    }
    else
    {
      //stdlog("\t\t\t3 index = %d line %d offset = %d %s\n", pNode->index, pNode->line, offset, pNode->getTreeText());
      
      if (!pNode->firstChild)
      {
        if (!did_again)
        {
          did_again = true;
          goto again;
        }
        else
        {
          //!!ATLASSERT(0);
          break;
        }
      }
      offset++;
      pNode = pNode->firstChild;
    }
    //stdlog("\tiItem %d offset = %d %s\n", iItem, offset, pNode->getTreeText());
  }
  //stdlog("getTreeNode<- %d\n", GetTickCount());
  //stdlog("\tiItem %d offset = %d %s\n", iItem, offset, pNode->getTreeText());
  if (pOffset)
    *pOffset = offset;
  return pNode;
}

void CLogTreeView::SetColumnLen(int len)
{
  RECT rc;
  GetClientRect(&rc);
  len = max(rc.right, len) + 16;
  if (m_colWidth < len)
  {
    //stdlog("m_colWidth %d len %d col %drc.right %d\n", m_colWidth, len, col, rc.right);
    m_colWidth = len;
    SetColumnWidth(0, len);
    Invalidate(FALSE);
  }
}

void CLogTreeView::OnSize(UINT nType, CSize size)
{
  m_size = size;
  if (!m_Initialised)
  {
    m_Initialised = true;
    InsertColumn(0, "");
  }
}

void CLogTreeView::ShowItem(DWORD i, bool scrollToMiddle)
{
  EnsureVisible(i, FALSE);
  if (scrollToMiddle)
  {
    SIZE size = { 0, 0 };
    RECT rect = { 0 };
    GetClientRect(&rect);
    size.cy = rect.bottom;
    POINT pt;
    GetItemPosition(i, &pt);
    //if (pt.y > rect.bottom / 2)
    {
      size.cy = pt.y - rect.bottom / 2;
      Scroll(size);
    }
  }
}

void CLogTreeView::EnsureItemVisible(int iItem)
{
  iItem = max(0, min(iItem, (int)m_recCount - 1));
  int offset;
  LOG_NODE* pNode = getTreeNode(iItem, &offset);
  if (!pNode){ ATLASSERT(0); return; }

  ShowItem(iItem, false);

  RECT rcClient, rcItem;
  GetClientRect(&rcClient);
  ListView_GetSubItemRect(m_hWnd, iItem, 0, LVIR_BOUNDS, &rcItem);
  //GetSubItemRect(iItem, 0, LVIR_BOUNDS, &rcItem);

  int cbText;
  char* szText = pNode->getTreeText(&cbText);

  int cxText, xStart, xEnd;
  GetNodetPos(m_hdc, pNode->hasCheckBox, offset, szText, cbText, cxText, xStart, xEnd);

  SetColumnLen(xEnd);

  SCROLLINFO si;
  si.cbSize = sizeof(si);
  si.fMask = SIF_RANGE | SIF_POS;
  GetScrollInfo(SB_HORZ, &si);

  SIZE size1 = { 0, 0 };
  SIZE size2 = { 0, 0 };
  if (xStart < si.nPos)
  {
    size1.cx = xStart - si.nPos - 16;
    Scroll(size1);
  }
  if (xEnd >= rcClient.right - rcItem.left)
  {
    size2.cx = xEnd - rcClient.right + rcItem.left + 16;
    Scroll(size2);
  }
  //stdlog("pNode = %p iItem = %d offset = %d size1.cx = %d size2.cx = %d xStart = %d cxText = %d xEnd = %d si.nPos = %d si.nMin = %d si.nMax = %d rcClient.left = %d rcClient.right = %d rcItem.left = %d rcItem.right = %d szText = %s\n", pNode, iItem, offset, size1.cx, size2.cx, xStart, cxText, xEnd, si.nPos, si.nMin, si.nMax, rcClient.left, rcClient.right, rcItem.left, rcItem.right, szText);
}

void CLogTreeView::EnsureNodeVisible(LOG_NODE* pNode, bool select, bool collapseOthers)
{
  if (collapseOthers)
    CollapseExpandAll(rootNode, false);

  LOG_NODE* p = pNode->parent;
  //stdlog("%u\n", GetTickCount());
  while (p)
  {
    if (p->expanded == 0)
      p->CollapseExpand(TRUE);
    p = p->parent;
  }  
  //stdlog("%u\n", GetTickCount());

  p = pNode->parent;
  while (p)
  {
    ATLASSERT(!p->lastChild || p->expanded);
    p = p->parent;
  }
  SetItemCount(rootNode->GetExpandCount() + 1);

  int iItem = pNode->GetPosInTree();
  EnsureItemVisible(iItem);
  if (select)
  {
    if (m_pSelectedNode != NULL)
    {
      int iCurSelected = m_pSelectedNode->GetPosInTree();
      RedrawItems(iCurSelected, iCurSelected);
    }
    SetSelectedNode(pNode);
  }
  RedrawItems(iItem, iItem);
}

void CLogTreeView::SetSelectedNode(LOG_NODE* pNode)
{
  m_pSelectedNode = pNode;
  m_pView->ShowBackTrace(pNode);
}

LRESULT CLogTreeView::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
  bHandled = TRUE;

  if (m_pSelectedNode == NULL)
    return 0;

  int iCurSelected = m_pSelectedNode->GetPosInTree();
  int iNewSelected = iCurSelected;

  bool bShiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
  bool bCtrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  bool redraw = true;

  switch (wParam)
  {
  case VK_SPACE:       
  case VK_RETURN:       
  {
    // just enshure visible
  }
  break;

  case VK_HOME:       // Home 
  {
    iNewSelected = 0;
  }
  break;
  case VK_END:        // End 
  {
    iNewSelected = GetItemCount();
  }
  break;
  case VK_PRIOR:      // Page Up 
  case VK_NEXT:       // Page Down 
  {
    CClientDC dc(m_hWnd);
    RECT rcItem, rcClient;
    GetClientRect(&rcClient);
    GetItemRect(0, &rcItem, LVIR_BOUNDS);
    int count = (rcClient.bottom - rcClient.top) / (rcItem.bottom - rcItem.top);
    if (wParam == VK_PRIOR)
      iNewSelected -= count;
    else
      iNewSelected += count;
  }
  break;
  case VK_LEFT:       // Left arrow 
  {
    LOG_NODE* pNode = m_pSelectedNode;
    if (pNode && pNode->expanded)
    {
      pNode->CollapseExpand(FALSE);
      SetItemCount(rootNode->GetExpandCount() + 1);
    }
    else if (pNode && pNode->parent && pNode->parent->expanded)
    {
      iNewSelected = pNode->parent->GetPosInTree();
    }
  }
  break;
  case VK_RIGHT:      // Right arrow 
  {
    LOG_NODE* pNode = m_pSelectedNode;
    if (pNode && !pNode->expanded && pNode->lastChild)
    {
      pNode->CollapseExpand(TRUE);
      SetItemCount(rootNode->GetExpandCount() + 1);
    }
    else
      iNewSelected++;
  }
  break;
  case VK_UP:         // Up arrow 
  {
    iNewSelected--;
  }
  break;
  case VK_DOWN:       // Down arrow 
  {
    iNewSelected++;
  }
  break;
  default:
    redraw = false;
  }

  if (iNewSelected > GetItemCount() - 1)
    iNewSelected = GetItemCount() - 1;
  if (iNewSelected < 0)
    iNewSelected = 0;

  if (redraw)
  {
    SetSelectedNode(getTreeNode(iNewSelected));
    RedrawItems(iCurSelected, iCurSelected);
    RedrawItems(iNewSelected, iNewSelected);
    EnsureItemVisible(iNewSelected);
  }
  return 0;
}

#ifndef NATIVE_TREE
int CLogTreeView::ItemByPos(int yPos)
{
  CRect rcItem = { 0 };
  GetSubItemRect(0, 0, LVIR_LABEL, &rcItem);

  LVHITTESTINFO ht = { 0 };
  ht.pt.x = rcItem.left;// xPos;
  ht.pt.y = yPos;
  ht.iItem = -1;

  SubItemHitTest(&ht);
  //stdlog("yPos = %d x = %d left = %d flags = %d ht.iItem = %d\n", yPos, ht.pt.x, rcItem.left, ht.flags, ht.iItem);
  return ht.iItem;
 }
#endif

LRESULT CLogTreeView::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
  if (GetFocus() != *this)
    SetFocus();

  bHandled = TRUE;


  int xPos = GET_X_LPARAM(lParam);
  int yPos = GET_Y_LPARAM(lParam);
  int iItem = ItemByPos(yPos);

  if (iItem >= 0)
  {
    int offset;
    LOG_NODE* pNode = getTreeNode(iItem, &offset);
    if (!pNode)
    {
      ATLASSERT(FALSE);
      return 0;
    }

    CRect rcItem = { 0 };
    GetSubItemRect(iItem, 0, LVIR_LABEL, &rcItem);
    LOG_DATA* pLogData = pNode->data;
    xPos -= rcItem.left;
    if (hitTest(pNode, xPos, offset) == TVHT_ONITEMBUTTON)
    {      
      pNode->CollapseExpand(!pNode->expanded);
      SetItemCount(rootNode->GetExpandCount() + 1);
    }
    else if (hitTest(pNode, xPos, offset) == TVHT_ONITEMSTATEICON)
    {
      pNode->checked = !pNode->checked;
      ::PostMessage(hwndMain, WM_UPDATE_FILTER, 0, (LPARAM)pNode);
    }

    if (m_pSelectedNode != NULL)
    {
      int iCurSelected = m_pSelectedNode->GetPosInTree();
      RedrawItems(iCurSelected, iCurSelected);
    }
    if (m_pSelectedNode != pNode)
    {
      SetSelectedNode(pNode);
      RedrawItems(iItem, iItem);
      //EnsureItemVisible(iItem);
    }
  }
  return 0;
}

int CLogTreeView::hitTest(LOG_NODE* pNode, int xPos, int offset)
{
  int x0 = offset * ICON_OFFSET;
  int x1 = x0 + ICON_LEN;
  if (pNode->lastChild) //has expand button
  {
    if (xPos >= x0 && xPos < x1)
      return TVHT_ONITEMBUTTON;
  }
  if (pNode->hasCheckBox) //has state button
  {
    x0 += ICON_OFFSET;
    x1 = x0 + ICON_LEN;
    if (xPos >= x0 && xPos < x1)
      return TVHT_ONITEMSTATEICON;
  }
  return TVHT_ONITEM;
}

void CLogTreeView::GetNodetPos(HDC hdc, BOOL hasCheckBox, int offset, char* szText, int cbText, int &cxText, int &xStart, int &xEnd)
{
  xStart = offset * ICON_OFFSET;
  xEnd = xStart;
  //next to button
  xEnd += ICON_OFFSET;
  if (hasCheckBox)
  {
    //next to checkox
    xEnd += ICON_OFFSET;
  }
  //next to image
  xEnd += ICON_OFFSET;

  SIZE size;
  GetTextExtentPoint32(hdc, szText, cbText, &size);
  cxText = size.cx;
  xEnd += size.cx;
}

void CLogTreeView::DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rcItem)
{
  if (iSubItem != 0)
  {
    ATLASSERT(FALSE);
    return;
  }
  m_rowHeight = rcItem.bottom - rcItem.top;
  int offset;
  LOG_NODE* pNode = getTreeNode(iItem, &offset);
  if (!pNode)
  {
    ATLASSERT(FALSE);
    return;
  }
  //POINT  point = { rcItem.left + offset * ICON_OFFSET, rcItem.top };

  //stdlog("DrawSubItem: iItem = %d\n", iItem);
  //stdlog("offset = %d rcItem l = %d r = %d t = %d b = %d\n", offset, rcItem.left, rcItem.right, rcItem.top, rcItem.bottom);
  int left = rcItem.left + offset * ICON_OFFSET;
  int top = rcItem.top;
  
  //HBRUSH hbr = ::CreateSolidBrush(RGB(255,255,255));
  //::FillRect(hdc, &rcItem, hbr);
  //::DeleteObject(hbr);

  int yMiddle = top + (rcItem.bottom - top) / 2;
  int yIconTop = yMiddle - ICON_LEN / 2;

  ::SelectObject(hdc, hDotPen);
  MoveToEx(hdc, left + 1 + ICON_LEN / 2, yMiddle, 0);
  LineTo(hdc, left + ICON_OFFSET, yMiddle);

  if (pNode->lastChild)
  {
    ImageList_Draw(m_hStateImageList, pNode->expanded ? STATE_IMAGE_EXPANDED : STATE_IMAGE_COLAPSED, hdc, left, yIconTop, ILD_NORMAL);
  }
  //next to button
  left += ICON_OFFSET;

  if (pNode->hasCheckBox)
  {
    ImageList_Draw(m_hStateImageList, pNode->checked ? STATE_IMAGE_CHECKED : STATE_IMAGE_UNCHECKE, hdc, left, yIconTop, ILD_NORMAL);
    //next to checkox
    left += ICON_OFFSET;
  }

  ImageList_Draw(m_hTypeImageList, pNode->getTreeImage(), hdc, left, yIconTop, ILD_NORMAL);
  //next to image
  left += ICON_OFFSET;

  int cbText;
  
  char* szText = pNode->getTreeText(&cbText);
  int cxText, xStart, xEnd;
  GetNodetPos(hdc, pNode->hasCheckBox, offset, szText, cbText, cxText, xStart, xEnd);
  ATLASSERT(xEnd + rcItem.left == left + cxText);
  
  SetColumnLen(xEnd);

  COLORREF old_textColor = ::GetTextColor(hdc);
  COLORREF old_bkColor = ::GetBkColor(hdc);
  int old_bkMode = ::GetBkMode(hdc);


  if (m_pView->selectedNode() == pNode)
  {
    RECT rc = rcItem;
    rc.left = left - 2;
    rc.right = rc.left + cxText + 4;
    CBrush brush1;
    brush1.CreateSolidBrush(RGB(0, 255, 128)); //gSettings.GetSyncColor()
    FillRect(hdc, &rc, brush1);

    if (m_pSelectedNode == pNode)
    {
      CBrush brush2;
      brush2.CreateSolidBrush(gSettings.SelectionBkColor(GetFocus() == m_hWnd));
      FrameRect(hdc, &rc, brush2);
      ::SetTextColor(hdc, gSettings.SelectionTxtColor());
    }

    ::SetBkMode(hdc, TRANSPARENT);
  }
  else
  {
    ::SetBkMode(hdc, OPAQUE);
    if (m_pSelectedNode == pNode)
    {
      ::SetBkColor(hdc, gSettings.SelectionBkColor(GetFocus() == m_hWnd));
      ::SetTextColor(hdc, gSettings.SelectionTxtColor());
    }
  }

  TextOut(hdc, left, top, szText, cbText);

  int i = 0, x = left - ICON_OFFSET - ICON_LEN/2 - 5;
  for (LOG_NODE* p = pNode; p; i++, x = x - ICON_OFFSET)
  {
    bool drawUp = false, drawDawn = false, isParent = p != pNode;
    drawUp = drawUp || (!isParent && (p->prevSibling || p->parent));
    drawUp = drawUp || (isParent && p->nextSibling);
    drawDawn = drawDawn || (!isParent && p->nextSibling);
    drawDawn = drawDawn || (isParent && p->nextSibling);
    if (!isParent && p->hasCheckBox)
      x = x - ICON_OFFSET;

    if (drawUp)
    {
      MoveToEx(hdc, x, top, 0);
      LineTo(hdc, x, isParent ? yMiddle + 1 : (p->lastChild ? yMiddle - 5 : (p->hasCheckBox ? yMiddle - 3 : yMiddle + 1)));
    }
    if (drawDawn)
    {
      MoveToEx(hdc, x, rcItem.bottom, 0);
      LineTo(hdc, x, isParent ? yMiddle : (p->lastChild ? yMiddle + 6 : (p->hasCheckBox ? yMiddle + 4 : yMiddle)));
    }
    p = p->parent;    
  }

  ::SetTextColor(hdc, old_textColor);
  ::SetBkColor(hdc, old_bkColor);
  ::SetBkMode(hdc, old_bkMode);
}
#endif
