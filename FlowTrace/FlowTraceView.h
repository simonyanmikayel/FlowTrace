// FlowTraceView.h : interface of the CFlowTraceView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Archive.h"
#include "LogListView.h"
#include "LogTreeView.h"

extern HWND hWndStatusBar;

class CFlowTraceView : public CWindowImpl<CFlowTraceView>
{
public:
	DECLARE_WND_CLASS(NULL)


  CFlowTraceView();
	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CFlowTraceView)
    MSG_WM_CREATE(OnCreate)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, OnPositionChanging)

    NOTIFY_CODE_HANDLER(LVN_ENDSCROLL, OnLvnEndScroll)
#ifdef NATIVE_TREE
    NOTIFY_CODE_HANDLER(TVN_GETDISPINFO, OnTvnGetDispInfo)
#endif
    NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
  END_MSG_MAP()

  LRESULT OnCreate(LPCREATESTRUCT lpcs);
  LRESULT OnNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnPositionChanging(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnLvnEndScroll(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
#ifdef NATIVE_TREE
  LRESULT OnTvnGetDispInfo(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
#endif
  LRESULT OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

  void SyncViews();
  void ShowTreeView(bool show);
  void ShowStackView(bool show);
  void ClearLog();
  void ApplySettings(bool fontChanged);
  void SetChildPos(int cx, int cy);
  LOG_NODE* selectedNode() { return m_selectedNode; }
  CLogListView& list() { return m_wndListView; }
  CLogTreeView& tree() { return m_wndTreeView; }
  int GetVertSplitterPosPct() { return m_wndVertSplitter.GetSplitterPosPct(); }
  int GetHorzSplitterPosPct() { return m_wndHorzSplitter.GetSplitterPosPct(); }
  void ShowBackTrace(LOG_NODE* pNode);

private:
#ifdef NATIVE_TREE
  LOG_NODE* getTreeNode(HTREEITEM hItem);
#endif
  CSplitterWindow m_wndVertSplitter;
  CSplitterWindow m_wndHorzSplitter;
  CLogTreeView    m_wndTreeView;
  CLogListView    m_wndListView;
  //CStatic         m_wndLog;
  CEdit           m_wndInfo;
  LOG_NODE*       m_selectedNode;
};
