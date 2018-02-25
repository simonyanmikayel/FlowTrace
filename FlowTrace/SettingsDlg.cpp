#include "stdafx.h"
#include "resource.h"
#include "SettingsDlg.h"
#include "Settings.h"

#define AS(s) m_cmbFontSize.AddString(#s)

LRESULT CSettingsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  m_lblFont.Attach(GetDlgItem(IDC_FONT_NAME));
  m_UdpPort.Attach(GetDlgItem(IDC_EDIT_PORT));
  m_UsePcTime.Attach(GetDlgItem(IDC_CHECK_USE_PC_TIME));
  m_btnFont.Attach(GetDlgItem(IDC_BTN_FONT));
  m_btnReset.Attach(GetDlgItem(IDC_BUTTON_RESET));
  m_ResolveAddr.Attach(GetDlgItem(IDC_CHECK_RESOLVE_ADDR));
  m_FullSrcPath.Attach(GetDlgItem(IDC_CHECK_FULL_SRC_PATH));
  m_edtEclipsePath.Attach(GetDlgItem(IDC_EDIT_ECLIPSE_ON_WIN));
  m_edtLinuxHome.Attach(GetDlgItem(IDC_EDIT_LINUX_HOME));
  m_edtMapOnWin.Attach(GetDlgItem(IDC_EDIT_MAP_ON_WIN));
  
  m_FontSize = gSettings.GetFontSize();
  strncpy_s(m_FaceName, _countof(m_FaceName), gSettings.GetFontName(), _countof(m_FaceName) - 1);
  m_FaceName[LF_FACESIZE - 1] = 0;
  m_lfWeight = gSettings.GetFontWeight();
  SetFontLabel();

  SetDlgItemInt(IDC_EDIT_PORT, gSettings.GetUdpPort(), FALSE);
  m_UsePcTime.SetCheck(gSettings.GetUsePcTime() ? BST_CHECKED : BST_UNCHECKED);
  m_ResolveAddr.SetCheck(gSettings.GetResolveAddr() ? BST_CHECKED : BST_UNCHECKED);
  m_FullSrcPath.SetCheck(gSettings.GetFullSrcPath() ? BST_CHECKED : BST_UNCHECKED);
  m_edtEclipsePath.SetWindowText(gSettings.GetEclipsePath());
  m_edtLinuxHome.SetWindowText(gSettings.GetLinuxHome());
  m_edtMapOnWin.SetWindowText(gSettings.GetMapOnWin());
  CenterWindow(GetParent());
  return TRUE;
}

void CSettingsDlg::SetFontLabel()
{
  CString str;
  str.Format(_T("Font: %s, %s%d point "), m_FaceName, m_lfWeight >= FW_SEMIBOLD  ? _T("Bold ") : _T(""), m_FontSize);
  m_lblFont.SetWindowText(str);
}

LRESULT CSettingsDlg::OnBnClickedBtnFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
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

LRESULT CSettingsDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  if (wID == IDOK)
  {
    gSettings.SetUIFont(m_FaceName, m_lfWeight, m_FontSize);
    gSettings.SetUdpPort(GetDlgItemInt(IDC_EDIT_PORT));
    gSettings.SetUsePcTime(m_UsePcTime.GetCheck());
    gSettings.SetResolveAddr(m_ResolveAddr.GetCheck());
    gSettings.SetFullSrcPath(m_FullSrcPath.GetCheck());

    CString strEclipsePath;
    m_edtEclipsePath.GetWindowText(strEclipsePath);
    gSettings.SetEclipsePath(strEclipsePath.GetString());

    CString strLinuxHome;
    m_edtLinuxHome.GetWindowText(strLinuxHome);
    gSettings.SetLinuxHome(strLinuxHome.GetString());
    CString strMapOnWin;
    m_edtMapOnWin.GetWindowText(strMapOnWin);
    gSettings.SetMapOnWin(strMapOnWin.GetString());

  }
  EndDialog(wID);
  return 0;
}

LRESULT CSettingsDlg::OnBnClickedButtonReset(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  return 0;
}
