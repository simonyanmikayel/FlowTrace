#pragma once

struct SEARCH_INFO
{
	SEARCH_INFO() { bSearchFilterOn = false;  bMatchCase = bMatchWholeWord = false; ClearSearchResults(); }
  HWND hwndEdit;
  bool bMatchCase;
  bool bMatchWholeWord;
  bool bSearchFilterOn;
  DWORD archiveCount;
  int curLine, firstLine, lastLine, total, posInCur;
  char* find(CHAR* p);
  int calcCountIn(CHAR* p);
  void ClearSearchResults() { curLine = -1; firstLine = lastLine = total = posInCur = archiveCount = 0; };
  void setSerachText(char* text) {
	  m_cbText = std::min<size_t>(strlen(text), _countof(szSearchText) - 1);
	  memcpy(szSearchText, text, m_cbText);
	  szSearchText[m_cbText] = 0;
  }
  size_t SearchTextSize() { return m_cbText; }
  const CHAR* SearchText() { return szSearchText; }
  bool applySearchFilter() { return bSearchFilterOn && total > 0; }
private:
  CHAR szSearchText[256];
  size_t m_cbText;

};
extern SEARCH_INFO searchInfo;

