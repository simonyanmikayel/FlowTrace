#pragma once

struct SEARCH_INFO
{
  SEARCH_INFO() { bMatchCase = bMatchWholeWord = false; ClearSearchResults(); }
  HWND hwndEdit;
  bool bMatchCase;
  bool bMatchWholeWord;
  int curLine, firstLine, lastLine, total, posInCur, listCount;
  char* find(CHAR* p);
  int calcCountIn(CHAR* p);
  void ClearSearchResults() { curLine = -1; firstLine = lastLine = total = posInCur = listCount = 0; };
  void setSerachText(char* text) { m_strFind = text; m_cbText = m_strFind.GetLength(); }
  size_t cbText() { return m_cbText; }
  //CHAR* getSerachText() { return m_strFind.GetBuffer(); }
private:
  CStringA  m_strFind;
  size_t m_cbText;
};
extern SEARCH_INFO searchInfo;

