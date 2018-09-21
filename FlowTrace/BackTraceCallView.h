#pragma once

#include "Archive.h"
#include "Helpers.h"

#define MAX_BACK_TRACE 512
enum BACK_TRACE_COL { BACK_TRACE_FN, BACK_TRACE_LINE, BACK_TRACE_SRC, BACK_TRACE_LAST_COL };//BACK_TRACE_LNK_CALL, BACK_TRACE_LNK_FUNC, 

class CFlowTraceView;
struct BackTraceSelection
{
	int iItem;
	int iSubItem;
	LOG_NODE* pNode;
};

class CBackTraceCallView : public CWindowImpl< CBackTraceCallView, CListViewCtrl>
{
public:
	CBackTraceCallView(CFlowTraceView* pView);
	~CBackTraceCallView();

	void ClearTrace();
	void UpdateBackTrace(LOG_NODE* pSelectedNode, bool bNested);
	void CopySelection();
	void CopySelection(bool all);

	BEGIN_MSG_MAP(CBackTraceView)
		MSG_WM_SIZE(OnSize)
        MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
	END_MSG_MAP()
	void ItemPrePaint(DWORD iItem, HDC hdc, RECT rc);
	void DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rc);
	void OnSize(UINT nType, CSize size);
	INFO_NODE* nodes[MAX_BACK_TRACE];
	int subItemColWidth[BACK_TRACE_LAST_COL];
	void SetSelectionOnMouseEven(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void SetSelection(int iItem, int iSubItem);
	void ClearSelection();
    LRESULT OnMButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
    LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
	LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	int getSubItemText(int iItem, int iSubItem, char* buf, int cbBuf);
	LOG_NODE* GetSelectedNode() { return m_sel.pNode; }
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);

private:
	BackTraceSelection m_sel;
	int c_nodes;
	bool m_Initialised;
	CFlowTraceView* m_pView;
	int min_col_width;
};