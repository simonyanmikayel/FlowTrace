#include "stdafx.h"
#include "resource.h"
#include "DlgModules.h"
#include "Settings.h"

#define COL_MODULES "Module`s paths"

LRESULT DlgModules::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_Apply.Attach(GetDlgItem(IDC_CHECK_APPLY));
    m_List.Attach(GetDlgItem(IDC_STATIC_LST));

    m_Apply.SetCheck(gSettings.GetResolveAddr() ? BST_CHECKED : BST_UNCHECKED);


    CRect rc;
    m_List.GetClientRect(&rc);
    m_Grid.Create(m_List.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE, 1);
    m_Grid.SetListener(this);
    m_Grid.SetExtendedGridStyle(GS_EX_CONTEXTMENU);
    m_Grid.GetClientRect(&rc);
    m_Grid.AddColumn(COL_MODULES, m_Grid.GetGridClientWidth(), CGridCtrl::EDIT_TEXT);

    m_Grid.SetRedraw(FALSE);
    m_Grid.DeleteAllItems();
    CHAR* szModules = gSettings.GetModules();
    char *next_token = NULL;
    char *p = strtok_s(szModules, "\n", &next_token);
    while (p) {
        long nItem = m_Grid.AddRow();
        m_Grid.SetItem(nItem, COL_MODULES, p);
        p = strtok_s(NULL, "\n", &next_token);
    }
    m_Grid.SetRedraw(TRUE);

	CenterWindow(GetParent());
    m_Grid.SetFocus();
    return TRUE;
}

LRESULT DlgModules::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (m_Grid.IsEditing())
    {
        m_Grid.EndEdit(wID != IDOK);
        return 0;
    }

    if (wID == IDOK)
    {
        gSettings.SetResolveAddr(m_Apply.GetCheck());

        string strModules;
        int count = m_Grid.GetRowCount();
        for (int i = 0; i < count; i++)
        {
            _variant_t vtModuleName = m_Grid.GetItem(i, COL_MODULES);
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
    }
	EndDialog(wID);
	return 0;
}


LRESULT DlgModules::OnBnClickedButtonAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_Grid.SetFocus();
    m_Grid.InsertRow();
    return 0;
}


LRESULT DlgModules::OnBnClickedButtonEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_Grid.SetFocus();
    m_Grid.EditSelectedRow();
    return 0;
}


LRESULT DlgModules::OnBnClickedButtonDel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_Grid.SetFocus();
    m_Grid.DeleteSelectedRow();
    return 0;
}
