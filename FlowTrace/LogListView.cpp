#include "stdafx.h"
#include "Resource.h"
#include "LogListView.h"
#include "LogListFrame.h"
#include "Settings.h"
#include "SearchInfo.h"
#include "MainFrm.h"

extern HWND       hwndMain;

CLogListView::CLogListView(CLogListFrame *pPareent, CFlowTraceView* pView, CLogListInfo* pListInfo)
	: m_pPareent(pPareent)
	, m_pFlowTraceView(pView)
	, m_pListInfo(pListInfo)
	, m_hasCaret(false)
	, m_IsCupture(false)
	, m_cActualColumns(0)
	, m_lengthCalculated(1)
	, m_recCount(0)
	, m_curBookmark(0)
	, m_ColLen(0)

{
	ClearColumnInfo();
}

CLogListView::~CLogListView()
{
}

void CLogListView::ClearColumnInfo()
{
	ZeroMemory(m_DetailType, sizeof(m_DetailType));
	m_ColLen = 0;
	m_lengthCalculated++;
}

void LOG_SELECTION::print()
{
	//stdlog("start %d %d end %d %d cur %d %d \n", start.iItem, start.iChar, end.iItem, end.iChar, cur.iItem, cur.iChar);
}

bool LOG_SELECTION::InSelection(int iItem, int iChar) {
	SEL_POINT sel(iItem, iChar);
	return (sel >= start) && (sel <= end);
}

bool LOG_SELECTION::Move(int iItem, int iChar, bool extend)
{
	LOG_SELECTION old = *this;
	SEL_POINT sel(iItem, iChar);
	if (extend)
	{
		if (start > sel)
		{
			start = sel;
		}
		else if (end < sel)
		{
			end = sel;
		}
		else if ((start < sel) && (end > sel))
		{
			if (cur == start)
			{
				start = sel;
			}
			else if (cur == end)
			{
				end = sel;
			}
			else
			{
				ATLASSERT(FALSE);
				start = end = cur = sel;
			}

		}
		cur = sel;
		if (start > end)
		{
			ATLASSERT(FALSE);
			start = end = cur = sel;
		}
	}
	else
	{
		start = end = cur = sel;
	}
	//stdlog("cur.iChar %d\n", cur.iChar);
	return !(old == *this);

}

void CLogListView::SelectAll()
{
	MoveSelectionEx(0, 0, false, false);
	MoveSelectionToEnd(true);
}

bool CLogListView::HasSelection()
{
	return !m_ListSelection.IsEmpty();
}

LOG_NODE* CLogListView::GetSynkNode()
{
	LOG_NODE* pNode = gArchive.getListedNodes()->getNode(m_ListSelection.CurItem());
	if (pNode)
		pNode = pNode->getSyncNode();
	return pNode;
}

static void inline CopyLine(char*& lock, int& len, char* buf, int l, int i, bool copySpecial, bool copy) {
	if (copySpecial) 
	{
		CHAR szNN[32];
		int cbNN = _stprintf_s(szNN, _countof(szNN), _T("%d: "), i + 1);
		if (copy)
			memcpy(lock + len, szNN, cbNN);
		len += cbNN;
	}
	if (copy)
		memcpy(lock + len, buf, l);
	len += l;
}

void CLogListView::CopySelection(bool onlyTraces, bool copySpecial)
{
	if (m_ListSelection.IsEmpty() && !onlyTraces)
		return;

	HGLOBAL clipdata = 0;
	char *lock = 0;

	int len = 0, l = 0;
	for (int j = 0; j < 2; j++)
	{
		bool copy = (j == 1);
		if (copy)
		{
			if (len)
			{
				clipdata = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, len * sizeof(char));
				if (!clipdata)
					return;
				if (!(lock = (char*)GlobalLock(clipdata))) {
					GlobalFree(clipdata);
					return;
				}
			}
			else
				return;
		}
		len = 0;
		l = 0;
		for (int i = m_ListSelection.StartItem(); i <= m_ListSelection.EndItem(); i++)
		{
			int logLen;
			char* log = getText(i, &logLen, onlyTraces);
			if (logLen)
			{
				if (onlyTraces)
				{
					l = logLen;
					if (copy) 
						memcpy(lock + len, log, l);
					len += l;
				}
				else if (i == m_ListSelection.StartItem() && i == m_ListSelection.EndItem())
				{
					l = 0;
					if (m_ListSelection.StartChar() < logLen)
					{
						l = min(m_ListSelection.EndChar(), logLen) - m_ListSelection.StartChar();
					}
					if (l)
					{
						CopyLine(lock, len, log + m_ListSelection.StartChar(), l, i, copySpecial, copy);
					}
				}
				else if (i == m_ListSelection.StartItem())
				{
					l = 0;
					if (m_ListSelection.StartChar() < logLen)
					{
						l = logLen - m_ListSelection.StartChar();
					}
					if (l)
					{
						CopyLine(lock, len, log + m_ListSelection.StartChar(), l, i, copySpecial, copy);
					}
				}
				else if (i == m_ListSelection.EndItem())
				{
					l = min(m_ListSelection.EndChar(), logLen);
					if (l)
					{
						CopyLine(lock, len, log, l, i, copySpecial, copy);
					}
				}
				else //(i > m_ListSelection.StartItem() && i < m_ListSelection.EndItem())
				{
					l = logLen;
					if (l)
					{
						CopyLine(lock, len, log, l, i, copySpecial, copy);
					}
				}
				if (l)
				{
					if (i != m_ListSelection.EndItem())
					{
						if (copySpecial) {
							if (copy)
								memcpy(lock + len, "\r\n", 2);
							len += 2;
						}
						if (copy)
							memcpy(lock + len, "\r\n", 2);
						len += 2;
					}
					else
					{
						if (copy)
							memcpy(lock + len, "", 1);
						len += 1;
					}
				}

			}
		}
	}

	if (len)
	{
		GlobalUnlock(clipdata);
		if (clipdata)
		{
			if (::OpenClipboard(m_hWnd)) {
				EmptyClipboard();
				SetClipboardData(CF_TEXT, clipdata);
				CloseClipboard();
			}
			else {
				GlobalFree(clipdata);
			}
		}
	}
	else
	{
		GlobalFree(clipdata);
	}

}

void CLogListView::Redraw(int nFirst, int nLast)
{
	if (nFirst < 0)
		nFirst = 0;
	if (nLast < 0)
		nLast = m_recCount;

	RedrawItems(nFirst, nLast);
	//UpdateWindow();
}

void CLogListView::MoveSelectionEx(int iItem, int iChar, bool extend, bool ensureVisible)
{
	int oldItem = m_ListSelection.CurItem();
	bool movToEnd = iItem >= (int)m_recCount;
	//stdlog("iItem  %d %d\n", iItem, iChar);
	iItem = max(0, min(iItem, (int)m_recCount - 1));

	int logLen;
	char* log = getText(iItem, &logLen);
	if (!logLen) { return; }

	if (iChar < 0 || movToEnd)
		iChar = logLen;

	iChar = max(0, min(iChar, logLen));
	//bool prevEmpty = m_ListSelection.IsEmpty();
	int preStart = m_ListSelection.StartItem();
	int prevEnd = m_ListSelection.EndItem();

	//stdlog("iItem1 %d %d\n", iItem, iChar);
	if (m_ListSelection.Move(iItem, iChar, extend))
	{
		//bool curEmpty = m_ListSelection.IsEmpty();
		int curStart = m_ListSelection.StartItem();
		int curEnd = m_ListSelection.EndItem();
		//if (!prevEmpty || !curEmpty) 
		{
			Redraw(min(preStart, curStart), max(prevEnd, curEnd));
		}
	}

	if (ensureVisible)
		EnsureTextVisible(m_ListSelection.CurItem(), m_ListSelection.CurChar(), m_ListSelection.CurChar());

	UpdateCaret();

	Helpers::UpdateStatusBar();
}

void CLogListView::MoveSelectionToEnd(bool extend)
{
	if (m_recCount)
	{
		int lastItem = (int)m_recCount - 1;
		int cbLastText;
		getText(lastItem, &cbLastText);
		MoveSelectionEx(lastItem, cbLastText, extend);
	}
}

void CLogListView::EnsureTextVisible(int iItem, int startChar, int endChar)
{
	iItem = max(0, min(iItem, (int)m_recCount - 1));

	int logLen;
	char* log = getText(iItem, &logLen);
	if (!logLen) return;
	startChar = max(0, min(startChar, logLen));
	endChar = max(0, min(endChar, logLen));

	ShowItem(iItem, false, false);

	SIZE size1, size2;
	GetTextExtentPoint32(wndMemDC, log, startChar, &size1);
	GetTextExtentPoint32(wndMemDC, log, endChar, &size2);

	RECT rcClient, rcItem;
	GetClientRect(&rcClient);
	ListView_GetSubItemRect(m_hWnd, iItem, 0, LVIR_BOUNDS, &rcItem);

	SCROLLINFO si;
	si.fMask = SIF_RANGE | SIF_POS;
	GetScrollInfo(SB_HORZ, &si);
	SIZE size = { 0, 0 };
	if (size1.cx < si.nPos)
	{
		size.cx = size1.cx - si.nPos - TEXT_MARGIN;
		Scroll(size);
	}
	if (size2.cx >= rcClient.right - rcItem.left - TEXT_MARGIN)
	{
		size.cx = size2.cx - rcClient.right + rcItem.left + TEXT_MARGIN + 10;
		Scroll(size);
	}
}

void CLogListView::ShowFirstSyncronised(bool scrollToMiddle)
{
	for (int i = 0; i < m_recCount; i++)
	{
		if (gArchive.getListedNodes()->getNode(i)->isSynchronized(gSyncronizedNode))
		{
			ShowItem(i, scrollToMiddle, true);
			break;
		}
	}

}

void CLogListView::ShowNode(LOG_NODE* pNode, bool scrollToMiddle)
{
	for (int i = 0; i < m_recCount; i++)
	{
		if (pNode == gArchive.getListedNodes()->getNode(i))
		{
			ShowItem(i, scrollToMiddle, true);
			break;
		}
	}
}

void CLogListView::ShowItem(DWORD i, bool scrollToMiddle, bool scrollToLeft)
{
	EnsureVisible(i, FALSE);
	if (scrollToMiddle || scrollToLeft)
	{
		SIZE size = { 0, 0 };
		RECT rect = { 0 };
		GetClientRect(&rect);
		size.cy = rect.bottom;
		POINT pt;
		GetItemPosition(i, &pt);
		if (scrollToMiddle)
		{
			size.cy = pt.y - rect.bottom / 2;
		}
		if (scrollToLeft)
		{
			size.cx = pt.x;
		}
		Scroll(size);
	}
}

void CLogListView::UpdateCaret()
{
	//m_Selection.print();
	//if (ListView_IsItemVisible(m_hWnd, m_Selection.cur.iItem))
	if (!m_hasCaret)
		return;

	int x = TEXT_MARGIN;
	int y = 0;

	if (m_recCount)
	{
		RECT itemRect;
		int curItem = m_ListSelection.CurItem();
		if (GetSubItemRect(curItem, 0, LVIR_LABEL, &itemRect))
		{
			RECT viewRect;
			GetViewRect(&viewRect);

			int cbText;
			char* szText = getText(curItem, &cbText);
			y = itemRect.top + (itemRect.bottom - itemRect.top) / 2 - gSettings.GetFontHeight() / 2;
			int cb = m_ListSelection.CurChar();
			if (cb > cbText)
				cb = cbText;
			SIZE  size;
			GetTextExtentPoint32(wndMemDC, szText, cb, &size);
			x = itemRect.left + size.cx + TEXT_MARGIN;
			//STDLOG("cb = %d x = %d y = %d iItem = %d itemRect l = %d r = %d t = %d b = %d\n", cb, x, y, curItem, itemRect.left, itemRect.right, itemRect.top, itemRect.bottom);

			//udjust searchInfo.curLine

		}
	}

	SetCaretPos(x, y);
}

LRESULT CLogListView::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	bHandled = TRUE;
	bool bShiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	bool bCtrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

	if (!m_recCount)
		return 0;
	int cbCurText;
	char* szText = getText(m_ListSelection.CurItem(), &cbCurText);
	if (!cbCurText) return 0;

	int curItem = m_ListSelection.CurItem();
	int curChar = m_ListSelection.CurChar();
	int startItem = m_ListSelection.StartItem();
	int startChar = m_ListSelection.StartChar();
	int endItem = m_ListSelection.EndItem();
	int endChar = m_ListSelection.EndChar();

	switch (wParam)
	{
	case VK_HOME:       // Home 
	{
		if (bCtrlPressed)
			MoveSelectionEx(0, 0, bShiftPressed);
		else
			MoveSelectionEx(startItem, 0, bShiftPressed);
	}
	break;
	case VK_RETURN:        // End 
	{
		MoveSelectionEx(m_recCount - 1, 0, false);
	}
	break;
	case VK_END:        // End 
	{
		if (bCtrlPressed)
			MoveSelectionToEnd(bShiftPressed);
		else
			MoveSelectionEx(endItem, cbCurText, bShiftPressed);
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
			MoveSelectionEx(startItem - count, startChar, bShiftPressed);
		else
			MoveSelectionEx(endItem + count, endChar, bShiftPressed);
	}
	break;
	case VK_LEFT:       // Left arrow 
	{
		if (startChar)
		{
			if (bCtrlPressed)
			{
				int prevChar = startChar;
				while (prevChar > 0 && IsWordChar(szText[prevChar - 1]))
					prevChar--;
				if (prevChar == startChar)
					prevChar--;
				MoveSelectionEx(startItem, prevChar, bShiftPressed);
			}
			else
			{
				if (bShiftPressed)
					MoveSelectionEx(curItem, curChar - 1, bShiftPressed);
				else if (m_ListSelection.IsEmpty())
					MoveSelectionEx(startItem, startChar - 1, bShiftPressed);
				else
					MoveSelectionEx(startItem, startChar, false);
			}
		}
		else
		{
			MoveSelectionEx(startItem - 1, -1, bShiftPressed);
		}
	}
	break;
	case VK_RIGHT:      // Right arrow 
	{
		if (endChar < cbCurText - 1)
		{
			if (bCtrlPressed)
			{
				int nextChar = endChar;
				while (nextChar < cbCurText && IsWordChar(szText[nextChar]))
					nextChar++;
				if (nextChar == endChar)
					nextChar++;
				MoveSelectionEx(endItem, nextChar, bShiftPressed);
			}
			else
			{
				if (bShiftPressed)
					MoveSelectionEx(curItem, curChar + 1, bShiftPressed);
				else if (m_ListSelection.IsEmpty())
					MoveSelectionEx(endItem, endChar + 1, bShiftPressed);
				else
					MoveSelectionEx(endItem, endChar, false);
			}
		}
		else
		{
			MoveSelectionEx(endItem + 1, 0, bShiftPressed);
		}
	}
	break;
	case VK_UP:         // Up arrow 
	{
		MoveSelectionEx(startItem - 1, startChar, bShiftPressed);
	}
	break;
	case VK_DOWN:       // Down arrow 
	{
		MoveSelectionEx(endItem + 1, endChar, bShiftPressed);
	}
	break;
	}
	return 0;
}

LRESULT CLogListView::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	//UINT nChar = (CHAR)wParam;
	bHandled = TRUE;
	return 0;
}

LRESULT CLogListView::OnLButtonDblClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL & bHandled)
{
	if (!m_recCount)
		return 0;

	int xPos = GET_X_LPARAM(lParam);
	if (xPos < TEXT_MARGIN)
		xPos = TEXT_MARGIN;


	int cbCurText;
	char* szText = getText(m_ListSelection.CurItem(), &cbCurText);
	if (!cbCurText)
		return 0;

	int curChar = m_ListSelection.CurChar();

	int prevChar = curChar;
	while (prevChar > 0 && IsWordChar(szText[prevChar - 1]))
		prevChar--;
	//if (prevChar == curChar)
	//  prevChar--;

	MoveSelectionEx(m_ListSelection.CurItem(), prevChar, false);

	int nextChar = curChar;
	while (nextChar < cbCurText && IsWordChar(szText[nextChar]))
		nextChar++;
	if (nextChar == curChar)
		nextChar++;
	MoveSelectionEx(m_ListSelection.CurItem(), nextChar, true);

	bHandled = TRUE;
	return 0;
}

LRESULT CLogListView::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	UdjustSelectionOnMouseEven(uMsg, wParam, lParam);

	bHandled = TRUE;

	int xPos = GET_X_LPARAM(lParam);
	int yPos = GET_Y_LPARAM(lParam);

	int iItem = getSelectionItem();
	LOG_NODE* pNode = gArchive.getListedNodes()->getNode(iItem);

    bool disable;
    int cMenu = 0;
	POINT pt = { xPos, yPos };
	ClientToScreen(&pt);
	HMENU hMenu = CreatePopupMenu();
	Helpers::AddCommonMenu(pNode, hMenu, cMenu);

	disable = (m_ListSelection.IsEmpty());
	Helpers::AddMenu(hMenu, cMenu, ID_EDIT_COPY, _T("&Copy\tCtrl+C"), disable);
	Helpers::AddMenu(hMenu, cMenu, ID_EDIT_COPY_TRACES, _T("Copy Trace"));
	Helpers::AddMenu(hMenu, cMenu, ID_EDIT_COPY_SPECIAL, _T("Copy Special"), disable);

	InsertMenu(hMenu, cMenu++, MF_BYPOSITION | MF_SEPARATOR, 0, _T(""));
	Helpers::AddMenu(hMenu, cMenu, ID_TREE_SHOW_THIS_THREAD, _T("Show only this thread\tCtrl+D"), false);
	Helpers::AddMenu(hMenu, cMenu, ID_TREE_SHOW_THIS_APP, _T("Show only this app\tCtrl+P"), false);
	Helpers::AddMenu(hMenu, cMenu, ID_TREE_SHOW_ALL, _T("Show All\tCtrl+L"), false);

    UINT nRet = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, 0);
	DestroyMenu(hMenu);

	if (nRet == ID_EDIT_COPY)
	{
		CopySelection();
	}
	else if (nRet == ID_EDIT_COPY_TRACES)
	{
		CopySelection(true);
	}
	else if (nRet == ID_EDIT_COPY_SPECIAL)
	{
		CopySelection(false, true);
	}	
	else if (nRet == ID_SYNC_VIEWES)
	{
		m_pFlowTraceView->SyncViews();
	}
	else if (nRet == ID_SHOW_CALL_IN_FUNCTION)
	{
		Helpers::ShowInIDE(pNode, true);
	}
	else if (nRet == ID_SHOW_FUNCTION)
	{
		Helpers::ShowInIDE(pNode, false);
	}
    else if (nRet == ID_VIEW_NODE_DATA)
	{
		Helpers::ShowNodeDetails(pNode);
	}
	else if (nRet == ID_TREE_SHOW_THIS_THREAD)
	{
		::SendMessage(hwndMain, WM_COMMAND, nRet, 0);
	}
	else if (nRet == ID_TREE_SHOW_THIS_APP)
	{
		::SendMessage(hwndMain, WM_COMMAND, nRet, 0);
	}
	else if (nRet == ID_TREE_SHOW_ALL)
	{
		::SendMessage(hwndMain, WM_COMMAND, nRet, 0);
	}

	return 0;
}

LRESULT CLogListView::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & bHandled)
{
	//STDLOG("Focus = %d", m_hWnd == ::GetFocus());
	if (!m_hasCaret)
	{
		CreateSolidCaret(1, gSettings.GetFontHeight());
		m_hasCaret = true;
		ShowCaret();
	}
	UpdateCaret();
	bHandled = FALSE;
	return 0;
}

LRESULT CLogListView::OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & bHandled)
{
	//STDLOG("Focus = %d", m_hWnd == ::GetFocus());
	if (m_hasCaret)
		DestroyCaret();
	m_hasCaret = false;
	bHandled = FALSE;
	//stdlog("m_ListSelection.StartItem() %d\n", m_ListSelection.StartItem());
	Redraw(m_ListSelection.StartItem(), m_ListSelection.StartItem());
	return 0;
}

LRESULT CLogListView::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (m_IsCupture)
	{
		UdjustSelectionOnMouseEven(uMsg, wParam, lParam);
	}
	return 0;
}

LRESULT CLogListView::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (m_IsCupture)
	{
		m_IsCupture = false;
		ReleaseCapture();
	}
	return 0;
}

LRESULT CLogListView::OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    return OnLButtonDown(uMsg, wParam, lParam, bHandled);
}

LRESULT CLogListView::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	UdjustSelectionOnMouseEven(uMsg, wParam, lParam);
    LOG_NODE* pNode = gArchive.getListedNodes()->getNode(getSelectionItem());
    Helpers::OnLButtonDoun(pNode, wParam, lParam);
	return 0;
}

void CLogListView::ToggleBookmark(DWORD item)
{
	LOG_NODE* pNode = gArchive.getListedNodes()->getNode(item);
	if (pNode)
	{
		pNode->bookmark = pNode->bookmark ? 0 : gArchive.getNewBookmarkNumber();
		Redraw(item, item);
		if (pNode->bookmark)
			m_curBookmark = item;
	}
}

void CLogListView::OnBookmarks(WORD wID)
{
	switch (wID)
	{
	case ID_BOOKMARKS_NEXTBOOKMARK:
	case ID_BOOKMARKS_PREVIOUSBOOKMARK:
	{
		int bookmark = -1;
		int count = (int)(gArchive.getListedNodes()->Count());
		if (m_curBookmark > count)
			m_curBookmark = count;
		if (ID_BOOKMARKS_NEXTBOOKMARK == wID)
		{
			for (int i = m_curBookmark + 1; i < count; i++)
			{
				if (gArchive.getListedNodes()->getNode(i)->bookmark)
				{
					bookmark = i;
					break;
				}
			}
			if (bookmark == -1)
			{
				for (int i = 0; i < count; i++)
				{
					if (gArchive.getListedNodes()->getNode(i)->bookmark)
					{
						bookmark = i;
						break;
					}
				}
			}
		}
		else
		{
			for (int i = m_curBookmark - 1; i >= 0; i--)
			{
				if (gArchive.getListedNodes()->getNode(i)->bookmark)
				{
					bookmark = i;
					break;
				}
			}
			if (bookmark == -1)
			{
				for (int i = count - 1; i >= 0; i--)
				{
					if (gArchive.getListedNodes()->getNode(i)->bookmark)
					{
						bookmark = i;
						break;
					}
				}
			}
		}
		if (bookmark != -1)
		{
			m_curBookmark = bookmark;
			MoveSelectionEx(bookmark, 0, false, false);
			ShowItem(bookmark, true, true);
		}
	}
	break;
	case ID_BOOKMARKS_CLEARBOOKMARKS:
	{
		for (DWORD i = 0; i < gArchive.getListedNodes()->Count(); i++)
		{
			gArchive.getListedNodes()->getNode(i)->bookmark = 0;
		}
		gArchive.resteBookmarkNumber();
		Redraw(0, gArchive.getListedNodes()->Count());
	}
	break;
	case ID_BOOKMARKS_TOGGLE:
	{
		ToggleBookmark(m_ListSelection.CurItem());
	}
	break;
	}
}

bool CLogListView::UdjustSelectionOnMouseEven(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//STDLOG("Focus = %d", m_hWnd == ::GetFocus());

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN)
	{
		if (GetFocus() != *this)
			SetFocus();
	}

	int xPos = GET_X_LPARAM(lParam);
	int yPos = GET_Y_LPARAM(lParam);

	LVHITTESTINFO ht = { 0 };
	ht.pt.x = xPos;
	ht.pt.y = yPos;
	ht.iItem = -1;
	SubItemHitTest(&ht);
	//stdlog("xPos = %d yPos = %d flags = %d ht.iItem = %d\n", xPos, yPos, ht.flags, ht.iItem);
	if ((ht.iItem >= 0)) //&& (LVHT_ONITEM & ht.flags)
	{
		CRect itemRect;
		GetSubItemRect(ht.iItem, 0, LVIR_LABEL, &itemRect);
		//stdlog("xPos = %d itemRect l = %d r = %d t = %d b = %d\n", xPos, itemRect.left, itemRect.right, itemRect.top, itemRect.bottom);
		if (xPos < itemRect.left)
		{
			xPos = itemRect.left;
		}
		int logLen;
		char* log = getText(ht.iItem, &logLen);
		int nFit = 0;
		if (logLen)
		{
			int nMaxExtent = xPos - itemRect.left - TEXT_MARGIN;
			int cb = logLen;
			SIZE  size;
			GetTextExtentExPoint(wndMemDC, log, cb, nMaxExtent, &nFit, NULL, &size);
			if (nFit < logLen)
			{
				SIZE  size1, size2;
				GetTextExtentPoint32(wndMemDC, log, nFit, &size1);
				GetTextExtentPoint32(wndMemDC, log, nFit + 1, &size2);
				if (nMaxExtent > (size2.cx + size1.cx) / 2)
					nFit++;
			}
			//stdlog("iChar = %d iCharMax = %d nFit = %d\n", iChar, iCharMax, nFit);
		}

		if (uMsg == WM_RBUTTONDOWN)
		{
			if (m_ListSelection.InSelection(ht.iItem, nFit))
				return false;
		}

		MoveSelectionEx(ht.iItem, nFit, (uMsg == WM_MOUSEMOVE) || (0 != (wParam & MK_SHIFT)));
		if (!m_IsCupture && (wParam & MK_LBUTTON))
		{
			m_IsCupture = true;
			SetCapture();
		}
		return true;
	}
	return false;
}

void CLogListView::ApplySettings(bool fontChanged)
{
	m_pListInfo->ApplySettings(fontChanged);
	if (fontChanged)
	{
		SetFont(gSettings.GetFont());
		SelectObject(wndMemDC, gSettings.GetFont());
		SelectObject(itemMemDC, gSettings.GetFont());
	}
	SetBkColor(gSettings.LogListBkColor());
	wndMemDC.SetBkColor(gSettings.LogListBkColor());
	itemMemDC.SetBkColor(gSettings.LogListBkColor());
	SetColumns();
}

void CLogListView::Clear()
{
	if (m_recCount)
	{
		m_pListInfo->SetRecCount(0);
		SetItemCountEx(0, 0);
		DeleteAllItems();
		m_recCount = 0;
		m_ListSelection.Clear();
		::SetFocus(hwndMain); //force carret destroy
		SetColumns();
		Helpers::UpdateStatusBar();
	}
}

bool CLogListView::SetColumnLen(int len)
{
	RECT rc;
	GetClientRect(&rc);
	len = max(rc.right + 16, (long)len);
	if (m_ColLen < len && len < 30000)
	{
		//stdlog("len = %d m_ColLen = %d\n", len, m_ColLen);
		m_ColLen = len;
		SetColumnWidth(0, m_ColLen);
		return true;
	}
	return false;
}

void CLogListView::OnSize(UINT nType, CSize size)
{
	if (size.cx > 0 && size.cy > 0)
	{
		wndMemDC.CreateCompatibleBitmap(NULL, size.cx, size.cy);
		SetColumnLen(0);
	}
}

void CLogListView::RefreshList(bool redraw)
{
	int newCount = (int)gArchive.getListedNodes()->Count();
	if (newCount > 0)
	{
		if (newCount == m_recCount && (!redraw))
			return;
		if (newCount < m_recCount || redraw)
			Clear();
		int oldRecCount = m_recCount;
		m_recCount = newCount;
		m_pListInfo->SetRecCount(newCount);
		SetItemCountEx(m_recCount, LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
		Update(newCount - 1);
		int curItem = m_ListSelection.CurItem();
		//stdlog("~~~%d %d\n", curItem, (int)m_recCount - 1);
		if (redraw || oldRecCount == 0 || curItem >= ((int)oldRecCount - 1))
			MoveSelectionEx(m_recCount - 1, 0, false, true);
	}
	else
	{
		Clear();
	}

	Helpers::UpdateStatusBar();
}

void CLogListView::SetColumns()
{
	DeleteColumn(0);
	InsertColumn(0, "", LVCFMT_LEFT);

	ClearColumnInfo();

	m_cActualColumns = 0;

	if (gSettings.GetColNN())
		m_DetailType[m_cActualColumns++] = NN_COL;
	if (gSettings.GetColApp())
		m_DetailType[m_cActualColumns++] = APP_COLL;
	if (gSettings.GetColPID() || gSettings.GetColTID() || gSettings.GetColThreadNN())
		m_DetailType[m_cActualColumns++] = THREAD_COL;
	if (gSettings.GetColTime())
		m_DetailType[m_cActualColumns++] = TIME_COL;
	if (gSettings.GetColFunc())
		m_DetailType[m_cActualColumns++] = FUNC_COL;

	m_DetailType[m_cActualColumns++] = LOG_COL;
	//TODO m_pListInfo
}

LRESULT CLogListView::OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & bHandled)
{
	//bHandled = false;  return 0; 
	RECT rcItem = {0,0,0,0}, rcEraze;
	GetClientRect(&rcEraze);

	int nLast = GetTopIndex() + GetCountPerPage() - 1;
	if (m_recCount && nLast > 0)
	{
		if (nLast >= m_recCount)
			nLast = m_recCount - 1;
		GetItemRect(nLast, &rcItem, LVIR_BOUNDS);
		if (rcItem.bottom <= rcEraze.bottom)
			rcEraze.top = rcItem.bottom;
	}
	::FillRect((HDC)wParam, &rcEraze, wndMemDC.m_bkBrush);
	m_pListInfo->Invalidate();
	//stdlog(" nLast=%d l=%d r=%d t=%d b=%d\n", nLast, rcEraze.left, rcEraze.right, rcEraze.top, rcEraze.bottom);
	return 1; // handled
}

void CLogListView::ItemPrePaint(int iItem, HDC hdc, RECT rcItem)
{
	LOG_NODE* pNode = gArchive.getListedNodes()->getNode(iItem);
	if (!pNode)
	{
		//ATLASSERT(0);
		return;
	}

	if (pNode->lengthCalculated != m_lengthCalculated)
	{
		pNode->lengthCalculated = m_lengthCalculated;
		//calculate lengths of all columns
		SIZE size;
		//for (int i = 1; i < m_cColumns; i++)
		{
			size.cx = 0;
			int cbText = 0;
			char* szText = getText(iItem, &cbText);
			if (cbText)
			{
				GetTextExtentPoint32(hdc, szText, cbText, &size);
				if (SetColumnLen(size.cx + 3 * TEXT_MARGIN))
				{
					//stdlog("\t iItem i %d size.cx %d %s\n", iItem, size.cx, szText);
				}
			}
		}
	}
}

void CLogListView::DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rcItem)
{
	LOG_NODE* pNode = gArchive.getListedNodes()->getNode(iItem);
	if (!pNode)
	{
		//ATLASSERT(0);
		return;
	}

	RECT rcClient;
	GetClientRect(&rcClient);
	int cx = rcClient.right - rcClient.left;
	int cy = rcItem.bottom - rcItem.top;
	if (!(cx && cy))
		return;

	itemMemDC.CreateCompatibleBitmap(hdc, cx, cy);
	itemMemDC.EraseBackground();
	itemMemDC.SetBkMode(OPAQUE);


	int cbText, cbInfo;
	char* szText = getText(iItem, &cbText, false, &cbInfo);

	int  textPoint = rcItem.left + TEXT_MARGIN;
	int startChar = 0;

	//if (rcItem.left < 0)
	//{
	//	int x = -rcItem.left;
	//	// startChar is number of characters which fit in x
	//	SIZE  textSizeUnused;
	//	GetTextExtentExPoint(itemMemDC, szText, cbText, x, &startChar, NULL, &textSizeUnused);
	//	if (startChar > 0)
	//	{
	//		SIZE  size;
	//		GetTextExtentPoint32(itemMemDC, szText, startChar, &size);
	//		if (size.cx < x)
	//			textPoint -= (x - size.cx);
	//	}
	//	else
	//	{
	//		textPoint -= x;
	//	}
	//}

	//stdlog("iItem %d textPoint %d startChar %d left %d\n", iItem, textPoint, startChar, rcItem.left);

	DWORD textColor, bkColor, selBkColor, selColor, serachColor, curSerachColor, infoBkColor, infoTextColor, curBkColor;
	selColor = gSettings.SelectionTxtColor();
	selBkColor = gSettings.SelectionBkColor();
	DWORD textColor_0 = gSettings.LogListTxtColor();
	DWORD bkColor_0 = gSettings.LogListBkColor();
	textColor = textColor_0;
	bkColor = bkColor_0;
	serachColor = bkColor_0;
	curSerachColor = gSettings.CurSerachColor();
	infoTextColor = gSettings.InfoTextColor();
	infoBkColor = gSettings.LogListBkColor();

	{
		textColor = textColor_0;
		bkColor = bkColor_0;
		if (pNode->isTrace())
		{
			if (((TRACE_NODE*)pNode)->color)
				gSettings.SetTraceColor(((TRACE_NODE*)pNode)->color, textColor, bkColor);
		}

#define bSearched  1
#define bSelected  2
#define bcurSearch 4
#define bNotTrace 8
		int curChar = startChar;
		int prevChar = startChar;
		BYTE oldFlag = 0;
		char* curSearch = szText;
		char* nextSearch = curSearch;
		int searchPos = 0;
		while (curChar < cbText)
		{
			BYTE curFlag = 0;
			if (curSearch && pNode->isInfo())
			{
				char* p = szText;
				if (pNode->lineSearchPos && searchInfo.total)
				{
					if (curChar == startChar)
					{
						curSearch = searchInfo.find(nextSearch);
						nextSearch = curSearch ? curSearch + searchInfo.cbText : NULL;
					}
					if (curSearch)
					{
						while (curSearch && (p + curChar) > (curSearch + searchInfo.cbText))
						{
							curSearch = searchInfo.find(nextSearch);
							nextSearch = curSearch ? curSearch + searchInfo.cbText : NULL;
							searchPos++;
						}
						if (curSearch && curSearch <= (p + curChar) && (curSearch + searchInfo.cbText) > (p + curChar))
						{
							curFlag |= bSearched;
							if (iItem == searchInfo.curLine && searchPos == searchInfo.posInCur)
								curFlag |= bcurSearch;
						}
					}
				}
			}

			if ((iItem >= m_ListSelection.StartItem() && iItem <= m_ListSelection.EndItem()) && !m_ListSelection.IsEmpty())
			{
				if (iItem > m_ListSelection.StartItem() && iItem < m_ListSelection.EndItem())
				{
					//if (iItem == 3)stdlog("0 %d\n", iItem);
					curFlag |= bSelected;
				}
				else
				{
					if (iItem == m_ListSelection.StartItem() && iItem == m_ListSelection.EndItem())
					{
						if (curChar >= m_ListSelection.StartChar() && curChar < m_ListSelection.EndChar())
						{
							//if (iItem == 3)stdlog("1 %d\n", iItem);
							curFlag |= bSelected;
						}
					}
					else if (iItem == m_ListSelection.StartItem())
					{
						if (curChar >= m_ListSelection.StartChar())
						{
							//if (iItem == 3)stdlog("2 %d\n", iItem);
							curFlag |= bSelected;
						}
					}
					else if (iItem == m_ListSelection.EndItem())
					{
						if (curChar < m_ListSelection.EndChar())
						{
							//if (iItem == 3)stdlog("3 %d\n", iItem);
							curFlag |= bSelected;
						}
					}
				}
			}
			if (cbInfo > curChar)
			{
				curFlag |= bNotTrace;
			}
			//if (iItem == 3)stdlog("oldFlag %d, curFlag %d curChar %d \n", oldFlag, curFlag, curChar);

			if (oldFlag != curFlag || curChar == (cbText - 1))
			{
				if (oldFlag == curFlag && curChar == (cbText - 1))
				{
					curChar++;
				}
				if (startChar != curChar)
				{
				again:
					if (oldFlag & bcurSearch)
						curBkColor = curSerachColor;
					else if (oldFlag & bSearched)
						curBkColor = serachColor;
					else if (pNode->isFlow() || (oldFlag  & bNotTrace))
						curBkColor = infoBkColor;
					else
						curBkColor = bkColor;

					if (oldFlag & bSelected)
					{
						curBkColor = selBkColor;
					}
					::SetBkColor(itemMemDC, curBkColor);

					if (curBkColor != infoBkColor)
						::SetTextColor(itemMemDC, selColor);
					else if (pNode->isFlow() || (oldFlag  & bNotTrace))
						::SetTextColor(itemMemDC, infoTextColor);
					else
						::SetTextColor(itemMemDC, textColor);

					if ((oldFlag & bSelected) || (oldFlag & bcurSearch) || (oldFlag & bSearched))
					{
						::SetTextColor(itemMemDC, selColor);
					}

					//if (iItem == 3)stdlog("prevChar %d, curChar %d cbText %d\n", prevChar, curChar, cbText);
					TextOut(itemMemDC, textPoint, 0, szText + prevChar, curChar - prevChar);
					SIZE size;
					GetTextExtentPoint32(itemMemDC, szText + prevChar, curChar - prevChar, &size);
					textPoint += size.cx;
					prevChar = curChar;
					oldFlag = curFlag;
					if (curChar == (cbText - 1))
					{
						prevChar = curChar;
						curChar = cbText;
						goto again;
					}
				}
				if (startChar == curChar)
					oldFlag = curFlag;
			}
			curChar++;
		}

		if (iItem == m_ListSelection.CurItem())
		{
			RECT rc = { -1, 0, cx+2, cy };
			FrameRect(itemMemDC, &rc, Gdi::SelectionBrush);
			//DrawFocusRect(memDC, &rc);
		}

	}

	::BitBlt(hdc, 0, rcItem.top, cx, cy, itemMemDC, 0, 0, SRCCOPY);

}

CHAR* CLogListView::getText(int iItem, int* cBuf, bool onlyTraces, int* cInfo)
{
	const int MAX_BUF_LEN = 2 * MAX_LOG_LEN - 10;
	static CHAR pBuf[MAX_BUF_LEN + 10];
	int c_Buf, c_Info;
	int& cb = cBuf ? *cBuf : c_Buf;
	int& ci = cInfo ? *cInfo : c_Info;
	CHAR* ret = pBuf;
	cb = 0;
	pBuf[0] = 0;
	LOG_NODE* pNode = gArchive.getListedNodes()->getNode(iItem);
	if (!pNode)
	{
		//ATLASSERT(0); 
		return ret;
	}

	cb = 0;
	bool funcColFound = false;
	for (int i = 0; i < m_cActualColumns && cb < MAX_BUF_LEN; i++)
	{
		if (onlyTraces && LOG_COL != m_DetailType[i])
			continue;
		if (FUNC_COL == m_DetailType[i])
			funcColFound = true;
		if (funcColFound && pNode->isFlow() && LOG_COL == m_DetailType[i])
			break;
		int c;
		CHAR* p = pNode->getListText(&c, m_DetailType[i], iItem);
		if (c + cb >= MAX_BUF_LEN)
			c = MAX_BUF_LEN - cb;
		memcpy(pBuf + cb, p, c);
		cb += c;
		if (i < m_cActualColumns - 1)
		{
			memcpy(pBuf + cb, " ", 1);
			cb += 1;
			ci = cb;
		}
	}
	pBuf[cb] = 0;

	return ret;
}