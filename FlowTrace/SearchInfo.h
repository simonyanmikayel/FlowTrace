#pragma once

struct SEARCH_INFO
{
  SEARCH_INFO() { bMatchCase = bMatchWholeWord = false; ClearSearchResults(); }
  HWND hwndEdit;
  bool bMatchCase;
  bool bMatchWholeWord;
  size_t cbText;
  int curLine, firstLine, lastLine, total, posInCur, cur;
  char* find(CHAR* p);
  int calcCountIn(CHAR* p);
  void ClearSearchResults() { curLine = firstLine = lastLine = total = posInCur = cur = 0; };
  void setSerachText(char* text) { m_strFind = text; cbText = m_strFind.GetLength(); }
  //CHAR* getSerachText() { return m_strFind.GetBuffer(); }
private:
  CStringA  m_strFind;
};
extern SEARCH_INFO searchInfo;

