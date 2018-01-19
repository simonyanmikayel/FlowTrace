#include "stdafx.h"
#include "resource.h"
#include "DetailesDlg.h"
#include "Settings.h"

LRESULT CColumnsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  CenterWindow(GetParent());

  m_chkLineNN.Attach(GetDlgItem(IDC_LINE_NN));
  m_chkNN.Attach(GetDlgItem(IDC_NN));
  m_chkApp.Attach(GetDlgItem(IDC_APP));
  m_chkPID.Attach(GetDlgItem(IDC_PID));
  m_chkTime.Attach(GetDlgItem(IDC_TIME));
  m_chkCallAddr.Attach(GetDlgItem(IDC_CALL_ADDR));
  m_chkFuncName.Attach(GetDlgItem(IDC_FUNC_NAME));
  m_chkCallLine.Attach(GetDlgItem(IDC_CALL_LINE));
  m_ShowAppIp.Attach(GetDlgItem(IDC_CHECK_SHOW_APP_IP));
  m_ShowAppTime.Attach(GetDlgItem(IDC_CHECK_SHOW_APP_TIME));
  m_ShowElapsedTime.Attach(GetDlgItem(IDC_CHECK_SHOW_ELAPSED_TIME));

  m_chkLineNN.SetCheck(gSettings.GetColLineNN());
  m_chkNN.SetCheck(gSettings.GetColNN());
  m_chkApp.SetCheck(gSettings.GetColApp());
  m_chkPID.SetCheck(gSettings.GetColPID());
  m_chkTime.SetCheck(gSettings.GetColTime());
  m_chkCallAddr.SetCheck(gSettings.GetColCallAddr());
  m_chkFuncName.SetCheck(gSettings.GetColFunc());
  m_chkCallLine.SetCheck(gSettings.GetColLine());
  m_ShowAppIp.SetCheck(gSettings.GetShowAppIp());
  m_ShowAppTime.SetCheck(gSettings.GetShowAppTime());
  m_ShowElapsedTime.SetCheck(gSettings.GetShowElapsedTime());

  return TRUE;
}

LRESULT CColumnsDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  if (wID == IDOK)
  {
    gSettings.SetColLineNN(m_chkLineNN.GetCheck());
    gSettings.SetColNN(m_chkNN.GetCheck());
    gSettings.SetColApp(m_chkApp.GetCheck());
    gSettings.SetColPID(m_chkPID.GetCheck());
    gSettings.SetColTime(m_chkTime.GetCheck());
    gSettings.SetColCallAddr(m_chkCallAddr.GetCheck());
    gSettings.SetColFunc(m_chkFuncName.GetCheck());
    gSettings.SetColLine(m_chkCallLine.GetCheck());
    gSettings.SetShowAppIp(m_ShowAppIp.GetCheck());
    gSettings.SetShowAppTime(m_ShowAppTime.GetCheck());
    gSettings.SetShowElapsedTime(m_ShowElapsedTime.GetCheck());

  }
  EndDialog(wID);
  return 0;
}
