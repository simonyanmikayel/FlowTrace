#pragma once

#include "Archive.h"
#include "Helpers.h"

class CFlowTraceView;

#ifdef NATIVE_TREE
class CLogTreeView : public CWindowImpl< CLogTreeView, CTreeViewCtrlEx>
#else
class CLogTreeView : public CWindowImpl< CLogTreeView, CListViewCtrl>
#endif
{
public:
  CLogTreeView(CFlowTraceView* pView);
  ~CLogTreeView();

  BEGIN_MSG_MAP(CLogTreeView)
#ifndef NATIVE_TREE
    MSG_WM_SIZE(OnSize)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown); 
#endif
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
  END_MSG_MAP()

  HIMAGELIST m_hTypeImageList;

  void ApplySettings(bool fontChanged);
  void Clear();
  void RefreshTree(bool showAll);
  void CollapseExpandAll(LOG_NODE* pNode, bool expand);

  LRESULT OnRButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
#ifndef NATIVE_TREE
  HIMAGELIST m_hStateImageList;

  void OnSize(UINT nType, CSize size);
  LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);

  LOG_NODE* getTreeNode(int iItem, int* pOffset = NULL);
  void DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rcItem);
  int hitTest(LOG_NODE* pNode, int xPos, int offset);
  void EnsureItemVisible(int iItem);
  void EnsureNodeVisible(LOG_NODE* pNode, bool select = true, bool collapseOthers = false);
  void ShowItem(DWORD i, bool scrollToMiddle);
  void GetNodetPos(HDC hdc, BOOL hasCheckBox, int offset, char* szText, int cbText, int &cxText, int &xStart, int &xEnd);
  LOG_NODE* GetSelectedNode() { return m_pSelectedNode; }
  void CopySelection();
  int ItemByPos(int yPos);
  void SetSelectedNode(LOG_NODE* pNode);

  int m_colWidth, m_rowHeight;
  bool m_Initialised;
  LOG_NODE* m_pSelectedNode;
  HPEN hDotPen;
  HDC m_hdc;
  CSize m_size;
#endif

private:
  void SetColumnLen(int len);

  CFlowTraceView* m_pView;
  DWORD m_recCount;
};