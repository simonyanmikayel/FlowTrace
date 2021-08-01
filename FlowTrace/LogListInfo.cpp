#include "stdafx.h"
#include "Resource.h"
#include "LogListFrame.h"
#include "LogListInfo.h"
#include "Settings.h"
#include "Gdi.h"

static int ICON_INDEX_LIST_ENTER;
static int ICON_INDEX_LIST_EXIT;
static int ICON_INDEX_LIST_TRACE_SEL;
static int ICON_INDEX_BOOKMARK;
static int ICON_INDEX_SYNC;

CLogListInfo::CLogListInfo(CLogListFrame* pView, CLogListView *pLogListView) 
	: m_pParent(pView)
	, m_pLogListView(pLogListView)
	, m_recCount(0)
	, m_nnWidth(0)
	, m_nnDigits(0)
{
	m_hListImageList = ImageList_Create(16, 16, ILC_MASK, 1, 0);
	ICON_INDEX_LIST_ENTER = ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_ENTER)));
	ICON_INDEX_LIST_EXIT = ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_EXIT)));
	ICON_INDEX_LIST_TRACE_SEL = ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_LIST_TRACE_SEL)));
	ICON_INDEX_BOOKMARK = ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_BOOKMARK)));
	ICON_INDEX_SYNC = ImageList_AddIcon(m_hListImageList, LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDI_ICON_SYNC)));
}

CLogListInfo::~CLogListInfo()
{
}

LRESULT CLogListInfo::OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & bHandled)
{
	bHandled = TRUE;
	return 0;
}

LRESULT CLogListInfo::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & bHandled)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(&ps);

	char tmp[32];
	RECT rcClient, rcItem;
	GetClientRect(&rcClient);
	int cx = rcClient.right - rcClient.left;
	int cy = rcClient.bottom - rcClient.top;
	int selItem = m_pLogListView->getSelectionItem();
	memDC.CreateCompatibleBitmap(hdc, cx, cy);
	memDC.EraseBackground();
	memDC.SetBkMode(TRANSPARENT);

	int nFirst = m_pLogListView->GetTopIndex();
	int nLast = nFirst + m_pLogListView->GetCountPerPage();
	if (m_recCount && nLast > 0)
	{
		if (nLast > m_recCount)
			nLast = m_recCount;
		RECT rcSel = { 0, 0, 0, 0 };
		for (int iItem = nFirst; iItem < nLast; iItem++)
		{
			LOG_NODE* pNode = gArchive.getListedNodes()->getNode(iItem);
			if (!pNode)
			{
				ATLASSERT(0);
				break;
			}
			m_pLogListView->GetItemRect(iItem, &rcItem, LVIR_BOUNDS);
			int y = rcItem.top + (rcItem.bottom - rcItem.top) / 2 - ICON_LEN / 2;
			int x = 0;

			if (pNode && pNode->isSynchronized(gSyncronizedNode))
			{
				ImageList_Draw(m_hListImageList, ICON_INDEX_LIST_TRACE_SEL, memDC, x, y, ILD_NORMAL);
			}
			if (pNode->bookmark)
			{
				ImageList_Draw(m_hListImageList, ICON_INDEX_BOOKMARK, memDC, x, y, ILD_NORMAL);
				TCHAR szText[20];
				int cbText = sprintf_s(szText, "%d", pNode->bookmark);
				SIZE  textSize;
				//stdlog("szText [%s] %d %d \n", szText, cbText, strlen(szText));
				GetTextExtentExPoint(memDC, szText, cbText, 0, NULL, NULL, &textSize);

				int y1 = rcItem.top + (rcItem.bottom - rcItem.top) / 2 - textSize.cy / 2;
				int x1 = ICON_LEN / 2 - textSize.cx / 2;
				SetTextAlign(memDC, TA_LEFT);
				::SetTextColor(memDC, RGB(64, 128, 255));
				TextOut(memDC, x1, y1, szText, cbText);
			}
			if (pNode->isFlow())
			{
				if (((FLOW_NODE*)pNode)->isEnter())
					ImageList_Draw(m_hListImageList, ICON_INDEX_LIST_ENTER, memDC, x, y, ILD_NORMAL);
				else
					ImageList_Draw(m_hListImageList, ICON_INDEX_LIST_EXIT, memDC, x, y, ILD_NORMAL);
			}

			x += m_nnWidth - INFO_TEXT_MARGIN;
			int c = sprintf_s(tmp, "%d", iItem + 1);
			SetTextAlign(memDC, TA_RIGHT);
			if (pNode->isJava())
				::SetTextColor(memDC, gSettings.InfoTextColorAndroid());
			else if (pNode->isSerial())
				::SetTextColor(memDC, gSettings.InfoTextColorSerial());
			else
				::SetTextColor(memDC, gSettings.InfoTextColorNative());
			TextOut(memDC, x, rcItem.top, tmp, c);

			if (selItem == iItem)
			{
				rcSel = rcItem;
			}
		}

		if (rcSel.bottom)
		{
			rcSel.left = -1;
			rcSel.right = m_nnWidth + 1;
			FrameRect(memDC, &rcSel, Gdi::SelectionBrush);
		}

	}

	::BitBlt(hdc, 0, 0, cx, cy, memDC, 0, 0, SRCCOPY);
	EndPaint(&ps);
	bHandled = TRUE;
	return 0;
}

LRESULT CLogListInfo::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	int xPos = GET_X_LPARAM(lParam);
	int yPos = GET_Y_LPARAM(lParam);
	LVHITTESTINFO ht = { 0 };	
	ht.pt.x = xPos;
	ht.pt.y = yPos;
	ht.iItem = -1;
	m_pLogListView->SubItemHitTest(&ht);
	//stdlog("xPos = %d yPos = %d flags = %d ht.iItem = %d\n", xPos, yPos, ht.flags, ht.iItem);
	if ((ht.iItem >= 0)) //&& (LVHT_ONITEM & ht.flags)
	{
		if (xPos <= ICON_LEN)
			m_pLogListView->ToggleBookmark(ht.iItem);
		lParam &= 0xffff0000;
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		m_pLogListView->UdjustSelectionOnMouseEven(uMsg, wParam, lParam);
	}
	bHandled = TRUE;
	return 0;
}

void CLogListInfo::SetRecCount(int recCount)
{
	m_recCount = recCount;
	int nnDigits = 0;
	int c = recCount;
	while (c) {
		c /= 10;
		nnDigits++;
	}

	if (nnDigits != m_nnDigits)
	{
		m_nnDigits = nnDigits;
		char* sz = "000000000000";
		if (nnDigits > 12)
			nnDigits = 12;
		SIZE  size;
		GetTextExtentPoint32(memDC, sz, nnDigits, &size);
		m_nnWidth = ICON_LEN + 2*INFO_TEXT_MARGIN + size.cx;
		m_pParent->SetChiledPos();
	}
}

void CLogListInfo::ApplySettings(bool fontChanged)
{
	if (fontChanged)
	{
		SetFont(gSettings.GetFont());
		SelectObject(memDC, gSettings.GetFont());
	}
	memDC.SetBkColor(gSettings.LogListInfoBkColor());
}

int CLogListInfo::GetWidth()
{
	return m_nnWidth;
}
