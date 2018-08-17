#pragma once

class DlgInfo : public CDialogImpl<DlgInfo>
{
public:
	DlgInfo(char* szInfo);
	enum { IDD = IDD_INFO };

	BEGIN_MSG_MAP(DlgInfo)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	CEdit m_edtInfo;
	std::string m_info;
};
