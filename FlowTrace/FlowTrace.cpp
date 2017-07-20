// FlowTrace.cpp : main source file for FlowTrace.exe
//

#include "stdafx.h"

#include "resource.h"

#include "FlowTraceView.h"
#include "aboutdlg.h"
#include "MainFrm.h"
#include "Helpers.h"

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

  CMainFrame* wndMain = new CMainFrame;

	if(wndMain->CreateEx() == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}
	wndMain->ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

  delete wndMain;

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
  //stdlog("hi\n");

  HRESULT hRes = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  ATLASSERT(SUCCEEDED(hRes));

  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    Helpers::SysErrMessageBox(TEXT("WSAStartup failed"));
    return 1;
  }

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
