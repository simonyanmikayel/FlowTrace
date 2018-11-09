#include "stdafx.h"
#include "Resource.h"
#include "LogListView.h"
#include "FlowTraceView.h"
#include "Settings.h"
#include "SearchInfo.h"
#include "MainFrm.h"

static const int TEXT_MARGIN = 8;
enum ICON_TYPE { ICON_BOOKMARK, ICON_MAX };//ICON_SYNC, ICON_BACK_TRACE, 
extern HWND       hwndMain;

static int ICON_INDEX_LIST_ENTER;
static int ICON_INDEX_LIST_EXIT;
static int ICON_INDEX_LIST_TRACE_SEL;
static int ICON_INDEX_BOOKMARK;
static int ICON_INDEX_SYNC;

CLogListView::CLogListView(CFlowTraceView* pView)
	: m_pView(pView)
	, m_hasCaret(false)
	, m_IsCupture(false)
	, m_cColumns(0)
	, m_cActualColumns(0)
	, m_lengthCalculated(1)
	, m_recCount(0)
	, m_curBookmark(0)

{
	ClearColumnInfo();
	m_hdc = CreateCompatibleDC(NULL);// CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);

	m_hListImageList = ImageList_Create(16, 16, ILC_MASK, 1, 0);
	ICON_INDEX_LIST_ENTER = ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_ENTER)));
	ICON_INDEX_LIST_EXIT = ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_EXIT)));
	ICON_INDEX_LIST_TRACE_SEL = ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_TRACE_SEL)));
	ICON_INDEX_BOOKMARK = ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_BOOKMARK)));
	ICON_INDEX_SYNC = ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_SYNC)));
}

CLogListView::~CLogListView()
{
	DeleteDC(m_hdc);
}

void CLogListView::ClearColumnInfo()
{
	ZeroMemory(m_ColType, sizeof(m_ColType));
	ZeroMemory(m_ActualColType, sizeof(m_ActualColType));
	ZeroMemory(m_ColSubItem, sizeof(m_ColSubItem));
	for (int i = 0; i < MAX_COL; i++)
	{
		m_ColLen[i] = ICON_LEN + TEXT_MARGIN;
	}
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

void CLogListView::CopySelection(bool onlyTraces)
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
					if (copy) memcpy(lock + len, log, l);
					len += l;
				}
				else if (i == m_ListSelection.StartItem() && i == m_ListSelection.EndItem())
				{
					if (m_ListSelection.StartChar() < logLen)
					{
						l = min(m_ListSelection.EndChar(), logLen) - m_ListSelection.StartChar();
					}
					if (l)
					{
						if (copy) memcpy(lock + len, log + m_ListSelection.StartChar(), l);
						len += l;
					}
				}
				else if (i == m_ListSelection.StartItem())
				{
					if (m_ListSelection.StartChar() < logLen)
					{
						l = logLen - m_ListSelection.StartChar();
					}
					if (l)
					{
						if (copy) memcpy(lock + len, log + m_ListSelection.StartChar(), l);
						len += l;
					}
				}
				else if (i == m_ListSelection.EndItem())
				{
					l = min(m_ListSelection.EndChar(), logLen);
					if (l)
					{
						if (copy) memcpy(lock + len, log, l);
						len += l;
					}
				}
				else //(i > m_ListSelection.StartItem() && i < m_ListSelection.EndItem())
				{
					l = logLen;
					if (l)
					{
						if (copy) memcpy(lock + len, log, l);
						len += l;
					}
				}
				if (l)
				{
					if (i != m_ListSelection.EndItem())
					{
						if (copy) memcpy(lock + len, "\r\n", 2);
						len += 2;
					}
					else
					{
						if (copy) memcpy(lock + len, "", 1);
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
		nLast = GetItemCount();

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
	GetTextExtentPoint32(m_hdc, log, startChar, &size1);
	GetTextExtentPoint32(m_hdc, log, endChar, &size2);

	RECT rcClient, rcItem;
	GetClientRect(&rcClient);
	ListView_GetSubItemRect(m_hWnd, iItem, m_LogCol, LVIR_BOUNDS, &rcItem);

	SCROLLINFO si;
	si.fMask = SIF_RANGE | SIF_POS;
	GetScrollInfo(SB_HORZ, &si);
	//stdlog("size1.cx %d size2.cx %d si.nPos = %d si.nMax = %d rcClient.right = %d \n", size1.cx, size2.cx, si.nPos, rcClient.right, si.nMax);
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
	for (DWORD i = 0; i < m_recCount; i++)
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
	for (DWORD i = 0; i < m_recCount; i++)
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

	int x = ICON_MAX * ICON_LEN + TEXT_MARGIN;
	int y = 0;

	if (GetItemCount())
	{
		RECT itemRect;
		int curItem = m_ListSelection.CurItem();
		if (GetSubItemRect(curItem, m_LogCol, LVIR_LABEL, &itemRect))
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
			GetTextExtentPoint32(m_hdc, szText, cb, &size);
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

	if (!GetItemCount())
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
	if (!GetItemCount())
		return 0;

	int xPos = GET_X_LPARAM(lParam);
	if (xPos < ICON_MAX * ICON_LEN + TEXT_MARGIN)
		return 0;


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

    Helpers::AddMenu(hMenu, cMenu, ID_EDIT_COPY_TRACES, _T("Copy Trace"));

    disable = (m_ListSelection.IsEmpty());
    Helpers::AddMenu(hMenu, cMenu, ID_EDIT_COPY, _T("&Copy\tCtrl+C"), disable);

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
	else if (nRet == ID_SYNC_VIEWES)
	{
		m_pView->SyncViews();
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
	SubItemHitTest(&ht);
	//stdlog("xPos = %d yPos = %d flags = %d ht.iItem = %d\n", xPos, yPos, ht.flags, ht.iItem);
	if ((LVHT_ONITEM & ht.flags) && (ht.iItem >= 0))
	{
		LIST_COL col = m_ColType[ht.iSubItem];

		if (col == ICON_COL)
		{
			//if (ICON_BOOKMARK == (xPos / ICON_LEN))
			ToggleBookmark(ht.iItem);
			return false;
		}

		CRect itemRect;
		GetSubItemRect(ht.iItem, m_LogCol, LVIR_LABEL, &itemRect);
		//stdlog("xPos = %d itemRect l = %d r = %d t = %d b = %d\n", xPos, itemRect.left, itemRect.right, itemRect.top, itemRect.bottom);
		if (xPos >= itemRect.left)
		{
			int logLen;
			char* log = getText(ht.iItem, &logLen);
			int nFit = 0;
			if (logLen)
			{
				int nMaxExtent = xPos - itemRect.left - TEXT_MARGIN;
				int cb = logLen;
				SIZE  size;
				GetTextExtentExPoint(m_hdc, log, cb, nMaxExtent, &nFit, NULL, &size);
				if (nFit < logLen)
				{
					SIZE  size1, size2;
					GetTextExtentPoint32(m_hdc, log, nFit, &size1);
					GetTextExtentPoint32(m_hdc, log, nFit + 1, &size2);
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
	}
	return false;
}

void CLogListView::ApplySettings(bool fontChanged)
{
	if (fontChanged)
	{
		SelectObject(m_hdc, gSettings.GetFont());
		SetFont(gSettings.GetFont());
		if (!gSettings.CheckUIFont(m_hdc))
		{
			SelectObject(m_hdc, gSettings.GetFont());
			SetFont(gSettings.GetFont());
		}
	}
	SetBkColor(gSettings.LogListBkColor());
	//SetTextBkColor(gSettings.LogListBkColor());
	//SetTextColor(gSettings.LogListTxtColor());
	SetColumns();
	//PostMessage(WM_SET_LIST_COLUNS, 0, 0);
}

void CLogListView::Clear()
{
	if (GetItemCount())
	{
		SetItemCountEx(0, 0);
		DeleteAllItems();
		m_recCount = 0;
		m_ListSelection.Clear();
		::SetFocus(hwndMain); //force carret destroy
		SetColumns();
		Helpers::UpdateStatusBar();
	}
}

void CLogListView::SetColumnLen(LIST_COL col, int len)
{
	if (LOG_COL == col)
	{
		RECT rc;
		GetClientRect(&rc);
		len = max(rc.right + 16, len);
	}
	//stdlog("len = %d col = %d m_ColLen = %d\n", len, col, m_ColLen[col]);
	int iSubItem = getSubItem(col);
	if (m_ColLen[col] < len)
	{
		m_ColLen[col] = len;
		SetColumnWidth(iSubItem, m_ColLen[col]);
	}
}

void CLogListView::OnSize(UINT nType, CSize size)
{
	SetColumnLen(LOG_COL, 2 * TEXT_MARGIN);
}

void CLogListView::RefreshList(bool redraw)
{
	DWORD newCount = gArchive.getListedNodes()->Count();
	if (newCount)
	{
		if (newCount == m_recCount && (!redraw))
			return;
		if (newCount < m_recCount || redraw)
			Clear();
		int oldRecCount = m_recCount;
		m_recCount = newCount;
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

void CLogListView::AddColumn(CHAR* szHeader, LIST_COL col)
{
	if (col == ICON_COL || col == LOG_COL)
	{
		InsertColumn(m_cColumns, szHeader, LVCFMT_LEFT, m_ColLen[col], 0, -1, -1);
		m_ColSubItem[col] = m_cColumns;
		m_ColType[m_cColumns] = col;
		m_cColumns++;
	}
	m_ActualColType[m_cActualColumns++] = col;
}

void CLogListView::SetColumns()
{
	for (int i = m_cColumns - 1; i > 0; i--)
		DeleteColumn(i);

	ClearColumnInfo();

	if (m_cColumns == 0)
		AddColumn(_T(""), ICON_COL);

	m_cColumns = 1;
	m_cActualColumns = 1;

	if (gSettings.GetColLineNN())
		AddColumn(_T("Line"), LINE_NN_COL);
	if (gSettings.GetColNN())
		AddColumn(_T("NN"), NN_COL);
	if (gSettings.GetColApp())
		AddColumn(_T("App"), APP_COLL);
	if (gSettings.GetColPID() || gSettings.GetColTID() || gSettings.GetColThreadNN())
		AddColumn(_T("TID"), THREAD_COL);
	if (gSettings.GetColTime())
		AddColumn(_T("Time"), TIME_COL);
	if (gSettings.GetColFunc())
		AddColumn(_T("Function"), FUNC_COL);
	if (gSettings.GetColLine())
		AddColumn(_T("Line"), CALL_LINE_COL);

	//Log alwayse lastt
	AddColumn(_T("Trace"), LOG_COL);

	m_LogCol = m_cColumns - 1;
}

LRESULT CLogListView::OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & bHandled)
{
	//bHandled = false;  return 0; // no handled

	CClientDC dc(m_hWnd);
	RECT rcItem, rcClient;
	GetClientRect(&rcClient);
	RECT rcErase = rcClient;
	int count = GetItemCount();
	if (count > 0)
	{
		int top = GetTopIndex();
		GetItemRect(top, &rcItem, LVIR_BOUNDS);
		int cy = (rcItem.bottom - rcItem.top);
		while ((top < count - 1) && (rcErase.top + cy) < rcErase.bottom)
		{
			rcErase.top += cy;
		}
	}
	if (rcErase.top < rcErase.bottom)
		dc.FillSolidRect(&rcErase, gSettings.LogListBkColor());

	//stdlog("iItemRect \tl = %d \tr = %d \tt = %d \tb = %d\n", rcItem.left, rcItem.right, rcItem.top, rcItem.bottom);
	//stdlog(" \trcClient \tl = %d \tr = %d \tt = %d \tb = %d\n", rcClient.left, rcClient.right, rcClient.top, rcClient.bottom);
	return 1; // handled
}

void CLogListView::ItemPrePaint(int iItem, HDC hdc, RECT rcItem)
{
	LOG_NODE* pNode = gArchive.getListedNodes()->getNode(iItem);
	if (!pNode)
	{
		ATLASSERT(0);
		return;
	}
	if (pNode->lengthCalculated != m_lengthCalculated)
	{
		pNode->lengthCalculated = m_lengthCalculated;
		//calculate lengths of all columns
		SetColumnLen(m_ColType[0], ICON_MAX * ICON_LEN);
		SIZE size;
		//for (int i = 1; i < m_cColumns; i++)
		{
			size.cx = 0;
			int cbText;
			char* szText = getText(iItem, &cbText);
			if (cbText)
				GetTextExtentPoint32(hdc, szText, cbText, &size);
			SetColumnLen(LOG_COL, size.cx + (2 * TEXT_MARGIN));
			//stdlog("\t iItem i %d %d size.cx %d %s\n", iItem, i, size.cx, szText1);
		}
	}
}

//#define _USE_MEMDC
void CLogListView::DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rcItem)
{
	LIST_COL col = getGolumn(iSubItem);
	LOG_NODE* pNode = gArchive.getListedNodes()->getNode(iItem);
	if (!pNode)
	{
		ATLASSERT(0);
		return;
	}

	if (col == ICON_COL)
	{
		int y = rcItem.top + (rcItem.bottom - rcItem.top) / 2 - ICON_LEN / 2;
		int x = rcItem.left + ICON_BOOKMARK * ICON_LEN;

		if (pNode && pNode->isSynchronized(gSyncronizedNode))
		{
			ImageList_Draw(m_hListImageList, ICON_INDEX_LIST_TRACE_SEL, hdc, x, y, ILD_NORMAL);
		}
		if (pNode->bookmark)
		{
			ImageList_Draw(m_hListImageList, ICON_INDEX_BOOKMARK, hdc, x, y, ILD_NORMAL);
			TCHAR szText[20];
			int cbText = sprintf_s(szText, "%d", pNode->bookmark);
			SIZE  textSize;
			//stdlog("szText [%s] %d %d \n", szText, cbText, strlen(szText));
			GetTextExtentExPoint(hdc, szText, cbText, 0, NULL, NULL, &textSize);

			int y1 = rcItem.top + (rcItem.bottom - rcItem.top) / 2 - textSize.cy / 2;
			int x1 = rcItem.left + ICON_BOOKMARK * ICON_LEN + ICON_LEN /2- textSize.cx / 2;
			::SetTextColor(hdc, RGB(64,128,255));
			TextOut(hdc, x1, y1, szText, cbText);
		}
		if (pNode->isFlow())
		{
			if (((FLOW_NODE*)pNode)->isEnter())
				ImageList_Draw(m_hListImageList, ICON_INDEX_LIST_ENTER, hdc, x, y, ILD_NORMAL);
			else
				ImageList_Draw(m_hListImageList, ICON_INDEX_LIST_EXIT, hdc, x, y, ILD_NORMAL);
		}
		return;
	}

	RECT rcClient;
	GetClientRect(&rcClient);

	//CClientDC dc(m_hWnd);
	//hdc = dc.m_hDC;
	//ListView_GetSubItemRect(m_hWnd, iItem, m_LogCol, LVIR_BOUNDS, &rcItem);
	//stdlog("%d \tiItemRect \tl = %d \tr = %d \tt = %d \tb = %d\n", iItem, rcItem.left, rcItem.right, rcItem.top, rcItem.bottom);
	//stdlog(" \trcClient \tl = %d \tr = %d \tt = %d \tb = %d\n", rcClient.left, rcClient.right, rcClient.top, rcClient.bottom);
	//return;

	rcItem.left += TEXT_MARGIN;
	rcItem.right -= TEXT_MARGIN;

	int cx = rcClient.right - rcClient.left;
	if (rcItem.left > 0)
		cx -= rcItem.left;
	if (cx > (rcItem.right - rcItem.left))
		cx = (rcItem.right - rcItem.left);

	if (cx <= 0)
		return;

	int x = 0;
	if (rcItem.left < 0)
		x = -rcItem.left;

	int cbText, cbInfo;
	char* szText = getText(iItem, &cbText, false, &cbInfo);

	//stdlog("iItem %d iSubItem %d col %d %s \n", iItem, iSubItem, col, szText);

	int startChar = 0, endChar = 0;
	SIZE  size, textSize;
	//stdlog("szText [%s] %d %d \n", szText, cbText, strlen(szText));
	GetTextExtentExPoint(hdc, szText, cbText, x, &startChar, NULL, &textSize);
	if (x <= 0)
	{
		startChar = 0;
	}
	GetTextExtentExPoint(hdc, szText + startChar, cbText - startChar, cx, &endChar, NULL, &size);
	GetTextExtentPoint32(hdc, szText, startChar, &size);
#ifdef _USE_MEMDC
	POINT  textPoint = { 0, 0 };
#else
	POINT  textPoint = { 0, rcItem.top };
#endif
	if (size.cx < x)
		textPoint.x -= (x - size.cx);
	endChar += startChar;
	endChar += 10;
	if (endChar > cbText)
		endChar = cbText;
	endChar = cbText;

	DWORD textColor, bkColor, selBkColor, selColor, serachColor, curSerachColor, infoBkColor, infoTextColor, curBkColor;
	selColor = gSettings.SelectionTxtColor();
	selBkColor = gSettings.SelectionBkColor(GetFocus() == m_hWnd);
	textColor = gSettings.LogListTxtColor();
	bkColor = gSettings.LogListBkColor();
	serachColor = gSettings.SerachColor();
	curSerachColor = gSettings.CurSerachColor();
	infoTextColor = gSettings.InfoTextColor();
	infoBkColor = gSettings.LogListBkColor();

#ifdef _USE_MEMDC
	HDC hMemDC = ::CreateCompatibleDC(hdc);
	SelectObject(hMemDC, gSettings.GetFont());
	RECT rcMemDC = { 0, 0, cx, rcItem.bottom - rcItem.top };
	HBITMAP hBmp = ::CreateCompatibleBitmap(hdc, rcMemDC.right - rcMemDC.left, rcMemDC.bottom - rcMemDC.top);
	HBITMAP oldBmp = (HBITMAP)::SelectObject(hMemDC, hBmp);
	HBRUSH hbr = ::CreateSolidBrush(bkColor);
	::FillRect(hMemDC, &rcMemDC, hbr);
	::DeleteObject(hbr);
#else
	HDC hMemDC = hdc;
	if (rcItem.left > 0)
		textPoint.x += rcItem.left;
#endif
	COLORREF old_textColor = ::GetTextColor(hdc);
	COLORREF old_bkColor = ::GetBkColor(hdc);
	int old_bkMode = ::GetBkMode(hdc);
	::SetBkMode(hMemDC, OPAQUE);

	if (col == LOG_COL)
	{
		if (pNode->isTrace())
		{
			gSettings.SetConsoleColor(((TRACE_NODE*)pNode)->color, textColor, bkColor);
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
		while (curChar < endChar)
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

			if (oldFlag != curFlag || curChar == (endChar - 1))
			{
				if (oldFlag == curFlag && curChar == (endChar - 1))
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
					::SetBkColor(hMemDC, curBkColor);

					if (curBkColor != infoBkColor)
						::SetTextColor(hMemDC, selColor);
					else if (pNode->isFlow() || (oldFlag  & bNotTrace))
						::SetTextColor(hMemDC, infoTextColor);
					else
						::SetTextColor(hMemDC, textColor);



					if ((oldFlag & bSelected) || (oldFlag & bcurSearch) || (oldFlag & bSearched))
					{
						::SetTextColor(hMemDC, selColor);
					}

					//if (iItem == 3)stdlog("prevChar %d, curChar %d endChar %d\n", prevChar, curChar, endChar);
					TextOut(hMemDC, textPoint.x, textPoint.y, szText + prevChar, curChar - prevChar);
					GetTextExtentPoint32(hMemDC, szText + prevChar, curChar - prevChar, &size);
					textPoint.x += size.cx;
					prevChar = curChar;
					oldFlag = curFlag;
					if (curChar == (endChar - 1))
					{
						prevChar = curChar;
						curChar = endChar;
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
			RECT rcText = { rcItem.left - 5, rcItem.top, rcItem.right + 5, rcItem.bottom };
			if (GetFocus() == m_hWnd)
			{
				CBrush brush2;
				brush2.CreateSolidBrush(gSettings.SelectionBkColor(true));
				FrameRect(hdc, &rcText, brush2);
			}
			else
			{
				DrawFocusRect(hdc, &rcText);
			}
		}

	}

#ifdef _USE_MEMDC
	::BitBlt(hdc, rcItem.left > 0 ? rcItem.left : 0, rcItem.top, cx, rcItem.bottom - rcItem.top, hMemDC, 0, 0, SRCCOPY);
	::SelectObject(hMemDC, oldBmp);
	DeleteObject(hBmp);
	DeleteDC(hMemDC);
#else
	::SetTextColor(hdc, old_textColor);
	::SetBkColor(hdc, old_bkColor);
	::SetBkMode(hdc, old_bkMode);
#endif
	}

CHAR* CLogListView::getText(int iItem, int* cBuf, bool onlyTraces, int* cInfo)
{
	const int MAX_BUF_LEN = 2 * MAX_TRCAE_LEN - 10;
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
	for (int i = 1; i < m_cActualColumns && cb < MAX_BUF_LEN; i++)
	{
		if (onlyTraces && LOG_COL != m_ActualColType[i])
			continue;
		if (FUNC_COL == m_ActualColType[i])
			funcColFound = true;
		if (funcColFound && pNode->isFlow() && LOG_COL == m_ActualColType[i])
			break;
		int c;
		CHAR* p = pNode->getListText(&c, m_ActualColType[i], iItem);
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