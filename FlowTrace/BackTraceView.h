#pragma once

#include "Archive.h"
#include "Helpers.h"

#define MAX_BACK_TRACE 512
enum BACK_TRACE_COL { BACK_TRACE_FN, BACK_TRACE_LINE, BACK_TRACE_SRC, BACK_TRACE_LAST_COL };

class CFlowTraceView;
#ifdef _USE_LIST_VIEW_FOR_BACK_TRACE
class CBackTraceView : public CWindowImpl< CBackTraceView, CListViewCtrl>
#else
#ifdef _USE_RICH_EDIT_FOR_BACK_TRACE
class CBackTraceView : public CWindowImpl< CBackTraceView, CRichEditCtrl>
#else
class CBackTraceView : public CWindowImpl< CBackTraceView, CEdit>
#endif
#endif
{
public:
  CBackTraceView(CFlowTraceView* pView);
  ~CBackTraceView();

  void ClearTrace();
  void UpdateTrace(LOG_NODE* pSelectedNode);
  void CopySelection();
  void CopySelection(bool all);

#ifdef _USE_LIST_VIEW_FOR_BACK_TRACE
  BEGIN_MSG_MAP(CBackTraceView)
    MSG_WM_SIZE(OnSize)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown);
  END_MSG_MAP()
  void ItemPrePaint(int iItem, HDC hdc, RECT rc);
  void DrawSubItem(int iItem, int iSubItem, HDC hdc, RECT rc);
  void OnSize(UINT nType, CSize size);
  LOG_NODE* nodes[MAX_BACK_TRACE];
  int subItemColWidth[BACK_TRACE_LAST_COL];
  void SetSelectionOnMouseEven(UINT uMsg, WPARAM wParam, LPARAM lParam);
  LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  LRESULT OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
  LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
  int getSubItemText(int iItem, int iSubItem, char* buf, int cbBuf);
  LOG_NODE* GetSelectedNode() { return (selItem >= 0) ? nodes[selItem] : NULL; }
  //void ResetColWidth(int iItem);
#else
  BEGIN_MSG_MAP(CBackTraceView)
  END_MSG_MAP()
#endif

private:
  int selItem;
  int selSubItem;
  int c_nodes;
  bool m_Initialised;
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
