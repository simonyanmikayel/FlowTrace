#include "stdafx.h"
#include "resource.h"
#include "DlgFilters.h"
#include "Settings.h"

#define COLLUMN_1 "Processes to bypass"

LRESULT DlgFilters::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_ApplyPorcessFilter.Attach(GetDlgItem(IDC_CHECK_APPLY));
    m_List.Attach(GetDlgItem(IDC_STATIC_LST));
	m_ApplyLogcutFilter.Attach(GetDlgItem(IDC_CHECK_APPLY_LOGCUT_FILTER));
	m_edtLogcutFilter.Attach(GetDlgItem(IDC_EDIT_LOGCUT_FILTER));

	m_ApplyPorcessFilter.SetCheck(gSettings.GetApplyPorcessFilter() ? BST_CHECKED : BST_UNCHECKED);
	m_ApplyLogcutFilter.SetCheck(gSettings.GetApplyLogcutFilter() ? BST_CHECKED : BST_UNCHECKED);

    CRect rc;
    m_List.GetClientRect(&rc);
    m_Grid.Create(m_List.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE, 1);
    m_Grid.SetListener(this);
    m_Grid.SetExtendedGridStyle(GS_EX_CONTEXTMENU);
    m_Grid.GetClientRect(&rc);
    m_Grid.AddColumn(COLLUMN_1, m_Grid.GetGridClientWidth(), CGridCtrl::EDIT_TEXT);

    m_Grid.SetRedraw(FALSE);
    m_Grid.DeleteAllItems();
	StringList& filterList = gSettings.getProcessFilterList();
	for (int i = 0; i < filterList.getItemCount(); i++)
	{
		long nItem = m_Grid.AddRow();
		m_Grid.SetItem(nItem, COLLUMN_1, filterList.getItem(i));
	}
    m_Grid.SetRedraw(TRUE);

	StringList& stringList = gSettings.getAdbFilterList();
	m_edtLogcutFilter.SetWindowText(stringList.toString());

	CenterWindow(GetParent());
    m_Grid.SetFocus();
    return TRUE;
}

LRESULT DlgFilters::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (m_Grid.IsEditing())
    {
        m_Grid.EndEdit(wID != IDOK);
        return 0;
    }

    if (wID == IDOK)
    {
        gSettings.SetApplyPorcessFilter(m_ApplyPorcessFilter.GetCheck());
		gSettings.SetApplyLogcutFilter(m_ApplyLogcutFilter.GetCheck());

        string strItems;
        int count = m_Grid.GetRowCount();
        for (int i = 0; i < count; i++)
        {
            _variant_t vtItemName = m_Grid.GetItem(i, COLLUMN_1);
            if (vtItemName.vt == VT_BSTR && vtItemName.bstrVal)
            {
                char* szItem = _com_util::ConvertBSTRToString(vtItemName.bstrVal);
                if (szItem[0]) {
                    strItems += szItem;
                    strItems += "\n";
                }
                delete szItem;
            }
        }
		StringList& filterList = gSettings.getProcessFilterList();
		filterList.setList(strItems.c_str());

		CString strLogcutFilter;
		m_edtLogcutFilter.GetWindowText(strLogcutFilter);
		StringList& stringList = gSettings.getAdbFilterList();
		stringList.setList(strLogcutFilter.GetString());
    }
	EndDialog(wID);
	return 0;
}


LRESULT DlgFilters::OnBnClickedButtonAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_Grid.SetFocus();
    m_Grid.InsertRow();
    return 0;
}


LRESULT DlgFilters::OnBnClickedButtonEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_Grid.SetFocus();
    m_Grid.EditSelectedRow();
    return 0;
}


LRESULT DlgFilters::OnBnClickedButtonDel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_Grid.SetFocus();
    m_Grid.DeleteSelectedRow();
    return 0;
}
