#include "stdafx.h"
#include "resource.h"
#include "DlgDetailes.h"
#include "Settings.h"

LRESULT DlgDetailes::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  CenterWindow(GetParent());

  m_chkLineNN.Attach(GetDlgItem(IDC_LINE_NN));
  m_chkNN.Attach(GetDlgItem(IDC_NN));
  m_chkApp.Attach(GetDlgItem(IDC_APP));
  m_chkPID.Attach(GetDlgItem(IDC_PID));
  m_chkThreadNN.Attach(GetDlgItem(IDC_THREAD_NN));
  m_chkTime.Attach(GetDlgItem(IDC_TIME));
  m_chkCallAddr.Attach(GetDlgItem(IDC_CALL_ADDR));
  m_chkFnCallLine.Attach(GetDlgItem(IDC_FN_CALL_LINE));
  m_chkFuncName.Attach(GetDlgItem(IDC_FUNC_NAME));
  m_chkCallLine.Attach(GetDlgItem(IDC_CALL_LINE));
  m_ShowAppIp.Attach(GetDlgItem(IDC_CHECK_SHOW_APP_IP));
  m_ShowElapsedTime.Attach(GetDlgItem(IDC_CHECK_SHOW_ELAPSED_TIME));
  m_chkChildCount.Attach(GetDlgItem(IDC_CHILD_COUNT));

  m_chkLineNN.SetCheck(gSettings.GetColLineNN());
  m_chkNN.SetCheck(gSettings.GetColNN());
  m_chkApp.SetCheck(gSettings.GetColApp());
  m_chkPID.SetCheck(gSettings.GetColPID());
  m_chkThreadNN.SetCheck(gSettings.GetColThreadNN());
  m_chkTime.SetCheck(gSettings.GetColTime());
  m_chkCallAddr.SetCheck(gSettings.GetColCallAddr());
  m_chkFnCallLine.SetCheck(gSettings.GetFnCallLine());
  m_chkFuncName.SetCheck(gSettings.GetColFunc());
  m_chkCallLine.SetCheck(gSettings.GetColLine());
  m_ShowAppIp.SetCheck(gSettings.GetShowAppIp());
  m_ShowElapsedTime.SetCheck(gSettings.GetShowElapsedTime());
  m_chkChildCount.SetCheck(gSettings.GetShowChildCount());

  return TRUE;
}

LRESULT DlgDetailes::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  if (wID == IDOK)
  {
    gSettings.SetColLineNN(m_chkLineNN.GetCheck());
    gSettings.SetColNN(m_chkNN.GetCheck());
    gSettings.SetColApp(m_chkApp.GetCheck());
    gSettings.SetColPID(m_chkPID.GetCheck());
	gSettings.SetColThreadNN(m_chkThreadNN.GetCheck());
    gSettings.SetColTime(m_chkTime.GetCheck());
    gSettings.SetColCallAddr(m_chkCallAddr.GetCheck());
    gSettings.SetFnCallLine(m_chkFnCallLine.GetCheck());
    gSettings.SetColFunc(m_chkFuncName.GetCheck());
    gSettings.SetColLine(m_chkCallLine.GetCheck());
    gSettings.SetShowAppIp(m_ShowAppIp.GetCheck());
    gSettings.SetShowElapsedTime(m_ShowElapsedTime.GetCheck());
    gSettings.SetShowChildCount(m_chkChildCount.GetCheck());
  }
  EndDialog(wID);
  return 0;
}
