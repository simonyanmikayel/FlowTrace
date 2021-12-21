#pragma once

#include "Archive.h"
#include "Helpers.h"
#include "GDI.h"

class CFlowTraceView;
class CLogListInfo;
class CLogListFrame;

struct LINE_BUF
{
	static const int cbMax = 2 * MAX_LOG_LEN - 10;
	CHAR p[cbMax + 10];
	int cb;
};

struct SEL_POINT
{
	SEL_POINT() { iItem = iChar = 0; }
	SEL_POINT(int iItem, int iChar) { this->iItem = iItem; this->iChar = iChar; }
	int iItem, iChar;
	void Clear() { iItem = iChar = 0; }
	SEL_POINT & operator=(const SEL_POINT &o) { iItem = o.iItem; iChar = o.iChar; return *this; }
	bool SEL_POINT::operator==(const SEL_POINT &o) const { return (iItem == o.iItem && iChar == o.iChar); }
	bool SEL_POINT::operator<(const SEL_POINT &o) const { return (iItem < o.iItem || (iItem == o.iItem && iChar < o.iChar)); }
	bool SEL_POINT::operator<=(const SEL_POINT &o) const { return (iItem < o.iItem || (iItem == o.iItem && iChar <= o.iChar)); }
	bool SEL_POINT::operator>(const SEL_POINT &o) const { return (iItem > o.iItem || (iItem == o.iItem && iChar > o.iChar)); }
	bool SEL_POINT::operator>=(const SEL_POINT &o) const { return (iItem > o.iItem || (iItem == o.iItem && iChar >= o.iChar)); }
};

struct LOG_SELECTION
{
	void Clear() { start = end = cur = { 0, 0 }; }
	bool IsEmpty() { return start == end && cur == end; };
	bool Move(int iItem, int iChar, bool extend);
	bool InSelection(int iItem, int iChar);
	bool LOG_SELECTION::operator==(const LOG_SELECTION &o) const { return start == o.start && end == o.end && cur == o.cur; }
	int CurItem() { return cur.iItem; }
	int CurChar() { return cur.iChar; }
	int StartItem() { return start.iItem; }
	int StartChar() { return start.iChar; }
	int EndItem() { return end.iItem; }
	int EndChar() { return end.iChar; }
	void print();
	void SelLogSelection() { Move(start.iItem, 0, false); };
private:
	SEL_POINT start, end, cur;
};

class CLogListView : public CWindowImpl< CLogListView, CListViewCtrl>
{
public:
	CLogListView(CLogListFrame *pPareent, CFlowTraceView* pFlowTraceView, CLogListInfo* pListInfo);
	~CLogListView();

	BEGIN_MSG_MAP(CLogListView)
		MSG_WM_SIZE(OnSize)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
        MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClick)
		MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
	END_MSG_MAP()

	void OnSize(UINT nType, CSize size);
	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnMButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnLButtonDblClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnRButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnKeyUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);

	void ApplySettings(bool fontChanged);
	void Clear();
	void RefreshList();
	void DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rc);
	void ItemPrePaint(int iItem, HDC hdc, RECT rc);
	void UpdateCaret();
	bool HasSelection();
	LOG_NODE* GetSynkNode();
	void CopySelection(bool onlyTraces = false, bool copySpecial = false);
	CHAR* getText(int iItem, int* cBuf = NULL, bool onlyTraces = false, int* cInfo = NULL);
	CHAR* getText(LOG_NODE* pNode, int* cBuf = NULL, bool onlyTraces = false, int* cInfo = NULL);
	void SelectAll();
    int getSelectionItem() { return m_ListSelection.CurItem(); }
    int getSelectionColumn() { return m_ListSelection.CurChar(); }
    void Redraw(int nFirst, int nLast);
	void EnsureTextVisible(int iItem, int startChar, int endChar);
	void ShowItem(DWORD i, bool scrollToMiddle, bool scrollToLeft);
	void ShowNode(LOG_NODE* pNode, bool scrollToMiddle);
	void ShowFirstSyncronised(bool scrollToMiddle);
	void MoveSelectionEx(int iItem, int iChar = 0, bool extend = false, bool ensureVisible = true);
	void SelLogSelection() { m_ListSelection.SelLogSelection(); }
	void OnBookmarks(WORD wID);
	DWORD GetRecCount() { return m_recCount; }
	void ToggleBookmark(DWORD item);
	bool UdjustSelectionOnMouseEven(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

	void SetColumns();
	bool SetColumnLen(int len);
	void MoveSelectionToEnd(bool extend);
	void ClearColumnInfo();
	bool IsWordChar(char c) { return (0 != isalnum(c)) || c == '_'; }

	LIST_COL m_ColType[MAX_COL];
	LIST_COL m_DetailType[MAX_COL];
	int m_ColLen;
	int m_cActualColumns;
	bool m_hasCaret;
	bool m_IsCupture;
	CFlowTraceView* m_pFlowTraceView;
	CLogListFrame *m_pPareent;
	CLogListInfo* m_pListInfo;
	int m_lengthCalculated;
	int m_recCount;
	int m_curBookmark;
	LOG_SELECTION m_ListSelection;
	MemDC wndMemDC;
	MemDC itemMemDC;
	LINE_BUF m_selText;
};


