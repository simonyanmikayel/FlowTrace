// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

// Change these values to use different versions

#define WINVER        0x0601
#define _WIN32_WINNT  0x0601 // Windows 7

#define _WIN32_IE     0x0501
#define _RICHEDIT_VER 0x0500

#define _HAS_EXCEPTIONS 0 //Better than compiling your code with /EHsc if you don't plan on adding exception handling.

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <HtmlHelp.h>
#include <ShellApi.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <winuser.h>
#include <Muiload.h>

//////////////////////////////////////////////////////////////////////////////
#include <atlbase.h>
#include <atlcoll.h>
#include <atlstr.h>
#include <atltypes.h>

#define _WTL_NO_CSTRING
#define _WTL_NO_WTYPES
#pragma warning(push)
#pragma warning(disable: 4091 4302 4458 4838 4996)
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <atlcrack.h>
#include <atlddx.h>
#include <atlmisc.h>
#include <atlsplit.h>

#include <atltheme.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlctrlx.h>
#pragma warning(pop)

#include <userenv.h>
#include <Lm.h>

#pragma warning(push)
#pragma warning(disable: 4091 4189 4267 4458 4838)
#include <atlgdix.h>
#include <gridctrl.h>

#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#include "CustomTabCtrl.h"
#include "DotNetTabCtrl.h"
#include "TabbedFrame.h"
//#include "multisplit.h"

#pragma warning(pop)


#if _MSC_VER >= 1900
/*
Muiload.lib(muiload.obj) : error LNK2019: unresolved external symbol _vsnwprintf referenced in function
"long __cdecl StringVPrintfWorkerW(unsigned short *,unsigned __int64,unsigned __int64 *,unsigned short const *,char *)" (?StringVPrintfWorkerW@@YAJPEAG_KPEA_KPEBGPEAD@Z)

The definitions of all of the printf and scanf functions have been moved inline into <stdio.h>, <conio.h>,
and other CRT headers.This is a breaking change that leads to a linker error(LNK2019, unresolved external
symbol) for any programs that declared these functions locally without including the appropriate CRT
headers.If possible, you should update the code to include the CRT headers(that is, add #include <stdio.h>)
and the inline functions, but if you do not want to modify your code to include these header files,
an alternative solution is to add an additional library to your linker input, legacy_stdio_definitions.lib.
*/
#pragma comment (lib, "legacy_stdio_definitions.lib")
#endif

//#include <shobjidl.h>

#ifndef _USING_V110_SDK71_
//#include <ShellScalingAPI.h>
#endif

#pragma warning(disable: 4503) // disables 'name truncated' warnings

#pragma warning(push)
#pragma warning(disable: 4702)

#include <algorithm> 
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <stack>
#include <array>
#include <regex>
#include <chrono>
#include <memory>
#include <list>

using namespace std;
#pragma warning(pop)

//////////////////////////////////////////////////////////////////////////////
// Memory allocation tracking

#ifdef _DEBUG
#include <crtdbg.h>
void* __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine);
#define DEBUG_NEW new(__FILE__, __LINE__)

void __cdecl operator delete(void* p, LPCSTR lpszFileName, int nLine);
#endif


//////////////////////////////////////////////////////////////////////////////
// trace function and TRACE macro

#include "StdLog.h"

#ifndef DEBUG
#undef ATLASSERT
#define ATLASSERT
#endif

#ifdef  UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

#undef min
#undef max

//#pragma warning(disable:4996) //This function or variable may be unsafe

//////////////////////////////////////////////////////////////////////////////
#define _AUTO_TEST
//#define _LOSE_TEST
//#define _NN_TEST
