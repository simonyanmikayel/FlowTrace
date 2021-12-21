#include "stdafx.h"
#include "SearchInfo.h"
#include "Helpers.h"

SEARCH_INFO searchInfo;

char* SEARCH_INFO::find(CHAR* p)
{
  const CHAR* szText = SearchText();
  size_t cbText = SearchTextSize();
  p = Helpers::find_str(p, SearchText(), bMatchCase);
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
    p += searchInfo.SearchTextSize();
  }
  return ret;
}

