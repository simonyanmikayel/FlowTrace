#include "stdafx.h"
#include "resource.h"
#include "DlgModules.h"
#include "Settings.h"

#define COL_MODULES "Module`s paths"

LRESULT DlgModules::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_ResolveAddr.Attach(GetDlgItem(IDC_CHECK_RESOLVE_ADDR));
    m_staticModulList.Attach(GetDlgItem(IDC_STATIC_MODUL_LST));
    m_UsePrefModule.Attach(GetDlgItem(IDC_CHECK_PREFERED_MODULE));
    m_PrefModulePath.Attach(GetDlgItem(IDC_EDIT_PREFERED_MODULE));

    m_ResolveAddr.SetCheck(gSettings.GetResolveAddr() ? BST_CHECKED : BST_UNCHECKED);
    m_UsePrefModule.SetCheck(gSettings.GetUsePrefModule() ? BST_CHECKED : BST_UNCHECKED);
    m_PrefModulePath.SetWindowText(gSettings.GetPrefModulePath());


    CRect rc;
    m_staticModulList.GetClientRect(&rc);
    m_ModulGrid.Create(m_staticModulList.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE, 1);
    m_ModulGrid.SetListener(this);
    m_ModulGrid.SetExtendedGridStyle(GS_EX_CONTEXTMENU);
    m_ModulGrid.GetClientRect(&rc);
    m_ModulGrid.AddColumn(COL_MODULES, m_ModulGrid.GetGridClientWidth(), CGridCtrl::EDIT_TEXT);

    m_ModulGrid.SetRedraw(FALSE);
    m_ModulGrid.DeleteAllItems();
    CHAR* szModules = gSettings.GetModules();
    char *next_token = NULL;
    char *p = strtok_s(szModules, "\n", &next_token);
    while (p) {
        long nItem = m_ModulGrid.AddRow();
        m_ModulGrid.SetItem(nItem, COL_MODULES, p);
        p = strtok_s(NULL, "\n", &next_token);
    }
    m_ModulGrid.SetRedraw(TRUE);

	CenterWindow(GetParent());
    m_ModulGrid.SetFocus();
    return TRUE;
}

LRESULT DlgModules::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (m_ModulGrid.IsEditing())
    {
        m_ModulGrid.EndEdit(wID != IDOK);
        return 0;
    }

    if (wID == IDOK)
    {
        gSettings.SetResolveAddr(m_ResolveAddr.GetCheck());

        string strModules;
        int count = m_ModulGrid.GetRowCount();
        for (int i = 0; i < count; i++)
        {
            _variant_t vtModuleName = m_ModulGrid.GetItem(i, COL_MODULES);
            if (vtModuleName.vt == VT_BSTR && vtModuleName.bstrVal)
            {
                char* szModul = _com_util::ConvertBSTRToString(vtModuleName.bstrVal);
                if (szModul[0]) {
                    strModules += szModul;
                    strModules += "\n";
                }
                delete szModul;
            }
        }
        gSettings.SetModules(strModules.c_str());

        gSettings.SetUsePrefModule(m_UsePrefModule.GetCheck());
        CString strPrefModulePath;
        m_PrefModulePath.GetWindowText(strPrefModulePath);
        gSettings.SetPrefModulePath(strPrefModulePath.GetString());

    }
	EndDialog(wID);
	return 0;
}


LRESULT DlgModules::OnBnClickedButtonAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_ModulGrid.SetFocus();
    m_ModulGrid.InsertRow();
    return 0;
}


LRESULT DlgModules::OnBnClickedButtonEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_ModulGrid.SetFocus();
    m_ModulGrid.EditSelectedRow();
    return 0;
}


LRESULT DlgModules::OnBnClickedButtonDel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_ModulGrid.SetFocus();
    m_ModulGrid.DeleteSelectedRow();
    return 0;
}
