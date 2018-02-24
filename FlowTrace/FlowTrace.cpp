// FlowTrace.cpp : main source file for FlowTrace.exe
//

#include "stdafx.h"

#include "resource.h"

#include "FlowTraceView.h"
#include "aboutdlg.h"
#include "MainFrm.h"
#include "Helpers.h"

CAppModule _Module;
CMainFrame* gMainFrame;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);


    gMainFrame = new CMainFrame;

	if(gMainFrame->CreateEx() == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}
    gMainFrame->ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

  delete gMainFrame;

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
#ifdef _USE_RICH_EDIT_FOR_BACK_TRACE
  HINSTANCE hInstRich = ::LoadLibrary(CRichEditCtrl::GetLibraryName());
  ATLASSERT(hInstRich != NULL);
#endif
	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
