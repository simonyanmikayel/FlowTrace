#pragma once

#include "Archive.h"
#include "Helpers.h"

class CFlowTraceView;

#ifdef _USE_RICH_EDIT
class CBackTraceView : public CWindowImpl< CBackTraceView, CRichEditCtrl>
#else
class CBackTraceView : public CWindowImpl< CBackTraceView, CEdit>
#endif
{
public:
  CBackTraceView(CFlowTraceView* pView);
  ~CBackTraceView();

  BEGIN_MSG_MAP(CBackTraceView)
  END_MSG_MAP()


private:
  CFlowTraceView* m_pView;
};

//CRichEditCtrl
//static DWORD CALLBACK MyStreamInCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
//{
//  char** pTxt = (char**)dwCookie;
//  int c = strlen(*pTxt);
//  c = min(c, cb);
//  if (c)
//  {
//    strcpy((char*)pbBuff, *pTxt);
//  }
//  *pTxt += c;
//  *pcb = c;
//
//  return 0;
//}

//m_wndBackTraceView.SetEventMask(ENM_LINK);
//m_wndBackTraceView.SetAutoURLDetect(TRUE);
//char* txt = R"({\rtf1{\field{\*\fldinst{ HYPERLINK http://example.com  }}{\fldrslt{DisplayText}}}}.)";
//EDITSTREAM es;
//es.dwCookie = (DWORD)&txt;
//es.pfnCallback = MyStreamInCallback;
//m_wndBackTraceView.StreamIn(SF_RTF, es);
//return;

/* PADDING EXAMPLE
int targetStrLen = 10;           // Target output length
const char *myString="Monkey";   // String for output
const char *padding="#####################################################";
 
int padLen = targetStrLen - strlen(myString); // Calc Padding length
if(padLen < 0) padLen = 0;    // Avoid negative length

printf("[%*.*s%s]", padLen, padLen, padding, myString);  // LEFT Padding
printf("[%s%*.*s]", myString, padLen, padLen, padding);  // RIGHT Padding
*/
