#include "stdafx.h"
#include "BackTraceView.h"
#include "Settings.h"

CBackTraceView::CBackTraceView(CFlowTraceView* pView)
  : m_pView(pView)
  , m_Initialised(false)
{
}

CBackTraceView::~CBackTraceView()
{
}

#ifdef _USE_LIST_VIEW_FOR_BACK_TRACE

void CBackTraceView::OnSize(UINT nType, CSize size)
{
  if (!m_Initialised)
  {
    m_Initialised = true;
    int cColumns = 0;
    InsertColumn(0, "aaaaaa", LVCFMT_LEFT, cColumns, 50, -1, -1); cColumns++;
    InsertColumn(0, "bbbbbb", LVCFMT_LEFT, cColumns, 50, -1, -1); cColumns++;
    InsertColumn(0, "cccccc", LVCFMT_LEFT, cColumns, 50, -1, -1); cColumns++;
  }
}

void CBackTraceView::UpdateTrace(LOG_NODE* pSelectedNode, bool pendingToResolveAddr)
{
  SetItemCountEx(1, LVSICF_NOSCROLL);
}

void CBackTraceView::ClearTrace()
{
  SetItemCountEx(0, 0);
}

void CBackTraceView::CopySelection()
{
}

void CBackTraceView::DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rcItem)
{
  POINT  textPoint = { 0, rcItem.top };
  if (rcItem.left > 0)
    textPoint.x += rcItem.left;

  TextOut(hdc, textPoint.x, textPoint.y, TEXT("123"), 3);
}

void CBackTraceView::ItemPrePaint(int iItem, HDC hdc, RECT rc)
{
}

#else

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

void CBackTraceView::UpdateTrace(LOG_NODE* pSelectedNode, bool pendingToResolveAddr)
{
  LOG_NODE* pNode;

  int max_fn_name = 0;
  pNode = pSelectedNode->parent ? pSelectedNode : pSelectedNode->getPeer();
  while (pNode && (pNode->isFlow() || pNode->isTrace()))
  {
    max_fn_name = max(max_fn_name, pNode->getFnNameSize());
    pNode = pNode->parent;
  }

  const int cMaxBuf = 1204 * 32;
  static char pBuf[cMaxBuf + 1];
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
            char* src = p_addr_info->src;
            if (!gSettings.GetFullSrcPath())
            {
              char* name = strrchr(src, '/');
              if (name)
                src = name + 1;
            }
            cb += _sntprintf(pBuf + cb, cMaxBuf - cb, "  | from addr: %-8X+%-2X| line: %-6d| src: %s", nearest_pc, addr - nearest_pc, line, src);
          }
          else
          {
            cb += _sntprintf(pBuf + cb, cMaxBuf - cb, "  | from addr: %-8X", addr);
          }
        }
        else if (pendingToResolveAddr)
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
      TRACE_CHANK* chank = traceData->getFirestChank();
      int cb_trace = 0;
      const int max_cb_trace = 150;
      while (chank && chank->len && cb_trace <= max_cb_trace)
      {
        int cb_chank_trace = min(max_cb_trace, chank->len);
        memcpy(pBuf + cb, chank->trace, cb_chank_trace);
        cb += cb_chank_trace;
        cb_trace += cb_chank_trace;
        pBuf[cb] = 0;
        chank = chank->next_chank;
      }
      if (cb_trace >= max_cb_trace)
        cb += _sntprintf(pBuf + cb, cMaxBuf - cb, " ...");
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
