#include "stdafx.h"
#include <string.h>
#include <stdio.h>

#pragma warning(disable:4996)


#ifndef _STDLOG
void stdlog(const char* lpFormat, ...)  {  }
void Trace(const char* pszFormat, ...)  {  }
void stdclear()  {  }
#else
typedef HWND(WINAPI *GETCONSOLEWINDOW)();
static HANDLE ghStdOut = NULL;
static FILE* gFileStdOut = NULL;
HWND hConsolWnd = 0;
GETCONSOLEWINDOW gpGetConsoleWindow = NULL;
BOOL bGetConsoleWindowChecked = FALSE;

void GetStdOut()
{
  if (!bGetConsoleWindowChecked)
  {
    HINSTANCE hUser32Lib = GetModuleHandleA("Kernel32.dll");
    gpGetConsoleWindow = (GETCONSOLEWINDOW)GetProcAddress(hUser32Lib, "GetConsoleWindow");
    bGetConsoleWindowChecked = TRUE;
  }

  if (!ghStdOut)
  {
    DWORD dwErr = 0;
    if (!AllocConsole())
    {
      dwErr = GetLastError();
      if (dwErr == 5) // acess denied ( already created for process)
        dwErr = 0;
    }

    if (!dwErr)
    {
      ghStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
      if (ghStdOut != INVALID_HANDLE_VALUE)
      {
        if (gpGetConsoleWindow)
        {
          RECT  rc;
          SetRect(&rc, 0, 0, 400, 320); // set default values
          hConsolWnd = gpGetConsoleWindow();
          SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0); rc.right /= 2;
          MoveWindow(hConsolWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        }

        COORD c = { 1000, 9999 };
        SetConsoleScreenBufferSize(ghStdOut, c);

        CHAR ModuleFileName[MAX_PATH + 1]; // name of the application
        if (0 == GetModuleFileNameA(NULL, ModuleFileName, sizeof(ModuleFileName)))
          SetConsoleTitle(TEXT("Dbg"));
        else
        {
          CHAR *pSlash = _tcsrchr(ModuleFileName, TEXT('/'));
          if (!pSlash)
            pSlash = _tcsrchr(ModuleFileName, TEXT('\\'));
          if (pSlash)
            pSlash++;

          if (!pSlash)
            pSlash = ModuleFileName;
          SetConsoleTitle(pSlash);
          SetConsoleOutputCP(CP_UTF8);
          gFileStdOut = fopen("CON", "w");
          ////////////////////////////////////////////
          CONSOLE_FONT_INFOEX cfi = { sizeof(cfi) };
          GetCurrentConsoleFontEx(ghStdOut, FALSE, &cfi);
          cfi.dwFontSize.X = cfi.dwFontSize.Y = 14;
          wcscpy_s(cfi.FaceName, ARRAYSIZE(cfi.FaceName), L"Lucida Console"); //sylfaen / Lucida Console
          SetCurrentConsoleFontEx(ghStdOut, FALSE, &cfi);
          ////////////////////////////////////////////
        }
      }
    }
  }
}

void stdlog(const char* lpFormat, ...)
{
  GetStdOut();
  if (gFileStdOut != NULL)
  {
    static DWORD nNom;
    static DWORD gdwLastPrintTick = 0;
    DWORD dwTick = GetTickCount();
    if (!gdwLastPrintTick)
      gdwLastPrintTick = GetTickCount();

    char chFlags = *lpFormat;
    if (chFlags > 0 && chFlags < 8)
    {
      lpFormat++;
    }
    else
    {
      //fprintf(gFileStdOut, "% 5u % 7u %ud ", ++nNom, dwTick - gdwLastPrintTick, GetCurrentThreadId());
      //fprintf(gFileStdOut, "% 5u % 7u ", ++nNom, dwTick - gdwLastPrintTick);
      fprintf(gFileStdOut, "%5u      ", ++nNom);
      gdwLastPrintTick = dwTick;
    }


    va_list vl;
    va_start(vl, lpFormat);
    vfprintf(gFileStdOut, lpFormat, vl);
    va_end(vl);
    fflush(gFileStdOut);
  }
}

void Trace(const char* pszFormat, ...)
{
  char szOutput[1024];
  va_list	vaList;

  va_start(vaList, pszFormat);
  vsprintf_s(szOutput, _countof(szOutput), pszFormat, vaList); //_countof(szOutput), 
  OutputDebugStringA(szOutput);
}

void stdclear()
{
  std::system("cls");
}


#endif
