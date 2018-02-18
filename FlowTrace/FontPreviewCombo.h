#pragma once

class CFontPreviewCombo : public CWindowImpl<CFontPreviewCombo, CComboBox>, public COwnerDraw<CFontPreviewCombo>
{
  // Construction
public:
  CFontPreviewCombo();

  // Attributes
public:
  typedef CWindowImpl<CFontPreviewCombo, CComboBox> baseClass;
  BEGIN_MSG_MAP(CFontPreviewCombo)
    REFLECTED_COMMAND_CODE_HANDLER(CBN_DROPDOWN, OnDropDown)
    CHAIN_MSG_MAP_ALT(COwnerDraw<CFontPreviewCombo>, 1)
    DEFAULT_REFLECTION_HANDLER()
    //CHAIN_MSG_MAP(baseClass)
  END_MSG_MAP()

  /*
  All of the following options must be set before you call Init() !!
  */

  // set the "sample" text string, defaults to "abcdeABCDE"
  CString m_csSample;

  // choose the sample color (only applies with NAME_THEN_SAMPLE and SAMPLE_THEN_NAME)
  COLORREF	m_clrSample;

  // choose how the name and sample are displayed
  typedef enum
  {
    NAME_ONLY = 0,		// font name, drawn in font
    NAME_GUI_FONT,		// font name, drawn in GUI font
    NAME_THEN_SAMPLE,	// font name in GUI font, then sample text in font
    SAMPLE_THEN_NAME,	// sample text in font, then font name in GUI font
    SAMPLE_ONLY			// sample text in font
  } PreviewStyle;

  // Operations
public:

public:
  LRESULT OnDropDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled);
  void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
  void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

  // Implementation
public:
  int GetPreviewStyle(void);
  void SetPreviewStyle(PreviewStyle style, bool reinitialize = true);
  int GetFontHeight(void);
  void SetFontHeight(int newHeight, bool reinitialize = true);
  virtual ~CFontPreviewCombo();
  void	Init(CHAR* szResFontName);
  void SetDropWidth();

protected:
  CImageList m_img;
  CAtlMap<CString, HFONT>  m_fonts;

  PreviewStyle	m_style;
  int m_iFontHeight;
  int m_iMaxNameWidth;
  int m_iMaxSampleWidth;
  CString m_strResFontName;

  void AddFont(const CString& faceName);
  friend BOOL CALLBACK FPC_EnumFontProc(LPLOGFONT, LPTEXTMETRIC, DWORD, LPARAM);
  void DeleteAllFonts(void);
};