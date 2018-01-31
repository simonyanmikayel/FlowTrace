#pragma once

class CColumnsDlg : public CDialogImpl<CColumnsDlg>
{
public:
  enum { IDD = IDD_DETAILES };

  BEGIN_MSG_MAP(CColumnsDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
    COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
  END_MSG_MAP()

  CButton m_chkLineNN;
  CButton m_chkNN;
  CButton m_chkApp;
  CButton m_chkPID;
  CButton m_chkTime;
  CButton m_chkCallAddr;
  CButton m_chkFnCallLine;
  CButton m_chkFuncName;
  CButton m_chkCallLine;
  CButton m_ShowAppIp;
  CButton m_ShowAppTime;
  CButton m_ShowElapsedTime;

  // Handler prototypes (uncomment arguments if needed):
  //	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
  //	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
  //	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
