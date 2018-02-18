#include "stdafx.h"
#include "SearchInfo.h"
#include "Helpers.h"

SEARCH_INFO searchInfo;

char* SEARCH_INFO::find(CHAR* p)
{
  CHAR* szText = m_strFind.GetBuffer();
  int cbText = m_strFind.GetLength();
  p = Helpers::find_str(p, m_strFind.GetBuffer(), bMatchCase);
  if (p && (!bMatchWholeWord || ((p == szText || !isalpha(*(p - 1))) && !isalpha(*(p + cbText)))))
    return p;
  else
    return NULL;
}

int SEARCH_INFO::calcCountIn(CHAR* p)
{
  int ret = 0;
  while (p = searchInfo.find(p))
  {
    ret++;
    p += searchInfo.cbText;
  }
  return ret;
}

