#include "stdafx.h"
#include "LogListFrame.h"
#include "FlowTraceView.h"

CLogListFrame::CLogListFrame(CFlowTraceView* pView) :
	m_pParent(pView)
	, m_wndListView(this, pView, &m_wndListInfo)
	, m_wndListInfo(this, &m_wndListView)
{
}


LRESULT CLogListFrame::OnCreate(LPCREATESTRUCT lpcs)
{
	ModifyStyle(0, WS_CLIPCHILDREN);
	ModifyStyleEx(WS_EX_CLIENTEDGE, WS_EX_TRANSPARENT);

	m_wndListView.Create(m_hWnd, rcDefault, NULL,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
		LVS_REPORT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_OWNERDATA | LVS_NOCOLUMNHEADER,
		0);

	m_wndListInfo.Create(m_hWnd, rcDefault, NULL,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		0);

	return 0; // windows sets focus to first control
}

LRESULT CLogListFrame::OnPositionChanged(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	SetChiledPos();
	return 1;
}

void CLogListFrame::SetChiledPos()
{
	WINDOWPLACEMENT wpl = { sizeof(WINDOWPLACEMENT) };
	if (::GetWindowPlacement(m_hWnd, &wpl)) {
		int cxInfo, cx, cy;
		cxInfo = m_wndListInfo.GetWidth();
		cx = wpl.rcNormalPosition.right - wpl.rcNormalPosition.left;
		cy = wpl.rcNormalPosition.bottom - wpl.rcNormalPosition.top;
		m_wndListInfo.SetWindowPos(NULL, 0, 0, cxInfo, cy, SWP_NOZORDER);
		m_wndListView.SetWindowPos(NULL, cxInfo, 0, (cx > cxInfo) ? (cx - cxInfo) : 0, cy, SWP_NOZORDER);
	}
}

LRESULT CLogListFrame::OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/)
{
	return 1; // handled
}

LRESULT CLogListFrame::OnLvnEndScroll(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	m_wndListView.UpdateCaret();
	return 0;
}

LRESULT CLogListFrame::OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	if (pnmh->hwndFrom == m_wndListView)
	{
		LPNMLVCUSTOMDRAW pNMLVCD = (LPNMLVCUSTOMDRAW)pnmh;
		switch (pNMLVCD->nmcd.dwDrawStage)
		{
		case CDDS_PREPAINT:
			return CDRF_NOTIFYSUBITEMDRAW;          // ask for subitem notifications.
		case CDDS_ITEMPREPAINT:
			m_wndListView.ItemPrePaint((int)(pNMLVCD->nmcd.dwItemSpec), pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
			return CDRF_NOTIFYSUBITEMDRAW;
		case CDDS_ITEMPREPAINT | CDDS_SUBITEM: // recd when CDRF_NOTIFYSUBITEMDRAW is returned in
		{                                    // response to CDDS_ITEMPREPAINT.
			m_wndListView.DrawSubItem((int)(pNMLVCD->nmcd.dwItemSpec), pNMLVCD->iSubItem, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
			return CDRF_SKIPDEFAULT;
		}
		break;
		}
		return CDRF_DODEFAULT;
	}
	return CDRF_DODEFAULT;
}