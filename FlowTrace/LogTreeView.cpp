#include "stdafx.h"
#include "Resource.h"
#include "LogTreeView.h"
#include "FlowTraceView.h"
#include "Settings.h"
#include "DlgProgress.h"
#include "MainFrm.h"

enum STATE_IMAGE { STATE_IMAGE_COLAPSED, STATE_IMAGE_EXPANDED, STATE_IMAGE_CHECKED, STATE_IMAGE_UNCHECKE };
static const int min_colWidth = 16;

CLogTreeView::CLogTreeView(CFlowTraceView* pView)
    : m_pView(pView)
    , m_recCount(0)
    , m_pSelectedNode(0)
    , m_rowHeight(1)
{
    m_hTypeImageList = ImageList_Create(16, 16, ILC_MASK, 1, 0);
    ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_ROOT))); //0
    ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_APP))); //1
    ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_THREAD))); //2
    ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_PAIRED))); //3
    ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_ENTER))); //4
    ImageList_AddIcon(m_hTypeImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_TREE_EXIT))); //5  

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
}


CLogTreeView::~CLogTreeView()
{
    DeleteDC(m_hdc);
}

void CLogTreeView::ApplySettings(bool fontChanged)
{
    if (fontChanged)
    {
        SetFont(gSettings.GetFont());
        SelectObject(m_hdc, gSettings.GetFont());
    }
    Invalidate();
}

void CLogTreeView::Clear()
{
    m_recCount = 0;
    m_pSelectedNode = NULL;
    SetItemCountEx(0, 0);
    m_colWidth = min_colWidth;
    SetColumnWidth(0, m_colWidth);
    Helpers::UpdateStatusBar();
}

LRESULT CLogTreeView::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    bHandled = TRUE;

    if (GetFocus() != *this)
        SetFocus();

    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);

    LOG_NODE* pNode = NULL;
    int iItem = ItemByPos(yPos);
    if (iItem >= 0)
    {
        pNode = getTreeNode(iItem);
    }

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
            EnsureItemVisible(iItem);
        }

        bool disable;
        int cMenu = 0;
        POINT pt = { xPos, yPos };
        ClientToScreen(&pt);
        HMENU hMenu = CreatePopupMenu();
        Helpers::AddCommonMenu(pNode, hMenu, cMenu);

        Helpers::AddMenu(hMenu, cMenu, ID_TREE_COLLAPSE_ALL, _T("Collapse All"));

        disable = (!pNode->lastChild);
        Helpers::AddMenu(hMenu, cMenu, ID_TREE_EXPAND_ALL, _T("Expand All"), disable);

		disable = (pNode->data_type != ROOT_DATA_TYPE && pNode->data_type != APP_DATA_TYPE);
		Helpers::AddMenu(hMenu, cMenu, ID_TREE_CHECK_ALL, _T("Show All Children"), disable);
		Helpers::AddMenu(hMenu, cMenu, ID_TREE_UNCHECK_ALL, _T("Hide All Children"), disable);

		Helpers::AddMenu(hMenu, cMenu, ID_TREE_SHOW_THIS_THREAD, _T("Show only this thread\tCtrl+D"), false);
		Helpers::AddMenu(hMenu, cMenu, ID_TREE_SHOW_THIS_APP, _T("Show only this app\tCtrl+P"), false);
		Helpers::AddMenu(hMenu, cMenu, ID_TREE_SHOW_ALL, _T("Show All\tCtrl+L"), false);
		
		InsertMenu(hMenu, cMenu++, MF_BYPOSITION | MF_SEPARATOR, 0, _T(""));

		disable = (!pNode->parent || !pNode->parent->firstChild || pNode->parent->firstChild == pNode->parent->lastChild);
        Helpers::AddMenu(hMenu, cMenu, ID_TREE_FIRST_SIBLING, _T("First Sibling\tCtrl+UP"), disable);

        disable = (!pNode->prevSibling);
        Helpers::AddMenu(hMenu, cMenu, ID_TREE_PREV_SIBLING, _T("Previous Sibling\tShift+UP"), disable);

        disable = (!pNode->nextSibling);
        Helpers::AddMenu(hMenu, cMenu, ID_TREE_NEXT_SIBLING, _T("Next Sibling\tShift+UP"), disable);

        disable = (!pNode->parent || !pNode->parent->lastChild || pNode->parent->firstChild == pNode->parent->lastChild);
        Helpers::AddMenu(hMenu, cMenu, ID_TREE_LAST_SIBLING, _T("Last Sibling\tCtrl+DOWN"), disable);
            
        InsertMenu(hMenu, cMenu++, MF_BYPOSITION | MF_SEPARATOR, 0, _T(""));

        Helpers::AddMenu(hMenu, cMenu, ID_EDIT_COPY, _T("&Copy\tCtrl+C"));

        UINT nRet = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, 0);
        DestroyMenu(hMenu);
        if (nRet == ID_TREE_EXPAND_ALL)
        {
            CollapseExpandAll(pNode, true);
        }
        else if (nRet == ID_TREE_COLLAPSE_ALL)
        {
            CollapseExpandAll(pNode, false);
        }
		else if (nRet == ID_TREE_CHECK_ALL)
		{
			CheckAll(pNode, true);
		}		
		else if (nRet == ID_TREE_UNCHECK_ALL)
		{
			CheckAll(pNode, false);
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
		else if (nRet == ID_EDIT_COPY)
        {
            ::SendMessage(hwndMain, WM_COMMAND, ID_EDIT_COPY, 0);
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
        else if (nRet == ID_TREE_FIRST_SIBLING)
        {
            _OnKeyDown(VK_UP, false, true);
        }
        else if (nRet == ID_TREE_PREV_SIBLING)
        {
            _OnKeyDown(VK_UP, true, false);
        }
        else if (nRet == ID_TREE_NEXT_SIBLING)
        {
            _OnKeyDown(VK_DOWN, true, false);
        }
        else if (nRet == ID_TREE_LAST_SIBLING)
        {
            _OnKeyDown(VK_DOWN, false, true);
        }

        //stdlog("%u\n", GetTickCount());
    }
    return 0;
}

void CLogTreeView::CollapseExpandAll(LOG_NODE* pNode, bool expand)
{
    pNode->CollapseExpandAll(expand);
    SetItemCount(gArchive.getRootNode()->GetExpandCount() + 1);
    RedrawItems(0, gArchive.getRootNode()->GetExpandCount());
}

void CLogTreeView::CheckAll(LOG_NODE* pNode, bool check)
{
	if (pNode->CheckAll(check))
	{
		::PostMessage(hwndMain, WM_UPDATE_FILTER, 0, 0);
		RedrawItems(0, gArchive.getRootNode()->GetExpandCount());
	}
}

void CLogTreeView::CopySelection()
{
    if (!m_pSelectedNode)
        return;
    int cbText;
    char* szText;
    szText = m_pSelectedNode->getTreeText(&cbText, false);
    Helpers::CopyToClipboard(m_hWnd, szText, cbText);
}

void CLogTreeView::RedrawAll()
{
	RedrawItems(0, gArchive.getRootNode()->GetExpandCount());
}

void CLogTreeView::RefreshTree(bool showAll)
{
    DWORD newCount = gArchive.getNodeCount();
    if (newCount <= 1)
        return;//only root node

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

    if (!m_Initialised)
        return;

    int prevExpanded = m_recCount ? gArchive.getRootNode()->GetExpandCount() : -1;
    int firstAffected = -1;
    //int firstRow = prevExpanded ? 0 : GetTopIndex();
    //int lastRow = m_size.cy / m_rowHeight + 1;
    gArchive.getRootNode()->CalcLines();

    //add new logs
    for (DWORD i = m_recCount; i < newCount; i++)
    {
        LOG_NODE* pNode = gArchive.getNode(i);

        if (!pNode->isTrace())
        {
            LOG_NODE* p = pNode->isTreeNode() ? pNode : ((FLOW_NODE*)pNode)->getPeer();
            //if(p) stdlog("pathExpanded = %d line = %d\n", p->parent->pathExpanded, p->parent->line);
            if (p && p->parent && p->parent->pathExpanded)
            {
                if (p->parent->childCount > 0 && !p->parent->hasNodeBox)
                {
                    p->parent->hasNodeBox = 1;
                    if (firstAffected == -1 || firstAffected > p->parent->posInTree)
                        firstAffected = p->parent->posInTree;
                }
                if (p->parent->expanded)
                {
                    if (firstAffected == -1 || firstAffected > p->posInTree)
                        firstAffected = p->posInTree;
                }
            }
        }
    }

    m_recCount = newCount;
    if (prevExpanded != gArchive.getRootNode()->GetExpandCount())
    {
        SetItemCountEx(gArchive.getRootNode()->GetExpandCount() + 1, LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);//LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL
    }
    if (firstAffected > -1)
    {
        RedrawItems(firstAffected, gArchive.getRootNode()->GetExpandCount());
    }

    Helpers::UpdateStatusBar();

}

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
    pNode = gArchive.getRootNode();
    ATLASSERT(gArchive.getRootNode()->expanded || iItem == 0);

    while (iItem != pNode->posInTree)
    {
        //stdlog("\tline %d offset = %d %s\n", pNode->line, offset, pNode->getTreeText());
        //if (pNode->nextSibling && (pNode->line + pNode->GetExpandCount()) < iItem)
        if (pNode->nextSibling && pNode->nextSibling->posInTree <= iItem)
        {
            while (pNode->nextChank && pNode->nextChank->posInTree <= iItem)
            {
                //stdlog("\t1 index = %d line %d offset = %d %s\n", pNode->index, pNode->line, offset, pNode->getTreeText());
                pNode = pNode->nextChank;
                //stdlog("\t\t1 index = %d line %d offset = %d %s\n", pNode->index, pNode->line, offset, pNode->getTreeText());
            }
            while (pNode->nextSibling && pNode->nextSibling->posInTree <= iItem)
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
    len = max(rc.right, (long)len) + 16;
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
    if (!pNode) { ATLASSERT(0); return; }

    ShowItem(iItem, false);

    RECT rcClient;// , rcItem;
    GetClientRect(&rcClient);
    //ListView_GetSubItemRect(m_hWnd, iItem, 0, LVIR_BOUNDS, &rcItem);
    int cxClient = rcClient.right;// -rcItem.left;
    //GetSubItemRect(iItem, 0, LVIR_BOUNDS, &rcItem);

    int cbText;
    char* szText = pNode->getTreeText(&cbText, false);

    int cxText, xStart, xEnd;
    GetNodetPos(m_hdc, pNode->hasCheckBox, offset, szText, cbText, cxText, xStart, xEnd);

    SetColumnLen(xEnd);

    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_POS;
    GetScrollInfo(SB_HORZ, &si);
    //stdlog("cxClient: %d, xStart: %d, xEnd: %d, si.nPos: %d\n", cxClient, xStart, xEnd, si.nPos);

    int cxRight = 0, cxleft = 0;
    if (xEnd > cxClient + si.nPos - 16)
    {
        cxRight = xEnd - (si.nPos + cxClient - 16);
        xStart -= cxRight;
        //stdlog("cxRight: %d, xStart: %d\n", cxRight, xStart);
    }
    if (xStart < si.nPos)
    {
        cxleft = xStart - si.nPos;
        //stdlog("cxleft: %d\n", cxleft);
    }
    if (cxRight + cxleft)
    {
        SIZE size = { cxRight + cxleft, 0 };
        Scroll(size);
    }
    //stdlog("pNode = %p iItem = %d offset = %d cxleft = %d cxleft = %d xStart = %d cxText = %d xEnd = %d si.nPos = %d si.nMin = %d si.nMax = %d rcClient.left = %d rcClient.right = %d rcItem.left = %d rcItem.right = %d szText = %s\n", pNode, iItem, offset, cxleft, cxleft, xStart, cxText, xEnd, si.nPos, si.nMin, si.nMax, rcClient.left, rcClient.right, rcItem.left, rcItem.right, szText);
}

void CLogTreeView::EnsureNodeVisible(LOG_NODE* pNode, bool select, bool collapseOthers)
{
    if (collapseOthers)
        CollapseExpandAll(gArchive.getRootNode(), false);

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
    SetItemCount(gArchive.getRootNode()->GetExpandCount() + 1);

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
}

LRESULT CLogTreeView::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    bHandled = TRUE;
    bool bShiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool bCtrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    return _OnKeyDown(wParam, bShiftPressed, bCtrlPressed);
}

LRESULT CLogTreeView::_OnKeyDown(WPARAM virt_key, bool bShiftPressed, bool bCtrlPressed)
{

    if (m_pSelectedNode == NULL)
        return 0;

    int iCurSelected = m_pSelectedNode->GetPosInTree();
    int iNewSelected = iCurSelected;

    bool redraw = true;

    switch (virt_key)
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
        if (virt_key == VK_PRIOR)
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
            SetItemCount(gArchive.getRootNode()->GetExpandCount() + 1);
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
            SetItemCount(gArchive.getRootNode()->GetExpandCount() + 1);
        }
        else
            iNewSelected++;
    }
    break;
    case VK_UP:         // Up arrow 
    {
        if (bCtrlPressed)
        {
            LOG_NODE* pNode = m_pSelectedNode;
            if (pNode && pNode->parent && pNode->parent->firstChild)
                iNewSelected = pNode->parent->firstChild->GetPosInTree();
        }
        else if (bShiftPressed)
        {
            LOG_NODE* pNode = m_pSelectedNode;
            if (pNode && pNode->prevSibling)
                iNewSelected = pNode->prevSibling->GetPosInTree();
        }
        else
        {
            iNewSelected--;
        }
    }
    break;
    case VK_DOWN:       // Down arrow 
    {
        if (bCtrlPressed)
        {
            LOG_NODE* pNode = m_pSelectedNode;
            if (pNode && pNode->parent && pNode->parent->lastChild)
                iNewSelected = pNode->parent->lastChild->GetPosInTree();
        }
        else if (bShiftPressed)
        {
            LOG_NODE* pNode = m_pSelectedNode;
            if (pNode && pNode->nextSibling)
                iNewSelected = pNode->nextSibling->GetPosInTree();
        }
        else
        {
            iNewSelected++;
        }
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

LRESULT CLogTreeView::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    return 0;
}

LRESULT CLogTreeView::OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    return OnLButtonDown(uMsg, wParam, lParam, bHandled);
}

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
        xPos -= rcItem.left;
        if (hitTest(pNode, xPos, offset) == TVHT_ONITEMBUTTON)
        {
            pNode->CollapseExpand(!pNode->expanded);
            SetItemCount(gArchive.getRootNode()->GetExpandCount() + 1);
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
            EnsureItemVisible(iItem);
        }
        Helpers::OnLButtonDoun(m_pSelectedNode, wParam, lParam);
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

LRESULT CLogTreeView::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & bHandled)
{
    bHandled = FALSE;
    if (m_pSelectedNode)
    {
        int iCurSelected = m_pSelectedNode->GetPosInTree();
        RedrawItems(iCurSelected, iCurSelected);
    }
    return 0;
}

LRESULT CLogTreeView::OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & bHandled)
{
    bHandled = FALSE;
    if (m_pSelectedNode)
    {
        int iCurSelected = m_pSelectedNode->GetPosInTree();
        RedrawItems(iCurSelected, iCurSelected);
    }
    return 0;
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


    RECT rcFrame = rcItem;
    rcFrame.left = left - 2;
    rcFrame.right = rcFrame.left + cxText + 4;
    if (gSyncronizedNode == pNode)
    {
        CBrush brush1;
        brush1.CreateSolidBrush(RGB(0, 255, 128)); //gSettings.GetSyncColor()
        FillRect(hdc, &rcFrame, brush1);

        ::SetBkMode(hdc, TRANSPARENT);
    }
    else
    {
        ::SetBkMode(hdc, OPAQUE);
    }

	if (pNode->isJava()) {
			::SetTextColor(hdc, gSettings.InfoTextColorNative());
	}
	else {
		::SetTextColor(hdc, gSettings.InfoTextColorNative());
	}
	TextOut(hdc, left, top, szText, cbText);

	if (m_pSelectedNode == pNode)
    {
        if (GetFocus() == m_hWnd)
        {
            CBrush brush2;
            brush2.CreateSolidBrush(gSettings.SelectionBkColor());
            FrameRect(hdc, &rcFrame, brush2);
        }
        else
        {
            DrawFocusRect(hdc, &rcFrame);
        }
        //::SetTextColor(hdc, gSettings.SelectionTxtColor());
    }

    int i = 0, x = left - ICON_OFFSET - ICON_LEN / 2 - 5;
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
