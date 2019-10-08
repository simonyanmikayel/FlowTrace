#pragma once

class DlgFilters : public CDialogImpl<DlgFilters>, public CGridCtrl::CListener
{
public:
	enum { IDD = IDD_FILTERS};

	BEGIN_MSG_MAP(DlgModules)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
        COMMAND_HANDLER(IDC_BUTTON_ADD, BN_CLICKED, OnBnClickedButtonAdd)
        COMMAND_HANDLER(IDC_BUTTON_EDIT, BN_CLICKED, OnBnClickedButtonEdit)
        COMMAND_HANDLER(IDC_BUTTON_DEL, BN_CLICKED, OnBnClickedButtonDel)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    CButton m_Apply;
    CStatic m_List;
    CGridCtrl m_Grid;
public:
    LRESULT OnBnClickedButtonAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedButtonEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedButtonDel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
