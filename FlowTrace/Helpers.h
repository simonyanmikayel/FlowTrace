#pragma once

extern HWND hwndMain;
#define WM_UPDATE_FILTER               WM_USER + 1000
#define WM_INPORT_TASK                 WM_USER + 1001
#define WM_MOVE_SELECTION_TO_END       WM_USER + 1002

static const int ICON_LEN = 16;
static const int ICON_OFFSET = 16 + 4;

namespace Helpers
{
  void SysErrMessageBox(TCHAR* lpFormat, ...);
  void ErrMessageBox(TCHAR* lpFormat, ...);
  TCHAR* find_str(const TCHAR *phaystack, const TCHAR *pneedle, int bMatchCase);  
  TCHAR* str_format_int_grouped(int num);
  void GetTime(DWORD &sec, DWORD& msec);
};