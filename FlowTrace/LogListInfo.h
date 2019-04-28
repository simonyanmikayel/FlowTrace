#pragma once
#include "GDI.h"

class CLogListFrame;
class CLogListView;

class CLogListInfo : public CWindowImpl< CLogListInfo>
{
public:
	CLogListInfo(CLogListFrame* pView, CLogListView *pLogListView);
	~CLogListInfo();

	BEGIN_MSG_MAP(CLogListInfo)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
	END_MSG_MAP()
	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & bHandled);
	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);

	void ApplySettings(bool fontChanged);
	int GetWidth();
	void SetRecCount(int recCount);

private:
	CLogListFrame* m_pParent;
	CLogListView *m_pLogListView;
	HIMAGELIST m_hListImageList;
	MemDC memDC;
	int m_recCount;
	int m_nnWidth;
	int m_nnDigits;
};

