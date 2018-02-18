#include "stdafx.h"
#include "BackTraceView.h"
#include "Settings.h"
#include "Resource.h"
#include "FlowTraceView.h"

CBackTraceView::CBackTraceView(CFlowTraceView* pView)
  : m_wndCallFuncView(pView)
  , m_wndCallStackView(pView)
{

}

LRESULT CBackTraceView::OnCreate(LPCREATESTRUCT lpcs)
{
  DWORD dwStyle;
  ModifyStyle(0, WS_CLIPCHILDREN);
  ModifyStyleEx(WS_EX_CLIENTEDGE, WS_EX_TRANSPARENT);

  m_wndVertSplitter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, SPLIT_PROPORTIONAL);

  dwStyle = WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
    LVS_REPORT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_OWNERDATA | LVS_NOCOLUMNHEADER;

  m_wndCallFuncView.Create(m_wndVertSplitter, rcDefault, NULL, dwStyle, 0);
  m_wndCallStackView.Create(m_wndVertSplitter, rcDefault, NULL, dwStyle, 0);

  m_wndVertSplitter.SetSplitterPanes(m_wndCallStackView, m_wndCallFuncView);
  m_wndVertSplitter.SetSplitterPosPct(max(10, min(90, gSettings.GetStackSplitterPos())), false);

  ApplySettings(true);

  return 0; // windows sets focus to first control
}

void CBackTraceView::ApplySettings(bool fontChanged)
{
  if (fontChanged)
  {
    m_wndCallFuncView.SetFont(gSettings.GetFont());
    m_wndCallStackView.SetFont(gSettings.GetFont());
  }
}

void CBackTraceView::SetChildPos(int cx, int cy)
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
    m_wndVertSplitter.MoveWindow(0, 0, cx, cy, TRUE);
  }
}

LRESULT CBackTraceView::OnPositionChanging(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  WINDOWPOS*	pWinPos = reinterpret_cast<WINDOWPOS*>(lParam);
  if (!(pWinPos->flags & SWP_NOSIZE))
  {
    SetChildPos(pWinPos->cx, pWinPos->cy);
  }
  return 1;
}

LRESULT CBackTraceView::OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
  if (pnmh->hwndFrom == m_wndCallFuncView || pnmh->hwndFrom == m_wndCallStackView)
  {
    CBackTraceCallView * pCBackTraceCallView = (pnmh->hwndFrom == m_wndCallFuncView) ? &m_wndCallFuncView : &m_wndCallStackView;
    LPNMLVCUSTOMDRAW pNMLVCD = (LPNMLVCUSTOMDRAW)pnmh;
    switch (pNMLVCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
      return CDRF_NOTIFYSUBITEMDRAW;          // ask for subitem notifications.
    case CDDS_ITEMPREPAINT:
      pCBackTraceCallView->ItemPrePaint((int)pNMLVCD->nmcd.dwItemSpec, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
      return CDRF_NOTIFYSUBITEMDRAW;
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: // recd when CDRF_NOTIFYSUBITEMDRAW is returned in
    {                                    // response to CDDS_ITEMPREPAINT.
      pCBackTraceCallView->DrawSubItem((int)pNMLVCD->nmcd.dwItemSpec, pNMLVCD->iSubItem, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
      return CDRF_SKIPDEFAULT;
    }
    break;
    }
  }
  return CDRF_DODEFAULT;
}