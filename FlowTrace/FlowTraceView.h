// FlowTraceView.h : interface of the CFlowTraceView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Archive.h"
#include "LogListFrame.h"
#include "LogTreeView.h"
#include "BackTraceView.h"

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

    NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
  END_MSG_MAP()

  LRESULT OnCreate(LPCREATESTRUCT lpcs);
  LRESULT OnNotify(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnPositionChanging(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

  LOG_NODE* SyncViews();
  void ShowTreeView(bool show);
  void ShowStackView(bool show);
  void ClearLog();
  void ApplySettings(bool fontChanged);
  void SetChildPos(int cx, int cy);
  CLogListView& list() { return m_wndListView; }
  CLogTreeView& tree() { return m_wndTreeView; }
  CBackTraceView& backTrace() { return m_wndBackTraceView; }
  int GetVertSplitterPosPct() { return m_wndVertSplitter.GetSplitterPosPct(); }
  int GetHorzSplitterPosPct() { return m_wndHorzSplitter.GetSplitterPosPct(); }
  void ShowBackTrace(LOG_NODE* pNode);
private:

  CSplitterWindow m_wndVertSplitter;
  CSplitterWindow m_wndHorzSplitter;
  CLogTreeView    m_wndTreeView;
  CLogListFrame    m_wndListFrame;
  CLogListView     &m_wndListView;
  CBackTraceView  m_wndBackTraceView;
};
