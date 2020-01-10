// FlowTraceView.cpp : implementation of the CFlowTraceView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "Settings.h"
#include "Helpers.h"
#include "FlowTraceView.h"

FLOW_NODE*  gSyncronizedNode = NULL;

CFlowTraceView::CFlowTraceView()
	: m_wndListFrame(this)
	, m_wndTreeView(this)
	, m_wndBackTraceView(this)
	, m_wndListView(m_wndListFrame.listView())
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

	dwStyle = WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	if (!gSettings.GetInfoHiden())
		dwStyle |= WS_VISIBLE;
	m_wndBackTraceView.Create(m_wndHorzSplitter, rcDefault, NULL, dwStyle, 0);

	m_wndListFrame.Create(m_wndVertSplitter, rcDefault, NULL,
		WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0);

	m_wndTreeView.Create(m_wndVertSplitter, rcDefault, NULL,
		WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		LVS_REPORT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_OWNERDATA | LVS_NOCOLUMNHEADER,
		LVS_EX_FULLROWSELECT);

	m_wndHorzSplitter.SetSplitterPanes(m_wndVertSplitter, m_wndBackTraceView);
	m_wndHorzSplitter.SetSplitterPosPct(max(10, min(90, gSettings.GetHorzSplitterPos())), false);

	m_wndVertSplitter.SetSplitterPanes(m_wndTreeView, m_wndListFrame);
	m_wndVertSplitter.SetSplitterPosPct(max(10, min(90, gSettings.GetVertSplitterPos())), false);

	m_wndHorzSplitter.m_bFullDrag = false;
	m_wndVertSplitter.m_bFullDrag = false;

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
	m_wndBackTraceView.ApplySettings(fontChanged);
	Gdi::ApplySettings(fontChanged);
}

void CFlowTraceView::ShowBackTrace(LOG_NODE* pSelectedNode)
{
	if (gSettings.GetInfoHiden())
		return;

	if (pSelectedNode == 0)
	{
		return;
	}

	m_wndBackTraceView.UpdateBackTrace(pSelectedNode);
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
    gSyncronizedNode = NULL;
	m_wndTreeView.Clear();
	m_wndBackTraceView.ClearTrace();
	m_wndListView.Clear();
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
			m_wndTreeView.DrawSubItem((int)(pNMLVCD->nmcd.dwItemSpec), pNMLVCD->iSubItem, pNMLVCD->nmcd.hdc, pNMLVCD->nmcd.rc);
			return CDRF_SKIPDEFAULT;
		}
		break;
		}
		return CDRF_DODEFAULT;
	}
	return CDRF_DODEFAULT;
}

LOG_NODE* CFlowTraceView::SyncViews()
{
	HWND hwnd = GetFocus();
	LOG_NODE* pNode = NULL;
	bool fromList = false;
	if (hwnd == m_wndTreeView)
	{
		pNode = m_wndTreeView.GetSelectedNode();
	}
	else if (hwnd == m_wndListView.m_hWnd)
	{
		int iItem = m_wndListView.getSelectionItem();
		fromList = true;
		pNode = gArchive.getListedNodes()->getNode(iItem);
	}
	else if (hwnd == m_wndBackTraceView.m_wndCallFuncView)
	{
		pNode = m_wndBackTraceView.m_wndCallFuncView.GetSelectedNode();
	}
	else if (hwnd == m_wndBackTraceView.m_wndCallStackView)
	{
		pNode = m_wndBackTraceView.m_wndCallStackView.GetSelectedNode();
	}

    if (pNode)
    {
        FLOW_NODE* pFlowNode = pNode->getSyncNode();
        if (pFlowNode)
        {
            gSyncronizedNode = pFlowNode;
            m_wndTreeView.EnsureNodeVisible(pFlowNode, false);
            if (!fromList)
				m_wndListView.ShowFirstSyncronised(true);
            ShowBackTrace(pFlowNode);

			m_wndListView.Invalidate();
            m_wndTreeView.Invalidate();
            m_wndBackTraceView.m_wndCallFuncView.Invalidate();
            m_wndBackTraceView.m_wndCallStackView.Invalidate();
        }
    }
	return pNode;
}

void CFlowTraceView::ShowStackView(bool show)
{
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
