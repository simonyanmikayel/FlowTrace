#include "stdafx.h"
#include "Resource.h"
#include "LogListView.h"
#include "FlowTraceView.h"
#include "Settings.h"
#include "SearchInfo.h"

static const int TEXT_MARGIN = 8;
extern HWND       hwndMain;

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
    ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_ENTER))); //0
    ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_EXIT))); //1
    ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_TRACE))); //2
    ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_ENTER_SEL))); //3
    ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_EXIT_SEL))); //4
    ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_TRACE_SEL))); //5
    ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_EMPTY))); //6

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
    stdlog("start %d %d end %d %d cur %d %d \n", start.iItem, start.iChar, end.iItem, end.iChar, cur.iItem, cur.iChar);
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
    LOG_NODE* pNode = listNodes->getNode(m_ListSelection.GetItem());
    if (pNode)
        pNode = pNode->getSyncNode();
    return pNode;
}

void CLogListView::CopySelection(bool bCopyPath)
{
    if (m_ListSelection.IsEmpty() && !bCopyPath)
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
        if (bCopyPath)
        {
            LOG_NODE* pNode = GetSynkNode();
            if (pNode)
            {
                int cbText;
                char* szText = pNode->getPathText(&cbText);
                l = cbText;
                if (l)
                {
                    if (copy) memcpy(lock + len, szText, l);
                    len += l;
                    if (copy) memcpy(lock + len, "", 1);
                    len += 1;
                }
            }
        }
        else if (IsLogSelection())
        {
            l = 0;
            for (int i = logSelection().StartItem(); i <= logSelection().EndItem(); i++)
            {
                int logLen;
                char* log = getText(i, &logLen, LOG_COL);
                if (logLen)
                {
                    if (i == logSelection().StartItem() && i == logSelection().EndItem())
                    {
                        if (logSelection().StartChar() < logLen)
                        {
                            l = min(logSelection().EndChar(), logLen) - logSelection().StartChar();
                        }
                        if (l)
                        {
                            if (copy) memcpy(lock + len, log + logSelection().StartChar(), l);
                            len += l;
                        }
                    }
                    else if (i == logSelection().StartItem())
                    {
                        if (logSelection().StartChar() < logLen)
                        {
                            l = logLen - logSelection().StartChar();
                        }
                        if (l)
                        {
                            if (copy) memcpy(lock + len, log + logSelection().StartChar(), l);
                            len += l;
                        }
                    }
                    else if (i == logSelection().EndItem())
                    {
                        l = min(logSelection().EndChar(), logLen);
                        if (l)
                        {
                            if (copy) memcpy(lock + len, log, l);
                            len += l;
                        }
                    }
                    else //(i > logSelection().StartItem() && i < logSelection().EndItem())
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
                        if (i != logSelection().EndItem())
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
        else
        {
            int iItem = m_ListSelection.GetInfoSelectionItem();
            if (iItem >= 0 && iItem < (int)m_recCount)
            {
                int cbText;
                char* szText = getText(iItem, &cbText, m_ListSelection.GetColumn());
                l = cbText;
                if (l)
                {
                    if (copy) memcpy(lock + len, szText, l);
                    len += l;
                    if (copy) memcpy(lock + len, "", 1);
                    len += 1;
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
    int oldItem = m_ListSelection.GetItem();
    if (IsLogSelection())
    {
        bool movToEnd = iItem >= (int)m_recCount;
        //stdlog("iItem  %d %d\n", iItem, iChar);
        iItem = max(0, min(iItem, (int)m_recCount - 1));

        int logLen;
        char* log = getText(iItem, &logLen, LOG_COL);
        if (!logLen) { return; }

        if (iChar < 0 || movToEnd)
            iChar = logLen;

        iChar = max(0, min(iChar, logLen));
        bool prevEmpty = logSelection().IsEmpty();
        int preStart = logSelection().StartItem();
        int prevEnd = logSelection().EndItem();

        //stdlog("iItem1 %d %d\n", iItem, iChar);
        if (logSelection().Move(iItem, iChar, extend))
        {
            bool curEmpty = logSelection().IsEmpty();
            int curStart = logSelection().StartItem();
            int curEnd = logSelection().EndItem();
            if (!prevEmpty || !curEmpty) {
                Redraw(min(preStart, curStart), max(prevEnd, curEnd));
            }
        }

        if (ensureVisible)
            EnsureTextVisible(logSelection().CurItem(), logSelection().CurChar(), logSelection().CurChar());
    }
    else
    {
        iItem = max(0, min(iItem, (int)m_recCount - 1));
        m_ListSelection.SetItem(iItem);
        Redraw(iItem, iItem);
        if (ensureVisible)
            ShowItem(iItem, false);
    }
    Redraw(oldItem, oldItem);
    UpdateCaret();

    static TCHAR pBuf[128];
    _sntprintf(pBuf, sizeof(pBuf) - 1, TEXT("Ln: %s"), Helpers::str_format_int_grouped(m_ListSelection.GetItem() + 1));
    ::SendMessage(hWndStatusBar, SB_SETTEXT, 2, (LPARAM)pBuf);
    LOG_NODE* pNode = GetSynkNode();
    if (pNode)
        m_pView->ShowInfo(pNode->getPathText());
    else
        m_pView->ShowInfo(_TEXT(""));
}

void CLogListView::MoveSelectionToEnd(bool extend)
{
    if (m_recCount)
    {
        int lastItem = (int)m_recCount - 1;
        int cbLastText;
        getText(lastItem, &cbLastText, LOG_COL);
        MoveSelectionEx(lastItem, cbLastText, extend);
    }
}

void CLogListView::EnsureTextVisible(int iItem, int startChar, int endChar)
{
    iItem = max(0, min(iItem, (int)m_recCount - 1));

    int logLen;
    char* log = getText(iItem, &logLen, LOG_COL);
    if (!logLen) return;
    startChar = max(0, min(startChar, logLen));
    endChar = max(0, min(endChar, logLen));

    ShowItem(iItem, false);

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
    LOG_NODE* pSyncNode = m_pView->selectedNode();
    for (DWORD i = 0; i < m_recCount; i++)
    {
        if (listNodes->getNode(i)->isSynchronized(pSyncNode))
        {
            ShowItem(i, scrollToMiddle);
            break;
        }
    }

}

void CLogListView::ShowNode(LOG_NODE* pNode, bool scrollToMiddle)
{
    for (DWORD i = 0; i < m_recCount; i++)
    {
        if (pNode == listNodes->getNode(i))
        {
            ShowItem(i, scrollToMiddle);
            break;
        }
    }
}

void CLogListView::ShowItem(DWORD i, bool scrollToMiddle)
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

void CLogListView::UpdateCaret()
{
    //m_Selection.print();
    //if (ListView_IsItemVisible(m_hWnd, m_Selection.cur.iItem))
    if (!m_hasCaret)
        return;

    int x = 0;
    int y = 0;
    if (IsLogSelection())
    {
        if (GetItemCount())
        {
            RECT itemRect;
            int curItem = logSelection().CurItem();
            if (GetSubItemRect(curItem, m_LogCol, LVIR_LABEL, &itemRect))
            {
                RECT viewRect;
                GetViewRect(&viewRect);

                int cbText;
                char* szText = getText(curItem, &cbText, LOG_COL);
                x = TEXT_MARGIN;
                y = itemRect.top + (itemRect.bottom - itemRect.top) / 2 - gSettings.GetFontHeight() / 2;
                int cb = logSelection().CurChar();
                if (cb > cbText)
                    cb = cbText;
                SIZE  size;
                GetTextExtentPoint32(m_hdc, szText, cb, &size);
                x = itemRect.left + size.cx + TEXT_MARGIN;
                //STDLOG("cb = %d x = %d y = %d iItem = %d itemRect l = %d r = %d t = %d b = %d\n", cb, x, y, curItem, itemRect.left, itemRect.right, itemRect.top, itemRect.bottom);

                //udjust searchInfo.curLine

            }
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
    bool isLogSelection = IsLogSelection();
    int cbCurText;
    char* szText = getText(m_ListSelection.GetItem(), &cbCurText, LOG_COL);
    if (!cbCurText) return 0;

    int curItem = m_ListSelection.GetItem();
    int curChar = isLogSelection ? logSelection().CurChar() : 0;
    int startItem = isLogSelection ? logSelection().StartItem() : m_ListSelection.GetItem();
    int startChar = isLogSelection ? logSelection().StartChar() : 0;
    int endItem = isLogSelection ? logSelection().EndItem() : m_ListSelection.GetItem();
    int endChar = isLogSelection ? logSelection().EndChar() : 0;

    switch (wParam)
    {
    case VK_HOME:       // Home 
    {
        if (bCtrlPressed || !isLogSelection)
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
        if (bCtrlPressed || !isLogSelection)
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
        if (isLogSelection)
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
                    else if (logSelection().IsEmpty())
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
        else
        {
            MoveSelectionEx(startItem - 1, 0, 0);
        }
    }
    break;
    case VK_RIGHT:      // Right arrow 
    {
        if (isLogSelection)
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
                    else if (logSelection().IsEmpty())
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
        else
        {
            MoveSelectionEx(startItem + 1, 0, 0);
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
    //UINT nChar = (TCHAR)wParam;
    bHandled = TRUE;
    return 0;
}

LRESULT CLogListView::OnLButtonDblClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & bHandled)
{
    if (!IsLogSelection())
        return 0;

    if (!GetItemCount())
        return 0;

    int cbCurText;
    char* szText = getText(logSelection().CurItem(), &cbCurText, LOG_COL);
    if (!cbCurText) 
        return 0;

    int curChar = logSelection().CurChar();

    int prevChar = curChar;
    while (prevChar > 0 && IsWordChar(szText[prevChar - 1]))
        prevChar--;
    //if (prevChar == curChar)
    //  prevChar--;

    MoveSelectionEx(logSelection().CurItem(), prevChar, false);

    int nextChar = curChar;
    while (nextChar < cbCurText && IsWordChar(szText[nextChar]))
        nextChar++;
    if (nextChar == curChar)
        nextChar++;
    MoveSelectionEx(logSelection().CurItem(), nextChar, true);

    bHandled = TRUE;
    return 0;
}

LRESULT CLogListView::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    UdjustSelectionOnMouseEven(uMsg, wParam, lParam);

    bHandled = TRUE;

    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);

    DWORD dwFlags;
    POINT pt = { xPos, yPos };
    ClientToScreen(&pt);
    HMENU hMenu = CreatePopupMenu();
    dwFlags = MF_BYPOSITION | MF_STRING;
    if (GetItemCount() == 0 || !GetSynkNode())
        dwFlags |= MF_DISABLED;
    InsertMenu(hMenu, 0, dwFlags, ID_TREE_COPY_CALL_PATH, _T("Copy call path"));
    dwFlags = MF_BYPOSITION | MF_STRING;
    if (m_ListSelection.IsEmpty())
        dwFlags |= MF_DISABLED;
    InsertMenu(hMenu, 0, dwFlags, ID_EDIT_COPY, _T("Copy"));
    UINT nRet = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, 0);
    DestroyMenu(hMenu);

    if (nRet == ID_EDIT_COPY)
    {
        CopySelection(false);
    }
    else if (nRet == ID_TREE_COPY_CALL_PATH)
    {
        CopySelection(true);
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

LRESULT CLogListView::OnLButtonUp(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    if (m_IsCupture)
    {
        m_IsCupture = false;
        ReleaseCapture();
    }
    return 0;
}

LRESULT CLogListView::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    UdjustSelectionOnMouseEven(uMsg, wParam, lParam);

    return 0;
}

void CLogListView::ToggleBookmark(DWORD item)
{
    LOG_NODE* pNode = listNodes->getNode(item);
    if (pNode)
    {
        pNode->bookmark = pNode->bookmark ? 0 : 1;
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
        int count = (int)(listNodes->Count());
        if (m_curBookmark > count)
            m_curBookmark = count;
        if (ID_BOOKMARKS_NEXTBOOKMARK == wID)
        {
            for (int i = m_curBookmark + 1; i < count; i++)
            {
                if (listNodes->getNode(i)->bookmark)
                {
                    bookmark = i;
                    break;
                }
            }
            if (bookmark == -1)
            {
                for (int i = 0; i < count; i++)
                {
                    if (listNodes->getNode(i)->bookmark)
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
                if (listNodes->getNode(i)->bookmark)
                {
                    bookmark = i;
                    break;
                }
            }
            if (bookmark == -1)
            {
                for (int i = count - 1; i >= 0; i--)
                {
                    if (listNodes->getNode(i)->bookmark)
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
            ShowItem(bookmark, true);
        }
    }
    break;
    case ID_BOOKMARKS_CLEARBOOKMARKS:
    {
        for (DWORD i = 0; i < listNodes->Count(); i++)
        {
            listNodes->getNode(i)->bookmark = 0;
        }
        Redraw(0, listNodes->Count());
    }
    break;
    case ID_BOOKMARKS_TOGGLE:
    {
        ToggleBookmark(m_ListSelection.GetItem());
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
            ToggleBookmark(ht.iItem);
            return false;
        }

        if (!m_ListSelection.IsEmpty())
        {
            LIST_COL curCol = m_ListSelection.GetColumn();
            if (IsLogSelection())
            {
                int curStart = logSelection().StartItem();
                int curEnd = logSelection().EndItem();
                Redraw(min(curStart, curEnd), max(curStart, curEnd));
            }
            else
            {
                Redraw(m_ListSelection.GetItem(), m_ListSelection.GetItem());
            }
        }

        m_ListSelection.SetColumn(col);

        if (col == LOG_COL)
        {
            CRect itemRect;
            GetSubItemRect(ht.iItem, m_LogCol, LVIR_LABEL, &itemRect);
            //stdlog("xPos = %d itemRect l = %d r = %d t = %d b = %d\n", xPos, itemRect.left, itemRect.right, itemRect.top, itemRect.bottom);
            if (xPos >= itemRect.left)
            {
                int logLen;
                char* log = getText(ht.iItem, &logLen, LOG_COL);
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
                    if (IsLogSelection())
                    {
                        if (logSelection().InSelection(ht.iItem, nFit))
                            return false;
                    }
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
        else
        {
            MoveSelectionEx(ht.iItem, 0, 0);
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
        //if (!gSettings.CheckUIFont(m_hdc))
        //{
        //  SelectObject(m_hdc, gSettings.GetFont());
        //  SetFont(gSettings.GetFont());
        //}
    }
    SetBkColor(gSettings.GetBkColor());
    //SetTextBkColor(gSettings.GetBkColor());
    //SetTextColor(gSettings.GetTextColor());
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
    if (m_ColLen[col] < len + (2 * TEXT_MARGIN))
    {
        m_ColLen[col] = len + (2 * TEXT_MARGIN);
        if (iSubItem) //first subitem is icon
            SetColumnWidth(iSubItem, m_ColLen[col]);
    }
}

void CLogListView::OnSize(UINT nType, CSize size)
{
    SetColumnLen(LOG_COL, 0);
}

void CLogListView::RefreshList(bool redraw)
{
    DWORD newCount = listNodes->Count();
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
        int curItem = m_ListSelection.GetItem();
        //stdlog("~~~%d %d\n", curItem, (int)m_recCount - 1);
        if (redraw || oldRecCount == 0 || curItem >= ((int)oldRecCount - 1))
            MoveSelectionEx(m_recCount - 1, 0, false, true);
    }
    else
    {
        Clear();
    }

    static TCHAR pBuf[128];
    _sntprintf(pBuf, sizeof(pBuf) - 1, TEXT("Listed: %s"), Helpers::str_format_int_grouped(m_recCount));
    ::SendMessage(hWndStatusBar, SB_SETTEXT, 1, (LPARAM)pBuf);
}

void CLogListView::AddColumn(TCHAR* szHeader, LIST_COL col)
{
    if (!gSettings.GetCompactView() || col == ICON_COL || col == LOG_COL)
    {
        InsertColumn(m_cColumns, szHeader, LVCFMT_LEFT, m_ColLen[col], 0, -1, -1);
        m_ColSubItem[col] = m_cColumns;
        m_ColType[m_cColumns++] = col;
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
    if (gSettings.GetColPID())
        AddColumn(_T("PID"), PROC_COL);
    if (gSettings.GetColTime())
        AddColumn(_T("Time"), TIME_COL);
    if (gSettings.GetColCallAddr())
        AddColumn(_T("Call Addr"), CALL_COL);
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
        dc.FillSolidRect(&rcErase, gSettings.GetBkColor());

    //stdlog("iItemRect \tl = %d \tr = %d \tt = %d \tb = %d\n", rcItem.left, rcItem.right, rcItem.top, rcItem.bottom);
    //stdlog(" \trcClient \tl = %d \tr = %d \tt = %d \tb = %d\n", rcClient.left, rcClient.right, rcClient.top, rcClient.bottom);
    return 1; // handled
}

void CLogListView::DrawItem(int iItem, HDC hdc, RECT rcItem)
{
    LOG_NODE* pNode = listNodes->getNode(iItem);
    if (!pNode)
    {
        ATLASSERT(0);
        return;
    }
    if (pNode->data->lengthCalculated != m_lengthCalculated)
    {
        pNode->data->lengthCalculated = m_lengthCalculated;
        //calculate lengths of all columns
        SIZE size;
        for (int i = 1; i < m_cColumns; i++)
        {
            int cbText;
            char* szText = getText(iItem, &cbText, m_ColType[i]);
            GetTextExtentPoint32(hdc, szText, cbText, &size);
            SetColumnLen(m_ColType[i], size.cx);
            //stdlog("\t iItem i %d %d size.cx %d %s\n", iItem, i, size.cx, szText1);
        }
    }
}

//#define _USE_MEMDC
void CLogListView::DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rcItem)
{
    LIST_COL col = getGolumn(iSubItem);
    LOG_NODE* pNode = listNodes->getNode(iItem);
    if (!pNode)
    {
        ATLASSERT(0);
        return;
    }

    if (col == ICON_COL)
    {
        RECT rcErase = rcItem;
        if (rcErase.left <= 0)
            rcErase.left = 0;
        if (rcErase.right > 0)
        {
            //COLORREF clrOld = ::SetBkColor(hdc, gSettings.GetBkColor());
            //::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcItem, NULL, 0, NULL);
            //::SetBkColor(hdc, clrOld);
        }

        if (pNode->bookmark)
        {
            CBrush brush;
            brush.CreateSolidBrush(RGB(0, 0, 255));
            HGDIOBJ oldBrush = ::SelectObject(hdc, (HGDIOBJ)(brush.m_hBrush));


            int x = rcItem.left + 1;
            int y = rcItem.top;
            int cx = ICON_LEN + TEXT_MARGIN - 1;
            int cy = rcItem.bottom - rcItem.top - 2;
            POINT apt[] = { { x, y }, { x, y + cy }, { x + cx, y + cy }, { x + cx, y } };
            ::Polygon(hdc, apt, sizeof(apt) / sizeof(apt[0]));

            ::SelectObject(hdc, oldBrush);
        }

        LOG_NODE* pSyncNode = m_pView->selectedNode();
        int iImage = pNode->getListImage(pNode->isSynchronized(pSyncNode));
        ImageList_Draw(m_hListImageList, iImage, hdc, rcItem.left, rcItem.top + (rcItem.bottom - rcItem.top) / 2 - ICON_LEN / 2, ILD_NORMAL);

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
    char* szText = getText(iItem, &cbText, col, &cbInfo);
    BOOL bCompactView = gSettings.GetCompactView();

    //stdlog("iItem %d iSubItem %d col %d %s \n", iItem, iSubItem, col, szText);

    int startChar = 0, endChar = 0;
    SIZE  size;
    if (x > 0)
    {
        GetTextExtentExPoint(hdc, szText, cbText, x, &startChar, NULL, &size);
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
    selColor = gSettings.GetSelColor();
    selBkColor = gSettings.GetBkSelColor();
    textColor = gSettings.GetTextColor();
    bkColor = gSettings.GetBkColor();
    serachColor = gSettings.GetSerachColor();
    curSerachColor = gSettings.GetCurSerachColor();
    infoTextColor = gSettings.GetInfoTextColor();
    infoBkColor = gSettings.GetBkColor();

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
            gSettings.SetConsoleColor(((TRACE_DATA*)pNode->data)->color, textColor, bkColor);
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
                INFO_DATA* pInfoData = (INFO_DATA*)(pNode->data);
                char* p = szText;
                if (pNode->hasSearchResult && searchInfo.total)
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

            if (IsLogSelection() && (iItem >= logSelection().StartItem() && iItem <= logSelection().EndItem()) && !logSelection().IsEmpty())
            {
                if (iItem > logSelection().StartItem() && iItem < logSelection().EndItem())
                {
                    //if (iItem == 3)stdlog("0 %d\n", iItem);
                    curFlag |= bSelected;
                }
                else
                {
                    if (iItem == logSelection().StartItem() && iItem == logSelection().EndItem())
                    {
                        if (curChar >= logSelection().StartChar() && curChar < logSelection().EndChar())
                        {
                            //if (iItem == 3)stdlog("1 %d\n", iItem);
                            curFlag |= bSelected;
                        }
                    }
                    else if (iItem == logSelection().StartItem())
                    {
                        if (curChar >= logSelection().StartChar())
                        {
                            //if (iItem == 3)stdlog("2 %d\n", iItem);
                            curFlag |= bSelected;
                        }
                    }
                    else if (iItem == logSelection().EndItem())
                    {
                        if (curChar < logSelection().EndChar())
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
    }
    else
    {
        if (col == m_ListSelection.GetColumn() && m_ListSelection.GetItem() == iItem)
        {
            ::SetBkColor(hMemDC, selBkColor);
            ::SetTextColor(hMemDC, selColor);
        }
        else
        {
            ::SetBkColor(hMemDC, infoBkColor);
            ::SetTextColor(hMemDC, infoTextColor);
        }
        TextOut(hMemDC, textPoint.x, textPoint.y, szText + startChar, endChar - startChar);
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

TCHAR* CLogListView::getText(int iItem, int* cBuf, LIST_COL col, int* cInfo)
{
    const int MAX_BUF_LEN = 2 * MAX_TRCAE_LEN - 10;
    static TCHAR pBuf[MAX_BUF_LEN + 10];
    int c_Buf, c_Info;
    int& cb = cBuf ? *cBuf : c_Buf;
    int& ci = cInfo ? *cInfo : c_Info;
    TCHAR* ret = pBuf;
    cb = 0;
    pBuf[0] = 0;
    LOG_NODE* pNode = listNodes->getNode(iItem);
    if (!pNode) { ATLASSERT(0); return ret; }

#ifdef _DEBUG
    if (221 == pNode->getCallLine())
        cb = cb;
#endif
    BOOL bCompactView = gSettings.GetCompactView();
    ATLASSERT(!bCompactView || (col == LOG_COL && m_cColumns == 2));
    if (!bCompactView)
    {
        ret = pNode->getListText(&cb, col, iItem);
        ci = 0;
    }
    else
    {
        cb = 0;
        bool funcColFound = false;
        for (int i = 1; i < m_cActualColumns && cb < MAX_BUF_LEN; i++)
        {
            if (FUNC_COL == m_ActualColType[i])
                funcColFound = true;
            if (funcColFound && pNode->isFlow() && LOG_COL == m_ActualColType[i])
                break;
            int c;
            TCHAR* p = pNode->getListText(&c, m_ActualColType[i], iItem);
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
    }

    return ret;
}