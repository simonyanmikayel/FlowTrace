#include "stdafx.h"
#include "BackTraceView.h"
#include "Settings.h"
#include "Resource.h"
#include "FlowTraceView.h"

const int cMaxBuf = 1204 * 64;
const int max_trace_text = 150;
static char pBuf[cMaxBuf + 1];

CBackTraceView::CBackTraceView(CFlowTraceView* pView)
  : m_pView(pView)
  , m_Initialised(false)
  , c_nodes(0)
  , selItem(-1)
  , selSubItem(-1)
{
}

CBackTraceView::~CBackTraceView()
{
}

static const char* szPending = "pending to resolve call line";

void CBackTraceView::OnSize(UINT nType, CSize size)
{
  if (!m_Initialised)
  {
    m_Initialised = true;
    int cColumns = 0;
    InsertColumn(cColumns++, "func", LVCFMT_LEFT, 16, 0, -1, -1); //BACK_TRACE_FN
    InsertColumn(cColumns++, "line", LVCFMT_RIGHT, 16, 0, -1, -1); //BACK_TRACE_LINE
    InsertColumn(cColumns++, "source", LVCFMT_LEFT, 16, 0, -1, -1); //BACK_TRACE_SRC
  }
}

int CBackTraceView::getSubItemText(int iItem, int iSubItem, char* buf, int cbBuf)
{
  LOG_NODE* pNode = nodes[iItem];
  int cb = 0;
  if (iSubItem == BACK_TRACE_FN)
  {
    cb = min(pNode->getFnNameSize(), cbBuf);
    if (cb != 0)
    {
      memcpy(buf, pNode->getFnName(), cb);
    }
    else
    {
      cb = 2;
      memcpy(buf, "?", cb);
    }
  }
  else if (iSubItem == BACK_TRACE_SRC)
  {
    if (pNode->isTrace())
    {
      cb = pNode->getTraceText(buf, min(max_trace_text, cbBuf));
    }
    else
    {
      const char* txt;
      if (pNode->PendingToResolveAddr())
      {
        txt = szPending;
      }
      else
      {
        txt = pNode->getSrcName(gSettings.GetFullSrcPath());
      }
      cb = min((int)strlen(txt), cbBuf);
      memcpy(buf, txt, cb);
    }
  }
  else if (iSubItem == BACK_TRACE_LINE)
  {
    int line = 0;
    if (pNode->isTrace())
    {
      TRACE_DATA* traceData = ((TRACE_NODE*)pNode)->getData();
      line = traceData->call_line;
    }
    else if (pNode->p_addr_info)
    {
      line = pNode->p_addr_info->line;
    }
    if (line > 0)
      cb = _snprintf(buf, cbBuf, "%d", line);
    else
      cb = _snprintf(buf, cbBuf, "?");
  }
  buf[cb] = 0;
  return cb;
}

void CBackTraceView::UpdateTrace(LOG_NODE* pSelectedNode, bool bNested)
{
  if (!m_Initialised)
    return;

  LOG_NODE* pNode;
  ClearTrace();
  pNode = pSelectedNode->parent ? pSelectedNode : pSelectedNode->getPeer();
  if (bNested)
    pNode = pNode->firstChild;
  while (pNode && (pNode->isFlow() || pNode->isTrace()) && c_nodes < MAX_BACK_TRACE)
  {
    nodes[c_nodes] = pNode;
    c_nodes++;

    if (bNested)
      pNode = pNode->nextSibling;
    else
      pNode = pNode->parent;
  }

  if (c_nodes == 0)
  {
    ClearTrace();
    return;
  }

  HDC hdc = ::CreateCompatibleDC(NULL);
  SelectObject(hdc, gSettings.GetFont());

  SIZE size;
  GetTextExtentPoint32(hdc, "   ", 3, &size);
  int min_col_width = size.cx;
  for (int iSubItem = 0; iSubItem < BACK_TRACE_LAST_COL; iSubItem++)
  {
    subItemColWidth[iSubItem] = 0;
  }
  for (int iItem = 0; iItem < c_nodes; iItem++)
  {
    for (int iSubItem = 0; iSubItem < BACK_TRACE_LAST_COL; iSubItem++)
    {
      size.cx = 0;
      int cb = getSubItemText(iItem, iSubItem, pBuf, cMaxBuf);
      if (cb)
      {
        GetTextExtentPoint32(hdc, pBuf, cb, &size);
      }
      if (subItemColWidth[iSubItem] < size.cx)
      {
        subItemColWidth[iSubItem] = size.cx;
      }
    }
  }
  for (int iSubItem = 0; iSubItem < BACK_TRACE_LAST_COL; iSubItem++)
  {
    SetColumnWidth(iSubItem, subItemColWidth[iSubItem] + min_col_width);
  }
  DeleteDC(hdc);

  SetItemCountEx(c_nodes, LVSICF_NOSCROLL);
}

void CBackTraceView::ClearTrace()
{
  SetItemCountEx(0, 0);
  DeleteAllItems();
  c_nodes = 0;
  selItem = -1;
  selSubItem = -1;
}

void CBackTraceView::CopySelection()
{
  CopySelection(false);
}

void CBackTraceView::CopySelection(bool all)
{
  int cb = 0;
  if (all)
  {
    for (int iItem = 0; iItem < c_nodes && (cb < cMaxBuf - 300); iItem++)
    {
      for (int iSubItem = 0; iSubItem < BACK_TRACE_LAST_COL && (cb < cMaxBuf - 300); iSubItem++)
      {
        cb += getSubItemText(iItem, iSubItem, pBuf + cb, cMaxBuf - cb);
        pBuf[cb++] = '\t';
      }
      pBuf[cb++] = '\r';
      pBuf[cb++] = '\n';
    }
  }
  else
  {
    if (selItem >= 0 && selSubItem >= 0)
      cb = getSubItemText(selItem, selSubItem, pBuf, cMaxBuf);
  }
  Helpers::CopyToClipboard(m_hWnd, pBuf, cb);
}

void CBackTraceView::ItemPrePaint(int iItem, HDC hdc, RECT rc)
{
}

void CBackTraceView::DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rcItem)
{
  POINT  pt = { rcItem.left, rcItem.top };
  COLORREF old_textColor = ::GetTextColor(hdc);
  COLORREF old_bkColor = ::GetBkColor(hdc);
  int old_bkMode = ::GetBkMode(hdc);

  ::SetBkMode(hdc, OPAQUE);
  bool selected = (iItem == selItem && iSubItem == selSubItem);
  if (selected)
  {
    ::SetTextColor(hdc, gSettings.SelectionTxtColor());
    ::SetBkColor(hdc, gSettings.SelectionBkColor(GetFocus() == m_hWnd));
  }

  int cb = getSubItemText(iItem, iSubItem, pBuf, cMaxBuf);
  if (cb)
  {
    //if (!selected) { pBuf[0] = '1'; pBuf[1] = '2'; pBuf[2] = '3'; pBuf[3] = '4'; pBuf[4] = '5';}//memset(pBuf, 'W', cb);
    //stdlog("iItem %d, iSubItem %d, cb %d, pt.x %d, w=%d, %s\n", iItem, iSubItem, cb, pt.x, nodes[iItem].subItemColWidth[iSubItem], pBuf);
    TextOut(hdc, pt.x, pt.y, pBuf, cb);
  }

  ::SetTextColor(hdc, old_textColor);
  ::SetBkColor(hdc, old_bkColor);
  ::SetBkMode(hdc, old_bkMode);
}

LRESULT CBackTraceView::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
  SetSelectionOnMouseEven(uMsg, wParam, lParam);
  return 0;
}

LRESULT CBackTraceView::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
  SetSelectionOnMouseEven(uMsg, wParam, lParam);

  bHandled = TRUE;

  int xPos = GET_X_LPARAM(lParam);
  int yPos = GET_Y_LPARAM(lParam);

  DWORD dwFlags;
  POINT pt = { xPos, yPos };
  ClientToScreen(&pt);
  HMENU hMenu = CreatePopupMenu();
  dwFlags = MF_BYPOSITION | MF_STRING;
  if (*gSettings.GetEclipsePath() == 0)
    dwFlags |= MF_DISABLED;
  InsertMenu(hMenu, 0, dwFlags, ID_SHOW_IN_ECLIPSE, _T("Open in Eclipse"));
  dwFlags = MF_BYPOSITION | MF_STRING;
  if (selItem < 0 || selSubItem < 0)
    dwFlags |= MF_DISABLED;
  InsertMenu(hMenu, 0, dwFlags, ID_SYNC_VIEWES, _T("Synchronize with call tree (Tab)"));
  InsertMenu(hMenu, 0, MF_BYPOSITION | MF_SEPARATOR, ID_TREE_COPY, _T(""));
  dwFlags = MF_BYPOSITION | MF_STRING;
  InsertMenu(hMenu, 0, dwFlags, ID_EDIT_COPY_ALL, _T("Copy All"));
  dwFlags = MF_BYPOSITION | MF_STRING;
  if (selItem < 0 || selSubItem < 0)
    dwFlags |= MF_DISABLED;
  InsertMenu(hMenu, 0, dwFlags, ID_EDIT_COPY, _T("Copy"));
  UINT nRet = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, 0);
  DestroyMenu(hMenu);

  if (nRet == ID_EDIT_COPY)
  {
    CopySelection();
  }
  else if (nRet == ID_EDIT_COPY_ALL)
  {
    CopySelection(true);
  }
  else if (nRet == ID_SYNC_VIEWES)
  {
    if (selItem >= 0)
      m_pView->SyncTree(nodes[selItem]);
  }
  else if (nRet == ID_SHOW_IN_ECLIPSE)
  {
    if (selItem >= 0)
    {
      m_pView->ShowInEclipse(nodes[selItem]);
    }
  }

  return 0;
}

LRESULT CBackTraceView::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
  bHandled = TRUE;
  if (selItem < 0 || selSubItem < 0)
    return 0;
  int curSelItem = selItem;
  int curSelSubItem = selSubItem;

  switch (wParam)
  {
  case VK_HOME:       // Home 
    selSubItem = 0;
    break;
  case VK_RETURN:        // End 
    selItem++;
    break;
  case VK_END:        // End 
    selSubItem = BACK_TRACE_LAST_COL - 1;
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
      selItem -= count;
    else
      selItem += count;
  }
  break;
  case VK_LEFT:       // Left arrow 
    selSubItem--;
    break;
  case VK_RIGHT:      // Right arrow 
    selSubItem++;
    break;
  case VK_UP:         // Up arrow 
    selItem--;
    break;
  case VK_DOWN:       // Down arrow 
    selItem++;
    break;
  }
  if (selItem < 0)
    selItem = 0;
  if (selSubItem < 0)
    selSubItem = 0;
  if (selItem >= c_nodes)
    selItem = c_nodes - 1;
  if (selSubItem >= BACK_TRACE_LAST_COL)
    selSubItem = BACK_TRACE_LAST_COL -1;

  int cb = getSubItemText(selItem, selSubItem, pBuf, cMaxBuf);
  while (cb == 0 && selSubItem < BACK_TRACE_LAST_COL - 1)
  {
    selSubItem++;
    cb = getSubItemText(selItem, selSubItem, pBuf, cMaxBuf);
  }
  while (cb == 0 && selSubItem > 0)
  {
    selSubItem--;
    cb = getSubItemText(selItem, selSubItem, pBuf, cMaxBuf);
  }
  if (curSelItem != selItem || curSelSubItem != selSubItem)
  {
    RedrawItems(curSelItem, curSelItem);
    RedrawItems(selItem, selItem);
  }

  return 0;
}

void CBackTraceView::SetSelectionOnMouseEven(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (GetFocus() != *this)
    SetFocus();

  int xPos = GET_X_LPARAM(lParam);
  int yPos = GET_Y_LPARAM(lParam);

  LVHITTESTINFO ht = { 0 };
  ht.iItem = -1;
  ht.pt.x = xPos;
  ht.pt.y = yPos;
  SubItemHitTest(&ht);
  //stdlog("xPos = %d yPos = %d flags = %d ht.iItem = %d\n", xPos, yPos, ht.flags, ht.iItem);
  if ((ht.iItem >= 0))
  {
    if (selItem != ht.iItem || selSubItem != ht.iSubItem)
    {
      int cb = getSubItemText(ht.iItem, ht.iSubItem, pBuf, cMaxBuf);
      if (cb > 0)
      {
        if (selItem >= 0)
        {
          RedrawItems(selItem, selItem);
        }
        selItem = ht.iItem;
        selSubItem = ht.iSubItem;
        RedrawItems(selItem, selItem);
      }
    }
  }
}

#if(0)
#ifdef _USE_RICH_EDIT_FOR_BACK_TRACE
struct StreamInCallbackCookie { char* buf; int cb; int pos; };
static DWORD CALLBACK MyStreamInCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
  StreamInCallbackCookie* pCookie = (StreamInCallbackCookie*)dwCookie;
  char* pTxt = pCookie->buf + pCookie->pos;
  int c = min(pCookie->cb - pCookie->pos, cb);
  memcpy(pbBuff, pTxt, c);
  pCookie->pos += c;
  *pcb = c;
  return 0;
}
#endif

void CBackTraceView::UpdateTrace(LOG_NODE* pSelectedNode)
{
  LOG_NODE* pNode;

  int max_fn_name = 0;
  pNode = pSelectedNode->parent ? pSelectedNode : pSelectedNode->getPeer();
  while (pNode && (pNode->isFlow() || pNode->isTrace()))
  {
    max_fn_name = max(max_fn_name, pNode->getFnNameSize());
    pNode = pNode->parent;
  }

  int cb = 0;
#ifdef _USE_RICH_EDIT_FOR_BACK_TRACE
  cb += _sntprintf(pBuf + cb, cMaxBuf - cb, R"({\rtf1)");
#endif
  pNode = pSelectedNode->parent ? pSelectedNode : pSelectedNode->getPeer();
  while (pNode && (pNode->isFlow() || pNode->isTrace()) && cb < cMaxBuf - 300)
  {
    DWORD cb_fn_name = min(cMaxBuf - cb, pNode->getFnNameSize());
    memcpy(pBuf + cb, pNode->getFnName(), cb_fn_name);
    cb += cb_fn_name;
    int padding = max_fn_name - cb_fn_name;
    if (padding > 0 && cb + padding + 10 < cMaxBuf)
    {
      memset(pBuf + cb, ' ', padding);
      cb += padding;
    }
    pBuf[cb] = 0;
#ifdef _USE_RICH_EDIT_FOR_BACK_TRACE
    //cb += _sntprintf(pBuf + cb, cMaxBuf - cb, R"({\field{\*\fldinst{ HYPERLINK http://example.com  }}{\fldrslt{ DisplayText}}})");
#endif

    if (pNode->isFlow())
    {
      FLOW_DATA* flowData = ((FLOW_NODE*)pNode)->getData();

      DWORD addr = (DWORD)flowData->call_site;
      if (gSettings.GetResolveAddr())
      {
        ADDR_INFO *p_addr_info = pNode->p_addr_info;
        if (p_addr_info)
        {
          DWORD line = p_addr_info->line;
          DWORD nearest_pc = p_addr_info->addr;
          if (line != INFINITE && addr - nearest_pc < 128)
          {
            char* src = pNode->getSrcName(gSettings.GetFullSrcPath());
            cb += _sntprintf(pBuf + cb, cMaxBuf - cb, "  | from addr: %-8X+%-2X| line: %-6d| src: %s", nearest_pc, addr - nearest_pc, line, src);
          }
          else
          {
            cb += _sntprintf(pBuf + cb, cMaxBuf - cb, "  | from addr: %-8X", addr);
          }
        }
        else if (pNode->PendingToResolveAddr())
        {
          cb += _sntprintf(pBuf + cb, cMaxBuf - cb, "  | from addr: %-8X   | pending to resolve call line", addr);
        }
      }
      else
      {
        cb += _sntprintf(pBuf + cb, cMaxBuf - cb, "  | from addr: %-8X", addr);
      }
    }
    else if (pNode->isTrace())
    {
      TRACE_DATA* traceData = ((TRACE_NODE*)pNode)->getData();
      cb += _sntprintf(pBuf + cb, cMaxBuf - cb, "  | line: %-6d | ", traceData->call_line);
      cb += pNode->getTraceText(pBuf + cb, max_trace_text);
    }

#ifdef _USE_RICH_EDIT_FOR_BACK_TRACE
    cb += _sntprintf(pBuf + cb, cMaxBuf - cb, R"({\par)");
#endif
    cb += _sntprintf(pBuf + cb, cMaxBuf - cb, TEXT("\r\n"));
    if (pNode->isFlow())
      pNode = pNode->parent;
    else
      pNode = pNode->getSyncNode();
  }

#ifdef _USE_RICH_EDIT_FOR_BACK_TRACE
  cb += _sntprintf(pBuf + cb, cMaxBuf - cb, "}");
#endif
  pBuf[cb] = 0;
  pBuf[cMaxBuf] = 0;

#ifdef _USE_RICH_EDIT_FOR_BACK_TRACE
  StreamInCallbackCookie cookie = { pBuf, cb, 0 };
  EDITSTREAM es;
  es.dwCookie = (DWORD)(&cookie);
  es.pfnCallback = MyStreamInCallback;
  StreamIn(SF_RTF, es);
#else
  ::SetWindowTextA(m_hWnd, pBuf);
#endif
}

void CBackTraceView::ClearTrace()
{
  ::SetWindowTextA(m_hWnd, "");
}

void CBackTraceView::CopySelection()
{
  Copy();
}
#endif
