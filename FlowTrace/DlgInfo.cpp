#include "stdafx.h"
#include "resource.h"
#include "DlgInfo.h"

DlgInfo::DlgInfo(char* szInfo)
{
	m_info = szInfo;
}

LRESULT DlgInfo::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_edtInfo.Attach(GetDlgItem(IDC_EIDT_INFO));
	m_edtInfo.SetWindowText(m_info.c_str());

	CenterWindow(GetParent());
	return TRUE;
}

LRESULT DlgInfo::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}
