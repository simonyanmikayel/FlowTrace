#include "stdafx.h"
#include "resource.h"
#include "DlgComPort.h"

LRESULT DlgComPort::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_Name.Attach(GetDlgItem(IDC_EDIT_PORT_NAME));
    m_Speed.Attach(GetDlgItem(IDC_EDIT_PORT_SPEED));
    m_DataBits.Attach(GetDlgItem(IDC_EDIT_PORT_DATA_BITS));
    m_StopBits.Attach(GetDlgItem(IDC_EDIT_PORT_STOP_BITS));
    m_Parity.Attach(GetDlgItem(IDC_COMBO_PORT_PARITY));
    m_FlowControl.Attach(GetDlgItem(IDC_COMBO_PORT_FLOW_CONTROL));

    m_Name.SetWindowText(m_comPort.GetName());
    SetDlgItemInt(IDC_EDIT_PORT_SPEED, m_comPort.GetSpeed(), FALSE);
    SetDlgItemInt(IDC_EDIT_PORT_DATA_BITS, m_comPort.GetDataBits(), FALSE);
    SetDlgItemInt(IDC_EDIT_PORT_STOP_BITS, m_comPort.GetStopBits(), FALSE);

    m_Parity.AddString(m_comPort.GetParityName(0));
    m_Parity.AddString(m_comPort.GetParityName(1));
    m_Parity.AddString(m_comPort.GetParityName(2));
    m_Parity.AddString(m_comPort.GetParityName(3));
    m_Parity.AddString(m_comPort.GetParityName(4));
    m_Parity.SetCurSel(m_comPort.GetParity());

    m_FlowControl.AddString(m_comPort.GetFlowControlName(0));
    m_FlowControl.AddString(m_comPort.GetFlowControlName(1));
    m_FlowControl.AddString(m_comPort.GetFlowControlName(2));
    m_FlowControl.AddString(m_comPort.GetFlowControlName(3));
    m_FlowControl.SetCurSel(m_comPort.GetFlowControl());

    CenterWindow(GetParent());
	return TRUE;
}

LRESULT DlgComPort::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (wID == IDOK)
    {
        CString strName;
        m_Name.GetWindowText(strName);
        m_comPort.SetName(strName.GetString());

        m_comPort.SetSpeed(GetDlgItemInt(IDC_EDIT_PORT_SPEED));
        m_comPort.SetDataBits(GetDlgItemInt(IDC_EDIT_PORT_DATA_BITS));
        m_comPort.SetStopBits(GetDlgItemInt(IDC_EDIT_PORT_STOP_BITS));

        m_comPort.SetParity(m_Parity.GetCurSel());
        m_comPort.SetFlowControl(m_FlowControl.GetCurSel());
    }
    EndDialog(wID);
    return 0;
}
