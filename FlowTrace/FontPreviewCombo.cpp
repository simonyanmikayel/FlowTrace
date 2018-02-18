#include "stdafx.h"
#include "resource.h"
#include "FontPreviewCombo.h"


#define SPACING      10
#define GLYPH_WIDTH  15

/////////////////////////////////////////////////////////////////////////////
// CFontPreviewCombo

CFontPreviewCombo::CFontPreviewCombo()
{
  m_iFontHeight = 16;
  m_iMaxNameWidth = 0;
  m_iMaxSampleWidth = 0;
  m_style = NAME_THEN_SAMPLE;
  m_csSample = "abcdeABCDE";
  m_clrSample = GetSysColor(COLOR_WINDOWTEXT);
}

CFontPreviewCombo::~CFontPreviewCombo()
{
  DeleteAllFonts();
  m_hWnd = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CFontPreviewCombo message handlers

BOOL CALLBACK FPC_EnumFontProc(LPLOGFONT lplf, LPTEXTMETRIC lptm, DWORD dwType, LPARAM lpData)
{
  CFontPreviewCombo *pThis = reinterpret_cast<CFontPreviewCombo*>(lpData);
  if (pThis->m_strResFontName != lplf->lfFaceName)
  {
    int index = pThis->AddString(lplf->lfFaceName);
    ATLASSERT(index != -1);
    //int maxLen = lptm->tmMaxCharWidth * strlen(lplf->lfFaceName);
    int ret = pThis->SetItemData(index, dwType);

    ATLASSERT(ret != -1);

    pThis->AddFont(lplf->lfFaceName);
  }
  else
  {
    dwType = dwType;
  }

  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

void CFontPreviewCombo::Init(CHAR* szResFontName)
{
  if (szResFontName)
  {
    m_strResFontName = szResFontName;
  }

  m_img.Create(IDB_TTF_BMP, GLYPH_WIDTH, 1, RGB(255, 255, 255));
  CClientDC dc(m_hWnd);

  ResetContent();
  DeleteAllFonts();
  if (!m_strResFontName.IsEmpty())
  {
    int index = AddString(m_strResFontName);
    ATLASSERT(index != -1);
    int ret = SetItemData(index, 4);
    AddFont(m_strResFontName);
  }
  EnumFonts(dc, 0, (FONTENUMPROC)FPC_EnumFontProc, (LPARAM)this); //Enumerate font

  //SetDropWidth();

  SetCurSel(0);
}

/////////////////////////////////////////////////////////////////////////////

void CFontPreviewCombo::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
  ATLASSERT(lpDIS->CtlType == ODT_COMBOBOX);

  CRect rc = lpDIS->rcItem;

  CDC dc;
  dc.Attach(lpDIS->hDC);

  if (lpDIS->itemState & ODS_FOCUS)
    dc.DrawFocusRect(&rc);

  if (lpDIS->itemID == -1)
    return;

  int nIndexDC = dc.SaveDC();

  CBrush br;

  COLORREF clrSample = m_clrSample;

  if (lpDIS->itemState & ODS_SELECTED)
  {
    br.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
    dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
    clrSample = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
  }
  else
  {
    br.CreateSolidBrush(dc.GetBkColor());
  }

  dc.SetBkMode(TRANSPARENT);
  dc.FillRect(&rc, br);

  CHAR szText[256];
  int i = GetLBText(0, szText);

  // which one are we working on?
  CString csCurFontName;
  GetLBText(lpDIS->itemID, csCurFontName);

  // draw the cute TTF glyph
  DWORD_PTR dwData = GetItemData(lpDIS->itemID);
  if (dwData & TRUETYPE_FONTTYPE)
  {
    m_img.Draw(dc, 0, CPoint(rc.left + 5, rc.top + 4), ILD_TRANSPARENT);
  }
  rc.left += GLYPH_WIDTH;

  int iOffsetX = SPACING;

  // draw the text
  CSize sz;
  int iPosY = 0;
  HFONT hf = NULL;
  HFONT cf;
  bool lookupResult = m_fonts.Lookup(csCurFontName, cf);
  ATLASSERT(lookupResult);
  switch (m_style)
  {
  case NAME_GUI_FONT:
  {
    // font name in GUI font
    dc.GetTextExtent(csCurFontName, csCurFontName.GetLength(), &sz);
    iPosY = (rc.Height() - sz.cy) / 2;
    dc.TextOut(rc.left + iOffsetX, rc.top + iPosY, csCurFontName);
  }
  break;
  case NAME_ONLY:
  {
    // font name in current font
    hf = (HFONT)dc.SelectFont(cf);
    dc.GetTextExtent(csCurFontName, csCurFontName.GetLength(), &sz);
    iPosY = (rc.Height() - sz.cy) / 2;
    dc.TextOut(rc.left + iOffsetX, rc.top + iPosY, csCurFontName);
    dc.SelectFont(hf);
  }
  break;
  case NAME_THEN_SAMPLE:
  {
    // font name in GUI font
    dc.GetTextExtent(csCurFontName, csCurFontName.GetLength(), &sz);
    iPosY = (rc.Height() - sz.cy) / 2;
    dc.TextOut(rc.left + iOffsetX, rc.top + iPosY, csCurFontName);

    // condense, for edit
    int iSep = m_iMaxNameWidth;
    if ((lpDIS->itemState & ODS_COMBOBOXEDIT) == ODS_COMBOBOXEDIT)
    {
      iSep = sz.cx;
    }

    // sample in current font
    hf = (HFONT)dc.SelectFont(cf);
    dc.GetTextExtent(m_csSample, m_csSample.GetLength(), &sz);
    iPosY = (rc.Height() - sz.cy) / 2;
    COLORREF clr = dc.SetTextColor(clrSample);
    dc.TextOut(rc.left + iOffsetX + iSep + iOffsetX, rc.top + iPosY, m_csSample);
    dc.SetTextColor(clr);
    dc.SelectFont(hf);
  }
  break;
  case SAMPLE_THEN_NAME:
  {
    // sample in current font
    hf = (HFONT)dc.SelectFont(cf);
    dc.GetTextExtent(m_csSample, m_csSample.GetLength(), &sz);
    iPosY = (rc.Height() - sz.cy) / 2;
    COLORREF clr = dc.SetTextColor(clrSample);
    dc.TextOut(rc.left + iOffsetX, rc.top + iPosY, m_csSample);
    dc.SetTextColor(clr);
    dc.SelectFont(hf);

    // condense, for edit
    int iSep = m_iMaxSampleWidth;
    if ((lpDIS->itemState & ODS_COMBOBOXEDIT) == ODS_COMBOBOXEDIT)
    {
      iSep = sz.cx;
    }

    // font name in GUI font
    dc.GetTextExtent(csCurFontName, csCurFontName.GetLength(), &sz);
    iPosY = (rc.Height() - sz.cy) / 2;
    dc.TextOut(rc.left + iOffsetX + iSep + iOffsetX, rc.top + iPosY, csCurFontName);
  }
  break;
  case SAMPLE_ONLY:
  {
    // sample in current font
    hf = (HFONT)dc.SelectFont(cf);
    dc.GetTextExtent(m_csSample, m_csSample.GetLength(), &sz);
    iPosY = (rc.Height() - sz.cy) / 2;
    dc.TextOut(rc.left + iOffsetX, rc.top + iPosY, m_csSample);
    dc.SelectFont(hf);
  }
  break;
  }

  dc.RestoreDC(nIndexDC);

  dc.Detach();
}

/////////////////////////////////////////////////////////////////////////////

void CFontPreviewCombo::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
  // ok, how big is this ?

  CString csFontName;
  GetLBText(lpMeasureItemStruct->itemID, csFontName);

  CFont cf;
  if (!cf.CreateFont(m_iFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, csFontName))
  {
    ATLASSERT(0);
    return;
  }

  LOGFONT lf;
  cf.GetLogFont(&lf);

  if ((m_style == NAME_ONLY) || (m_style == SAMPLE_ONLY) || (m_style == NAME_GUI_FONT))
  {
    m_iMaxNameWidth = 0;
    m_iMaxSampleWidth = 0;
  }
  else
  {
    CClientDC dc(m_hWnd);

    // measure font name in GUI font
    HFONT hFont = ((HFONT)GetStockObject(DEFAULT_GUI_FONT));
    HFONT hf = (HFONT)dc.SelectFont(hFont);
    CSize sz;
    dc.GetTextExtent(csFontName, csFontName.GetLength(), &sz);
    m_iMaxNameWidth = max(m_iMaxNameWidth, sz.cx);
    dc.SelectFont(hf);

    // measure sample in cur font
    hf = (HFONT)dc.SelectFont(cf);
    if (hf)
    {
      dc.GetTextExtent(m_csSample, m_csSample.GetLength(), &sz);
      m_iMaxSampleWidth = max(m_iMaxSampleWidth, sz.cx);
      dc.SelectFont(hf);
    }
  }

  lpMeasureItemStruct->itemHeight = lf.lfHeight + 4;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CFontPreviewCombo::OnDropDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
  bHandled = FALSE;
  SetDropWidth();
  return 0;
}

void CFontPreviewCombo::SetDropWidth()
{
  int nScrollWidth = ::GetSystemMetrics(SM_CXVSCROLL);
  int nWidth = nScrollWidth;
  nWidth += GLYPH_WIDTH;

  switch (m_style)
  {
  case NAME_GUI_FONT:
    nWidth += m_iMaxNameWidth;
    break;
  case NAME_ONLY:
    nWidth += m_iMaxNameWidth;
    break;
  case NAME_THEN_SAMPLE:
    nWidth += m_iMaxNameWidth;
    nWidth += m_iMaxSampleWidth;
    nWidth += SPACING * 2;
    break;
  case SAMPLE_THEN_NAME:
    nWidth += m_iMaxNameWidth;
    nWidth += m_iMaxSampleWidth;
    nWidth += SPACING * 2;
    break;
  case SAMPLE_ONLY:
    nWidth += m_iMaxSampleWidth;
    break;
  }

  SetDroppedWidth(nWidth);
}

void
CFontPreviewCombo::AddFont(const CString& faceName)
{
  if (m_style != NAME_GUI_FONT)
  {
    HFONT cf =
      CreateFont(m_iFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE,
        FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, faceName);
    ATLASSERT(cf);
    m_fonts.SetAt(faceName, cf);
  }
}

void CFontPreviewCombo::SetFontHeight(int newHeight, bool reinitialize)
{
  if (newHeight == m_iFontHeight) return;
  m_iFontHeight = newHeight;
  if (reinitialize)
    Init(0);
}

int CFontPreviewCombo::GetFontHeight()
{
  return m_iFontHeight;
}

void CFontPreviewCombo::SetPreviewStyle(PreviewStyle style, bool reinitialize)
{
  if (style == m_style) return;
  m_style = style;
  if (reinitialize)
    Init(0);
}

int CFontPreviewCombo::GetPreviewStyle()
{
  return m_style;
}

void CFontPreviewCombo::DeleteAllFonts()
{
  POSITION i = m_fonts.GetStartPosition();
  HFONT font;
  CString dummy;
  while (i != NULL)
  {
    m_fonts.GetNextAssoc(i, dummy, font);
    DeleteObject(font);
  }
  m_fonts.RemoveAll();
}


