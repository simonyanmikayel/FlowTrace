#pragma once
#include "LogListView.h"
#include "LogListInfo.h"

class CFlowTraceView;

class CLogListFrame : public CWindowImpl<CLogListFrame>
{
public:
	DECLARE_WND_CLASS(NULL)

	CLogListFrame(CFlowTraceView* pView);

	BEGIN_MSG_MAP(CLogListFrame)
		MSG_WM_CREATE(OnCreate)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnPositionChanged)
		NOTIFY_CODE_HANDLER(LVN_ENDSCROLL, OnLvnEndScroll)
		NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
	END_MSG_MAP()

	LRESULT OnCreate(LPCREATESTRUCT lpcs);
	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnPositionChanged(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnLvnEndScroll(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	void SetChiledPos();

	CLogListView& listView() { return m_wndListView; }
	CLogListInfo& listInfo() { return m_wndListInfo; }
private:
	CLogListView m_wndListView;
	CLogListInfo m_wndListInfo;
	CFlowTraceView* m_pParent;
};
