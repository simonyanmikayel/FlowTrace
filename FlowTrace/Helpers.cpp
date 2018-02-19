#include "stdafx.h"
#include "Helpers.h"
#include "Resource.h"

namespace Helpers
{
  void CopyToClipboard(HWND hWnd, char* szText, int cbText)
  {
    if (!szText || !cbText)
      return;

    char *lock = 0;
    HGLOBAL clipdata = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, (cbText + 1) * sizeof(char));
    if (!clipdata)
      return;
    if (!(lock = (char*)GlobalLock(clipdata))) {
      GlobalFree(clipdata);
      return;
    }

    memcpy(lock, szText, cbText);
    memcpy(lock + cbText, "", 1);

    GlobalUnlock(clipdata);
    if (::OpenClipboard(hWnd)) {
      EmptyClipboard();
      SetClipboardData(CF_TEXT, clipdata);
      CloseClipboard();
    }
    else {
      GlobalFree(clipdata);
    }
  }

  void ErrMessageBox(CHAR* lpFormat, ...)
  {
    va_list vl;
    va_start(vl, lpFormat);

    CHAR* buf = new CHAR[1024];
    _vsntprintf_s(buf, 1023, 1023, lpFormat, vl);
    va_end(vl);

    ::PostMessage(hwndMain, WM_SHOW_NGS, (WPARAM)buf, (LPARAM)0);
  }

  void SysErrMessageBox(CHAR* lpFormat, ...)
  {
    DWORD err = GetLastError();
    CHAR *s = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, err,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      s, 0, NULL);

    va_list vl;
    va_start(vl, lpFormat);

    CHAR buf[1024];
    _vsntprintf_s(buf, _countof(buf), lpFormat, vl);

    ErrMessageBox(TEXT("%s\nErr: %d\n%s"), buf, err, s);
    va_end(vl);

    LocalFree(s);
  }

  typedef unsigned chartype;

  wchar_t char_table[256];
  static int initialised = 0;

  void init_stristr(void)
  {
    for (int i = 0; i < 256; i++)
    {
      char_table[i] = towupper(i);
    }
  }

  #define uppercase(_x) ( (chartype) (bMatchCase ? _x : (_x < 256 ? char_table[_x] : towupper(_x) )) )

  CHAR* find_str(const CHAR *phaystack, const CHAR *pneedle, int bMatchCase)
  {
    register const CHAR *haystack, *needle;
    register chartype b, c;

    if (!initialised)
    {
      initialised = 1;
      init_stristr();
    }

    if (!phaystack || !pneedle || !pneedle[0])
      goto ret0;

    haystack = (const CHAR *)phaystack;
    needle = (const CHAR *)pneedle;
    b = uppercase(*needle);

    haystack--;             /* possible ANSI violation */
    do
    {
      c = *++haystack;
      if (c == 0)
        goto ret0;
    } while (uppercase(c) != (int)b);

    c = *++needle; //dont pass ++needle to macros
    c = uppercase(c);
    if (c == '\0')
      goto foundneedle;

    ++needle;
    goto jin;

    for (;;)
    {
      register chartype a;
      register const CHAR *rhaystack, *rneedle;

      do
      {
        a = *++haystack;
        if (a == 0)
          goto ret0;
        if (uppercase(a) == (int)b)
          break;
        a = *++haystack;
        if (a == 0)
          goto ret0;
      shloop:
        ;
      } while (uppercase(a) != (int)b);

    jin:
      a = *++haystack;
      if (a == 0)
        goto ret0;

      if (uppercase(a) != (int)c)
        goto shloop;

      rhaystack = haystack-- + 1;
      rneedle = needle;

      a = uppercase(*rneedle);
      if (uppercase(*rhaystack) == (int)a)
        do
        {
          if (a == 0)
            goto foundneedle;

          ++rhaystack;
          a = *++needle;
          a = uppercase(a);
          if (uppercase(*rhaystack) != (int)a)
            break;

          if (a == '\0')
            goto foundneedle;

          ++rhaystack;
          a = *++needle;
          a = uppercase(a);
        } while (uppercase(*rhaystack) == (int)a);

        needle = rneedle;       /* took the register-poor approach */

        if (a == 0)
          break;
    }

  foundneedle:
    return (CHAR*)haystack;
  ret0:
    return 0;
  }

  CHAR* str_format_int_grouped(__int64 num)
  {
    static CHAR dst[16];
    CHAR src[16];
    char *p_src = src;
    char *p_dst = dst;

    const char separator = ',';
    int num_len, commas;

    num_len = sprintf_s(src, _countof(src), "%lld", num);

    if (*p_src == '-') {
      *p_dst++ = *p_src++;
      num_len--;
    }

    for (commas = 2 - num_len % 3; *p_src; commas = (commas + 1) % 3) {
      *p_dst++ = *p_src++;
      if (commas == 1) {
        *p_dst++ = separator;
      }
    }
    *--p_dst = '\0';
    return dst;
  }
  void GetTime(DWORD &sec, DWORD& msec)
  {
    //__int64 wintime; 
    //GetSystemTimeAsFileTime((FILETIME*)&wintime);
    //wintime -= 116444736000000000i64;  //1jan1601 to 1jan1970
    //sec = wintime / 10000000i64;           //seconds
    //nano_sec = wintime % 10000000i64 * 100;      //nano-second

    SYSTEMTIME st;
    GetLocalTime(&st);

    //sec = _time32(NULL);
    msec = st.wMilliseconds;

    tm local;
    memset(&local, 0, sizeof(local));

    local.tm_year = st.wYear - 1900;
    local.tm_mon = st.wMonth - 1;
    local.tm_mday = st.wDay;
    local.tm_wday = st.wDayOfWeek;
    local.tm_hour = st.wHour;
    local.tm_min = st.wMinute;
    local.tm_sec = st.wSecond;

    sec = (DWORD)(mktime(&local));
  }
  void SetMenuIcon(HMENU hMenu, UINT item, MENU_ICON icon)
  {
    static HBITMAP hbmpItem[MENU_ICON_MAX] = {0};

    if (icon >= MENU_ICON_MAX)
      return;
    if (hbmpItem[icon] == 0)
    {
      int iRes = -1;
      if (icon == MENU_ICON_SYNC)
        iRes = IDB_SYNC;
      else if (icon == MENU_ICON_FUNC_IN_ECLIPSE)
        iRes = IDB_FUNC_IN_ECLIPSE;
      else if (icon == MENU_ICON_CALL_IN_ECLIPSE)
        iRes = IDB_CALL_IN_ECLIPSE;
      if (iRes >= 0)
      {
        hbmpItem[icon] = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(iRes));
      }
    }
    MENUITEMINFO mii;
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_BITMAP;
    GetMenuItemInfo(hMenu, item, TRUE, &mii);
    mii.hbmpItem = hbmpItem[icon];
    SetMenuItemInfo(hMenu, item, TRUE, &mii);
  }

};

