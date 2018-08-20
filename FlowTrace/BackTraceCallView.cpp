#include "stdafx.h"
#include "BackTraceCallView.h"
#include "Settings.h"
#include "Resource.h"
#include "FlowTraceView.h"
#include "Helpers.h"

const int cMaxBuf = 1204 * 64;
const int max_trace_text = 150;
static char pBuf[cMaxBuf + 1];

CBackTraceCallView::CBackTraceCallView(CFlowTraceView* pView)
    : m_pView(pView)
    , m_Initialised(false)
    , c_nodes(0)
    , min_col_width(0)
{
    ClearSelection();
}

CBackTraceCallView::~CBackTraceCallView()
{
}

static const char* szPending = "?";// "pending to resolve call line";

void CBackTraceCallView::OnSize(UINT nType, CSize size)
{
    if (!m_Initialised)
    {
        m_Initialised = true;
        int cColumns = 0;
        InsertColumn(cColumns++, "func", LVCFMT_LEFT, 16, 0, -1, -1); //BACK_TRACE_FN
                                                                      //InsertColumn(cColumns++, "lnk1", LVCFMT_RIGHT, 16, 0, -1, -1); //BACK_TRACE_LNK_CALL
                                                                      //InsertColumn(cColumns++, "lnk2", LVCFMT_RIGHT, 16, 0, -1, -1); //BACK_TRACE_LNK_FUNC
        InsertColumn(cColumns++, "line", LVCFMT_RIGHT, 16, 0, -1, -1); //BACK_TRACE_LINE
        InsertColumn(cColumns++, "source", LVCFMT_LEFT, 16, 0, -1, -1); //BACK_TRACE_SRC
    }
}

int CBackTraceCallView::getSubItemText(int iItem, int iSubItem, char* buf, int cbBuf)
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
            memcpy(buf, " ", cb);
        }
    }
    //else if (iSubItem == BACK_TRACE_LNK_CALL || iSubItem == BACK_TRACE_LNK_FUNC)
    //{
    //  cb = 2;
    //  memcpy(buf, "?", cb);
    //}
    else if (iSubItem == BACK_TRACE_SRC)
    {
        if (pNode->isJava())
        {
            //cb = pNode->getTraceText(buf, min(max_trace_text, cbBuf));
        }
        else if (pNode->isTrace())
        {
            cb = pNode->getTraceText(buf, min(max_trace_text, cbBuf));
        }
        else
        {
            const char* txt;
            if (pNode->PendingToResolveAddr())
            {
                gArchive.resolveAddr(pNode, false);
                txt = szPending;
            }
            else
            {
                txt = ((FLOW_NODE*)pNode)->getCallSrc(gSettings.GetFullSrcPath());
            }
            cb = min((int)strlen(txt), cbBuf);
            memcpy(buf, txt, cb);
        }
    }
    else if (iSubItem == BACK_TRACE_LINE)
    {
        int line = 0;
        if (pNode->isFlow())
        {
            line = ((FLOW_NODE*)pNode)->callLine();
        }
        else if (pNode->isTrace())
        {
            line = ((TRACE_NODE*)pNode)->call_line;
        }
        if (line > 0)
            cb = _snprintf_s(buf, cbBuf, cbBuf, "%d", line);
        else
            cb = _snprintf_s(buf, cbBuf, cbBuf, "?");
    }
    buf[cb] = 0;
    return cb;
}

void CBackTraceCallView::AddTraceNodes(LOG_NODE* pSelectedNode, FLOW_NODE* pFlowNode, DWORD& traceNodeIndex, bool beforeFlowNode)
{
    for (; traceNodeIndex < gArchive.getListedNodes()->Count(); traceNodeIndex++)
    {
        LOG_NODE* pListedNode = gArchive.getListedNodes()->getNode(traceNodeIndex);
        if (pListedNode->isSynchronized(pSelectedNode))
        {
            if (pListedNode != pSelectedNode && pListedNode->isTrace() && pListedNode->parent == pSelectedNode)
            {
                int line = ((TRACE_NODE*)pListedNode)->call_line;
                if (!pFlowNode->p_call_addr_info)
                  gArchive.resolveAddr(pFlowNode, false);
                if (line > 0 && pFlowNode->p_call_addr_info)
                {
                    if ((beforeFlowNode && line <= pFlowNode->p_call_addr_info->line) || (!beforeFlowNode && line >= pFlowNode->p_call_addr_info->line))
                    {
                        nodes[c_nodes] = pListedNode;
                        c_nodes++;
                    }
                    else
                        break;
                }
            }
        }
    }
}

void CBackTraceCallView::UpdateBackTrace(LOG_NODE* pSelectedNode, bool bNested)
{
    if (!m_Initialised)
        return;

    LOG_NODE* pNode = NULL;
    ClearTrace();
    pNode = pSelectedNode->parent ? pSelectedNode : pSelectedNode->getPeer();
    if (bNested)
    {
        if (pSelectedNode->isFlow())
            pNode = pNode->firstChild;
        else
            pNode = pNode->parent->firstChild;
    }
    DWORD traceNodeIndex = 0;
    if (!pNode && pSelectedNode->isFlow())
    {
        AddTraceNodes(pSelectedNode, (FLOW_NODE*)pSelectedNode, traceNodeIndex, true);
    }
    else
    {
        while (pNode && pNode->isInfo() && c_nodes < MAX_BACK_TRACE)
        {
            if (bNested && pNode->isFlow())
                AddTraceNodes(pSelectedNode, (FLOW_NODE*)pNode, traceNodeIndex, true);

            nodes[c_nodes] = pNode;
            c_nodes++;

            if (bNested && pNode->isFlow())
            {
                if (!pNode->nextSibling)
                    AddTraceNodes(pSelectedNode, (FLOW_NODE*)pNode, traceNodeIndex, false);
            }

            if (bNested)
                pNode = pNode->nextSibling;
            else
                pNode = pNode->parent;
        }
    }
    if (c_nodes == 0)
    {
        ClearTrace();
        return;
    }

    HDC hdc = ::CreateCompatibleDC(NULL);
    SelectObject(hdc, gSettings.GetFont());

    SIZE size;
    GetTextExtentPoint32(hdc, " ", 3, &size);
    min_col_width = size.cx;
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

void CBackTraceCallView::ClearTrace()
{
    SetItemCountEx(0, 0);
    DeleteAllItems();
    c_nodes = 0;
    ClearSelection();
}

void CBackTraceCallView::CopySelection()
{
    CopySelection(false);
}

void CBackTraceCallView::CopySelection(bool all)
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
        if (m_sel.pNode)
            cb = getSubItemText(m_sel.iItem, m_sel.iSubItem, pBuf, cMaxBuf);
    }
    Helpers::CopyToClipboard(m_hWnd, pBuf, cb);
}

void CBackTraceCallView::ItemPrePaint(DWORD iItem, HDC hdc, RECT rc)
{
}

LRESULT CBackTraceCallView::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & bHandled)
{
    bHandled = FALSE;
    if (m_sel.pNode)
        RedrawItems(m_sel.iItem, m_sel.iItem);
    return 0;
}

LRESULT CBackTraceCallView::OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & bHandled)
{
    bHandled = FALSE;
    if (m_sel.pNode)
        RedrawItems(m_sel.iItem, m_sel.iItem);
    return 0;
}

void CBackTraceCallView::DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rcItem)
{
    POINT  pt = { rcItem.left, rcItem.top };
    COLORREF old_textColor = ::GetTextColor(hdc);
    COLORREF old_bkColor = ::GetBkColor(hdc);
    int old_bkMode = ::GetBkMode(hdc);

    ::SetBkMode(hdc, OPAQUE);

    int cb = getSubItemText(iItem, iSubItem, pBuf, cMaxBuf);
    if (cb)
    {
        //if (!selected) { pBuf[0] = '1'; pBuf[1] = '2'; pBuf[2] = '3'; pBuf[3] = '4'; pBuf[4] = '5';}//memset(pBuf, 'W', cb);
        //stdlog("iItem %d, iSubItem %d, cb %d, pt.x %d, w=%d, %s\n", iItem, iSubItem, cb, pt.x, nodes[iItem].subItemColWidth[iSubItem], pBuf);
        TextOut(hdc, pt.x, pt.y, pBuf, cb);
    }

    if (iItem == m_sel.iItem && iSubItem == m_sel.iSubItem)
    {
        //::SetTextColor(hdc, gSettings.SelectionTxtColor());
        //::SetBkColor(hdc, gSettings.SelectionBkColor(GetFocus() == m_hWnd));
        RECT rcText = { rcItem.left - 2, rcItem.top, rcItem.right - min_col_width + 10, rcItem.bottom };
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

    ::SetTextColor(hdc, old_textColor);
    ::SetBkColor(hdc, old_bkColor);
    ::SetBkMode(hdc, old_bkMode);
}

LRESULT CBackTraceCallView::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    Helpers::OnLButtonUp(m_sel.pNode, wParam, lParam);
    return 0;
}

LRESULT CBackTraceCallView::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    SetSelectionOnMouseEven(uMsg, wParam, lParam);
    Helpers::OnLButtonDoun(wParam, lParam);
    return 0;
}

LRESULT CBackTraceCallView::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    SetSelectionOnMouseEven(uMsg, wParam, lParam);

    bHandled = TRUE;

    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);
    int cMenu = 0;
    DWORD dwFlags;
    POINT pt = { xPos, yPos };
    ClientToScreen(&pt);
    HMENU hMenu = CreatePopupMenu();
    Helpers::AddCommonMenu(m_sel.pNode, hMenu, cMenu);
    dwFlags = MF_BYPOSITION | MF_STRING;
    InsertMenu(hMenu, cMenu++, dwFlags, ID_EDIT_COPY_ALL, _T("Copy All"));
    dwFlags = MF_BYPOSITION | MF_STRING;
    if (m_sel.pNode == NULL)
        dwFlags |= MF_DISABLED;
    InsertMenu(hMenu, cMenu++, dwFlags, ID_EDIT_COPY, _T("&Copy\tCtrl+C"));

    //MENUITEMINFO mii;
    //mii.cbSize = sizeof(MENUITEMINFO);
    //mii.fMask = MIIM_BITMAP;
    //GetMenuItemInfo(hMenu, 0, TRUE, &mii);
    //mii.hbmpItem = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SYNC));
    //SetMenuItemInfo(hMenu, 0, TRUE, &mii);



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
        m_pView->SyncViews();
    }
    else if (nRet == ID_SHOW_CALL_IN_ECLIPSE)
    {
        Helpers::ShowInIDE(m_sel.pNode, true);
    }
    else if (nRet == ID_SHOW_FUNC_IN_ECLIPSE)
    {
        Helpers::ShowInIDE(m_sel.pNode, false);
    }
    else if (nRet == ID_VIEW_NODE_DATA)
    {
        Helpers::ShowNodeDetails(m_sel.pNode);
    }

    return 0;
}

LRESULT CBackTraceCallView::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    bHandled = TRUE;
    if (!m_sel.pNode)
        return 0;

    int curSelItem = m_sel.iItem;
    int curSelSubItem = m_sel.iSubItem;
    int newSelItem = m_sel.iItem;
    int newSelSubItem = m_sel.iSubItem;

    switch (wParam)
    {
    case VK_HOME:       // Home 
        newSelSubItem = 0;
        break;
    case VK_RETURN:        // End 
        newSelItem++;
        break;
    case VK_END:        // End 
        newSelSubItem = BACK_TRACE_LAST_COL - 1;
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
            newSelItem -= count;
        else
            newSelItem += count;
    }
    break;
    case VK_LEFT:       // Left arrow 
        newSelSubItem--;
        break;
    case VK_RIGHT:      // Right arrow 
        newSelSubItem++;
        break;
    case VK_UP:         // Up arrow 
        newSelItem--;
        break;
    case VK_DOWN:       // Down arrow 
        newSelItem++;
        break;
    }
    if (newSelItem < 0)
        newSelItem = 0;
    if (newSelSubItem < 0)
        newSelSubItem = 0;
    if (newSelItem >= c_nodes)
        newSelItem = c_nodes - 1;
    if (newSelSubItem >= BACK_TRACE_LAST_COL)
        newSelSubItem = BACK_TRACE_LAST_COL - 1;

    int cb = getSubItemText(newSelItem, newSelSubItem, pBuf, cMaxBuf);
    while (cb == 0 && newSelSubItem < BACK_TRACE_LAST_COL - 1)
    {
        newSelSubItem++;
        cb = getSubItemText(newSelItem, newSelSubItem, pBuf, cMaxBuf);
    }
    while (cb == 0 && newSelSubItem > 0)
    {
        newSelSubItem--;
        cb = getSubItemText(newSelItem, newSelSubItem, pBuf, cMaxBuf);
    }
    if (curSelItem != newSelItem || curSelSubItem != newSelSubItem)
    {
        RedrawItems(curSelItem, curSelItem);
        RedrawItems(newSelItem, newSelItem);
    }
    SetSelection(newSelItem, newSelSubItem);

    return 0;
}

void CBackTraceCallView::ClearSelection()
{
    m_sel.iItem = -1;
    m_sel.iSubItem = -1;
    m_sel.pNode = NULL;
}

void CBackTraceCallView::SetSelection(int iItem, int iSubItem)
{
    int curSelItem = m_sel.iItem;
    int curSelSubItem = m_sel.iSubItem;
    if (iItem >= 0 && iSubItem >= 0 && iItem < c_nodes)
    {
        m_sel.iItem = iItem;
        m_sel.iSubItem = iSubItem;
        m_sel.pNode = nodes[iItem];
    }
    int newSelItem = m_sel.iItem;
    int newSelSubItem = m_sel.iSubItem;
    if (curSelItem != newSelItem || curSelSubItem != newSelSubItem)
    {
        if (curSelItem >= 0)
            RedrawItems(curSelItem, curSelItem);
        if (newSelItem >= 0)
        {
            RedrawItems(newSelItem, newSelItem);
            EnsureVisible(newSelItem, FALSE);
        }
    }
}

void CBackTraceCallView::SetSelectionOnMouseEven(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        if (m_sel.iItem != ht.iItem || m_sel.iSubItem != ht.iSubItem)
        {
            int cb = getSubItemText(ht.iItem, ht.iSubItem, pBuf, cMaxBuf);
            if (cb > 0)
            {
                if (m_sel.iItem >= 0)
                {
                    RedrawItems(m_sel.iItem, m_sel.iItem);
                }
                SetSelection(ht.iItem, ht.iSubItem);
            }
        }
    }
}
