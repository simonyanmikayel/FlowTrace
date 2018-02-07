#pragma once

#include "BackTraceCallView.h"

class CBackTraceView : public CWindowImpl<CBackTraceView >
{
public:
  DECLARE_WND_CLASS(NULL)

  CBackTraceView(CFlowTraceView* pView);

  BEGIN_MSG_MAP(CBackTraceView)
    MSG_WM_CREATE(OnCreate)
    MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, OnPositionChanging)
    NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
  END_MSG_MAP()

  LRESULT OnCreate(LPCREATESTRUCT lpcs);
  LRESULT OnPositionChanging(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
  void SetChildPos(int cx, int cy);
  int GetStackSplitterPosPct() { return m_wndVertSplitter.GetSplitterPosPct(); }

  void UpdateBackTrace(LOG_NODE* pSelectedNode) 
  {
    m_wndCallFuncView.UpdateBackTrace(pSelectedNode, true);
    m_wndCallStackView.UpdateBackTrace(pSelectedNode, false);
  }

  void ClearTrace()
  {
    m_wndCallFuncView.ClearTrace();
    m_wndCallStackView.ClearTrace();
  }

  void ApplySettings(bool fontChanged);

  CBackTraceCallView  m_wndCallFuncView;
  CBackTraceCallView  m_wndCallStackView;
private:
  CSplitterWindow m_wndVertSplitter;
};
