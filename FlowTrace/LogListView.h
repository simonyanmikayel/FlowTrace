#pragma once

#include "Archive.h"
#include "Helpers.h"

class CFlowTraceView;

struct SEL_POINT
{
  SEL_POINT() { iItem = iChar = 0; }
  SEL_POINT(int iItem, int iChar){ this->iItem = iItem; this->iChar = iChar; }
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
private:
  SEL_POINT start, end, cur;
};

struct LIST_SELECTION
{
  LIST_SELECTION(){ Clear(); }
  void Clear() { logSelection.Clear(); column = ICON_COL; iItem = 0; };
  void SelLogSelection() { if (!IsLogSelection()) { column = LOG_COL; logSelection.Move(iItem, 0, false); } };
  bool IsEmpty() { return IsLogSelection() ? logSelection.IsEmpty() : column == ICON_COL; };
  bool IsLogSelection() { return column == LOG_COL; }
  LOG_SELECTION& getLogSelection() { ATLASSERT(IsLogSelection()); return logSelection; }
  int GetItem() { return IsLogSelection() ? logSelection.CurItem() : iItem; }
  int GetLogSelectionItem() { return logSelection.CurItem(); }
  int GetInfoSelectionItem() { return iItem; }
  void SetItem(int i){ ATLASSERT(!IsLogSelection()); iItem = i; }
  void SetColumn(LIST_COL col){ column = col; }
  LIST_COL GetColumn(){ return column; }
private:
  LOG_SELECTION logSelection;
  LIST_COL column;
  int iItem;
};

class CLogListView : public CWindowImpl< CLogListView, CListViewCtrl>
{
public:
  CLogListView(CFlowTraceView* pView);
  ~CLogListView();

  BEGIN_MSG_MAP(CLogListView)
    //MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MSG_WM_SIZE(OnSize)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClick)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown);
    MESSAGE_HANDLER(WM_KEYUP, OnKeyUp);
  END_MSG_MAP()

  void OnSize(UINT nType, CSize size);
  LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnLButtonDblClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnRButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnKeyUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);

  void ApplySettings(bool fontChanged);
  void Clear();
  void RefreshList(bool redraw);
  void DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rc);
  void ItemPrePaint(int iItem, HDC hdc, RECT rc);
  LIST_COL getGolumn(int iSubItem) { return (LIST_COL)m_ColType[iSubItem]; }
  int getSubItem(LIST_COL col) { return m_ColSubItem[col]; }
  void UpdateCaret();
  bool HasSelection();
  LOG_NODE* GetSynkNode();
  void CopySelection();
  TCHAR* getText(int iItem, int* cBuf = NULL, LIST_COL col = LOG_COL, int* cInfo = NULL);
  void SelectAll();
  int getSelectionItem() { return m_ListSelection.GetItem(); }
  void Redraw(int nFirst, int nLast);
  void EnsureTextVisible(int iItem, int startChar, int endChar);
  void ShowItem(DWORD i, bool scrollToMiddle, bool scrollToLeft);
  void ShowNode(LOG_NODE* pNode, bool scrollToMiddle);
  void ShowFirstSyncronised(bool scrollToMiddle);
  void MoveSelectionEx(int iItem, int iChar = 0, bool extend = false, bool ensureVisible = true);
  void SelLogSelection() { m_ListSelection.SelLogSelection(); }
  bool IsLogSelection() { return m_ListSelection.IsLogSelection(); }
  void OnBookmarks(WORD wID);

private:

  void SetColumns();
  void SetColumnLen(LIST_COL col, int len);
  void MoveSelectionToEnd(bool extend);
  void ClearColumnInfo();
  void AddColumn(TCHAR* szHeader, LIST_COL col);
  bool UdjustSelectionOnMouseEven(UINT uMsg, WPARAM wParam, LPARAM lParam);
  bool IsWordChar(char c) { return (0 != isalnum(c)) || c == '_'; }
  LOG_SELECTION& logSelection() { return m_ListSelection.getLogSelection(); }
  void ToggleBookmark(DWORD item);

  LIST_COL m_ColType[MAX_COL];
  LIST_COL m_ActualColType[MAX_COL];
  int m_ColSubItem[MAX_COL];
  int m_ColLen[MAX_COL];
  int m_cColumns, m_cActualColumns;
  int m_LogCol;
  bool m_hasFocus;
  bool m_IsCupture;
  CFlowTraceView* m_pView;
  HDC m_hdc;
  HIMAGELIST m_hListImageList;
  int m_lengthCalculated;
  DWORD m_recCount;
  int m_curBookmark;
  LIST_SELECTION m_ListSelection;
};


