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
  m_CompactView.Attach(GetDlgItem(IDC_CHECK_COMPACT_VIEW));
  m_ShowAppIp.Attach(GetDlgItem(IDC_CHECK_SHOW_APP_IP));
  m_btnFont.Attach(GetDlgItem(IDC_BTN_FONT));
  m_btnReset.Attach(GetDlgItem(IDC_BUTTON_RESET));

  m_FontSize = gSettings.GetFontSize();
  strncpy(m_FaceName, gSettings.GetFontName(), sizeof(m_FaceName) - 1);
  m_FaceName[LF_FACESIZE - 1] = 0;
  m_lfWeight = gSettings.GetFontWeight();
  SetFontLabel();

#ifdef USE_FONT_RES
  m_FontPreviewCombol.SubclassWindow(GetDlgItem(IDC_COMBO_FONTS));
  m_cmbFontSize.Attach(GetDlgItem(IDC_COMBO_FONT_SIZE));
  m_FontBold.Attach(GetDlgItem(IDC_CHECK_FONT_WEIGHT));

  m_FontPreviewCombol.ShowWindow(SW_SHOW);
  m_cmbFontSize.ShowWindow(SW_SHOW);
  m_FontBold.ShowWindow(SW_SHOW);
  m_btnReset.ShowWindow(SW_SHOW);

  m_lblFont.ShowWindow(SW_HIDE);
  m_btnFont.ShowWindow(SW_HIDE);

  char* fs[] = { "8","9","10","11","12","13","14","15","16","17","18","20","22","24","26","28","30","32","34","36","48","72" };
  for (int i = 0; i < sizeof(fs) / sizeof(fs[0]); i++)
  {
    m_cmbFontSize.AddString(fs[i]);
  }

  m_FontPreviewCombol.Init(gSettings.GetResFontName());

  CString str; str.Format(_T("%d"), m_FontSize);
  m_cmbFontSize.SetWindowText(str);
  m_FontPreviewCombol.SelectString(0, gSettings.GetFontName());
  m_FontBold.SetCheck(m_lfWeight >= FW_SEMIBOLD ? BST_CHECKED : BST_UNCHECKED);
#endif

  SetDlgItemInt(IDC_EDIT_PORT, gSettings.GetUdpPort(), FALSE);
  m_UsePcTime.SetCheck(gSettings.GetUsePcTime() ? BST_CHECKED : BST_UNCHECKED);
  m_CompactView.SetCheck(gSettings.GetCompactView() ? BST_CHECKED : BST_UNCHECKED);
  m_ShowAppIp.SetCheck(gSettings.GetShowAppIp() ? BST_CHECKED : BST_UNCHECKED);

  CenterWindow(GetParent());
  return TRUE;
}

void CSettingsDlg::SetFontLabel()
{
  CString str;
  str.Format(_T("Font: %s, %s%d point "), m_FaceName, m_lfWeight >= FW_SEMIBOLD  ? "Bold " : _T(""), m_FontSize);
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

  strncpy(lf.lfFaceName, m_FaceName, sizeof(lf.lfFaceName) - 1);
  lf.lfFaceName[sizeof(lf.lfFaceName) - 1] = '\0';
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
    strncpy(m_FaceName, lf.lfFaceName, sizeof(m_FaceName) - 1);
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
#ifdef USE_FONT_RES
    CString str;      
    m_cmbFontSize.GetWindowText(str);
    m_FontSize = atoi(str);
    if (m_FontSize < 6)
      m_FontSize = 6;
    if (m_FontSize <= 0)
      m_FontSize = 12;
    if (m_FontSize > 72)
      m_FontSize = 72;

    m_FontPreviewCombol.GetWindowText(str);
    strncpy(m_FaceName, str, sizeof(m_FaceName) - 1);
    m_FaceName[LF_FACESIZE - 1] = 0;

    m_lfWeight = (m_FontBold.GetCheck() == BST_CHECKED) ? FW_BOLD : FW_NORMAL;
#endif

    gSettings.SetUIFont(m_FaceName, m_lfWeight, m_FontSize);
    gSettings.SetUdpPort(GetDlgItemInt(IDC_EDIT_PORT));
    gSettings.SetUsePcTime(m_UsePcTime.GetCheck());
    gSettings.SetCompactView(m_CompactView.GetCheck());
    gSettings.SetShowAppIp(m_ShowAppIp.GetCheck());
    
  }
  EndDialog(wID);
  return 0;
}

LRESULT CSettingsDlg::OnBnClickedButtonReset(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
#ifdef USE_FONT_RES
	m_cmbFontSize.SelectString(0, _T("12"));
  m_FontPreviewCombol.SelectString(0, gSettings.GetResFontName());
  m_FontBold.SetCheck(BST_UNCHECKED);
#endif
  return 0;
}
