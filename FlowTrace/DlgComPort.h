#pragma once

#include "Settings.h"

class DlgComPort : public CDialogImpl<DlgComPort>
{
public:
    DlgComPort(ComPort& comPort) : m_comPort(comPort) {}
    enum { IDD = IDD_COM_PORT
    };

    BEGIN_MSG_MAP(DlgComPort)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    ComPort& m_comPort;
    CEdit m_Name;
    CEdit m_Speed;
    CEdit m_DataBits;
    CEdit m_StopBits;
    CComboBox m_Parity;
    CComboBox m_FlowControl;
};


