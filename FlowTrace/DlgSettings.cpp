#include "stdafx.h"
#include "resource.h"
#include "DlgSettings.h"
#include "DlgComPort.h"

#define AS(s) m_cmbFontSize.AddString(#s)

LRESULT DlgSettings::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  m_lblFont.Attach(GetDlgItem(IDC_FONT_NAME));
  m_UdpPort.Attach(GetDlgItem(IDC_EDIT_PORT));
  m_AdbArg.Attach(GetDlgItem(IDC_EDIT_ADB_ARG));
  m_btnFont.Attach(GetDlgItem(IDC_BTN_FONT));
  m_btnReset.Attach(GetDlgItem(IDC_BUTTON_RESET));
  m_UseAdb.Attach(GetDlgItem(IDC_CHECK_USE_ADB));
  m_RestartAdb.Attach(GetDlgItem(IDC_CHECK_RESTART_ADB));
  m_FullSrcPath.Attach(GetDlgItem(IDC_CHECK_FULL_SRC_PATH));
  m_edtEclipsePath.Attach(GetDlgItem(IDC_EDIT_ECLIPSE_ON_WIN));
  m_edtExternalCmd_1.Attach(GetDlgItem(IDC_EDIT_EXTERNAL_COMMAND_1));
  m_edtExternalCmd_2.Attach(GetDlgItem(IDC_EDIT_EXTERNAL_COMMAND_2));
  m_edtExternalCmd_3.Attach(GetDlgItem(IDC_EDIT_EXTERNAL_COMMAND_3));
  m_edtLinuxHome.Attach(GetDlgItem(IDC_EDIT_LINUX_HOME));
  m_edtMapOnWin.Attach(GetDlgItem(IDC_EDIT_MAP_ON_WIN));
  m_edtAndroidStudio.Attach(GetDlgItem(IDC_EDIT_ANDROID_STUDIO));
  m_edtAndroidProject.Attach(GetDlgItem(IDC_EDIT_ANDROID_PROJECT));
  m_UsePort_1.Attach(GetDlgItem(IDC_CHECK_USE_PORT_1));
  m_UsePort_2.Attach(GetDlgItem(IDC_CHECK_USE_PORT_2));
  m_lblPort_1.Attach(GetDlgItem(IDC_STATIC_PORT_1));
  m_lblPort_2.Attach(GetDlgItem(IDC_STATIC_PORT_2));

  m_FontSize = gSettings.GetfontSize();
  strncpy_s(m_FaceName, _countof(m_FaceName), gSettings.GetFontName(), _countof(m_FaceName) - 1);
  m_FaceName[LF_FACESIZE - 1] = 0;
  m_lfWeight = gSettings.GetFontWeight();

  SetDlgItemInt(IDC_EDIT_PORT, gSettings.GetUdpPort(), FALSE);
  m_AdbArg.SetWindowText(gSettings.GetAdbArg());
  m_FullSrcPath.SetCheck(gSettings.GetFullSrcPath() ? BST_CHECKED : BST_UNCHECKED);
  m_UseAdb.SetCheck(gSettings.GetUseAdb() ? BST_CHECKED : BST_UNCHECKED);
  m_RestartAdb.SetCheck(gSettings.GetRestartAdb() ? BST_CHECKED : BST_UNCHECKED);
  m_edtEclipsePath.SetWindowText(gSettings.GetEclipsePath());
  m_edtExternalCmd_1.SetWindowText(gSettings.GetExternalCmd_1());
  m_edtExternalCmd_2.SetWindowText(gSettings.GetExternalCmd_2());
  m_edtExternalCmd_3.SetWindowText(gSettings.GetExternalCmd_3());
  m_edtLinuxHome.SetWindowText(gSettings.GetLinuxHome());
  m_edtMapOnWin.SetWindowText(gSettings.GetMapOnWin());
  m_edtAndroidStudio.SetWindowText(gSettings.GetAndroidStudio());
  m_edtAndroidProject.SetWindowText(gSettings.GetAndroidProject());
  m_UsePort_1.SetCheck(gSettings.GetUseComPort_1() ? BST_CHECKED : BST_UNCHECKED);
  m_UsePort_2.SetCheck(gSettings.GetUseComPort_2() ? BST_CHECKED : BST_UNCHECKED);

  SetFontLabel();
  SetPortLabel(m_lblPort_1, gSettings.comPort_1);
  SetPortLabel(m_lblPort_2, gSettings.comPort_2);

  CenterWindow(GetParent());
  return TRUE;
}

void DlgSettings::SetFontLabel()
{
  CString str;
  str.Format(_T("Font: %s, %s%d point "), m_FaceName, m_lfWeight >= FW_SEMIBOLD  ? _T("Bold ") : _T(""), m_FontSize);
  m_lblFont.SetWindowText(str);
}

void DlgSettings::SetPortLabel(CStatic& lbl, ComPort& port)
{
    CString str;
    const CHAR* name = port.GetName();

    str.Format(_T("%s   [%d %d %d %s %s]"), 
        name[0] ? name : "???", port.GetSpeed(), port.GetDataBits(), port.GetStopBits(),
        port.GetParityName(port.GetParity()), port.GetFlowControlName(port.GetFlowControl()));
    lbl.SetWindowText(str);
}

LRESULT DlgSettings::OnBnClickedBtnFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  CHOOSEFONT cf;
  HDC hdc;
  LOGFONT lf;

  hdc = ::GetDC(0);
  lf.lfHeight = -MulDiv(m_FontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
  ::ReleaseDC(0, hdc);

  _tcsncpy_s(lf.lfFaceName, _countof(lf.lfFaceName), m_FaceName, _countof(lf.lfFaceName) - 1);
  lf.lfFaceName[_countof(lf.lfFaceName) - 1] = 0;
  lf.lfWidth = lf.lfEscapement = lf.lfOrientation = 0;
  lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = 0;
  lf.lfWeight = (m_lfWeight >= FW_SEMIBOLD ? FW_BOLD : FW_NORMAL);
  lf.lfCharSet = DEFAULT_CHARSET;
  lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
  lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  lf.lfQuality = DEFAULT_QUALITY;
  lf.lfPitchAndFamily = FF_DONTCARE; // | FIXED_PITCH

  ZeroMemory(&cf, sizeof(cf));
  cf.lStructSize = sizeof(cf);
  cf.hwndOwner = m_hWnd;
  cf.lpLogFont = &lf;
  cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_INACTIVEFONTS;// | CF_FIXEDPITCHONLY;

  if (ChooseFont(&cf)) {
    m_FontSize = cf.iPointSize / 10;
    _tcsncpy_s(m_FaceName, _countof(m_FaceName), lf.lfFaceName, _countof(m_FaceName) - 1);
    m_FaceName[LF_FACESIZE - 1] = 0;
    m_lfWeight = (lf.lfWeight >= FW_SEMIBOLD ? FW_BOLD : FW_NORMAL);
    SetFontLabel();
  }

  return 0;
}

LRESULT DlgSettings::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  if (wID == IDOK)
  {
    gSettings.SetUIFont(m_FaceName, m_lfWeight, m_FontSize);
    gSettings.SetUdpPort(GetDlgItemInt(IDC_EDIT_PORT));
    gSettings.SetFullSrcPath(m_FullSrcPath.GetCheck());
    gSettings.SetUseAdb(m_UseAdb.GetCheck());
    gSettings.SetRestartAdb(m_RestartAdb.GetCheck());

    CString strAdbrg;
    m_AdbArg.GetWindowText(strAdbrg);
    gSettings.SetAdbArg(strAdbrg.GetString());

    CString strEclipsePath;
    m_edtEclipsePath.GetWindowText(strEclipsePath);
    gSettings.SetEclipsePath(strEclipsePath.GetString());

	CString strExternalCmd;
	m_edtExternalCmd_1.GetWindowText(strExternalCmd);
	gSettings.SetExternalCmd_1(strExternalCmd.GetString());
	m_edtExternalCmd_2.GetWindowText(strExternalCmd);
	gSettings.SetExternalCmd_2(strExternalCmd.GetString());
	m_edtExternalCmd_3.GetWindowText(strExternalCmd);
	gSettings.SetExternalCmd_3(strExternalCmd.GetString());

    CString strLinuxHome;
    m_edtLinuxHome.GetWindowText(strLinuxHome);
    gSettings.SetLinuxHome(strLinuxHome.GetString());
    CString strMapOnWin;
    m_edtMapOnWin.GetWindowText(strMapOnWin);
    gSettings.SetMapOnWin(strMapOnWin.GetString());
	CString strAndroidStudio;
	m_edtAndroidStudio.GetWindowText(strAndroidStudio);
	gSettings.SetAndroidStudio(strAndroidStudio.GetString());
	CString strAndroidProject;
	m_edtAndroidProject.GetWindowText(strAndroidProject);
	gSettings.SetAndroidProject(strAndroidProject.GetString());

    gSettings.SetUseComPort_1(m_UsePort_1.GetCheck());
    gSettings.SetUseComPort_2(m_UsePort_2.GetCheck());
  }
  EndDialog(wID);
  return 0;
}

LRESULT DlgSettings::OnBnClickedButtonReset(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  return 0;
}

void DlgSettings::SetPort(CStatic& lbl, ComPort& port)
{
    DlgComPort dlg(port);
    dlg.DoModal();
    SetPortLabel(lbl, port);
}
LRESULT DlgSettings::OnBnClickedButtonSetPort_1(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SetPort(m_lblPort_1, gSettings.comPort_1);
    return 0;
}

LRESULT DlgSettings::OnBnClickedButtonSetPort_2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SetPort(m_lblPort_2, gSettings.comPort_2);
    return 0;
}
