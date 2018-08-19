#pragma once

class DlgModules : public CDialogImpl<DlgModules>, public CGridCtrl::CListener
{
public:
	enum { IDD = IDD_MODULES };

	BEGIN_MSG_MAP(DlgModules)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    CButton m_ResolveAddr;
    CStatic m_staticModulList;//	IDC_STATIC_MODUL_LST
    CGridCtrl m_ModulGrid;
};
