#include "StdAfx.h"
#include "Settings.h"
#include "resource.h"
#include "LogData.h"

CSettings gSettings;

// registry keys and values
LPCTSTR STR_APP_REG_KEY = _T("Software\\TermianlTools\\FlowTrace");
LPCTSTR STR_APP_REG_VAL_WINDOWPOS = _T("WindowPos");
LPCTSTR STR_APP_REG_VAL_VERT_SPLITTER_POS = _T("VertSplitterPos");
LPCTSTR STR_APP_REG_VAL_HORZ_SPLITTER_POS = _T("HorzSplitterPos");
LPCTSTR STR_APP_REG_VAL_STACK_SPLITTER_POS = _T("StackSplitterPos");

LPCTSTR STR_APP_REG_VAL_FONTNAME = _T("FontName_1");
LPCTSTR STR_APP_REG_VAL_FONTWIDTH = _T("FontWeight");
LPCTSTR STR_APP_REG_VAL_FONTSIZE = _T("FontSize");
//LPCTSTR STR_APP_REG_VAL_BK_COLOR = _T("BkColor");
//LPCTSTR STR_APP_REG_VAL_TEXT_COLOR = _T("TextColor");
//LPCTSTR STR_APP_REG_VAL_INFO_TEXT_COLOR = _T("InfoTextColor");
//LPCTSTR STR_APP_REG_VAL_SEL_COLOR = _T("SelColor");
//LPCTSTR STR_APP_REG_VAL_BK_SEL_COLOR = _T("BkSelColor");
//LPCTSTR STR_APP_REG_VAL_SEARCH_COLOR = _T("SearchColor");
//LPCTSTR STR_APP_REG_VAL_CUR_SEARCH_COLOR = _T("CurSearchColor");
//LPCTSTR STR_APP_REG_VAL_SYNC_COLOR = _T("SyncColor");
LPCTSTR STR_APP_REG_VAL_SEARCH_LIST = _T("SearchList");

LPCTSTR STR_APP_REG_VAL_UDP_PORT = _T("UdpPort");
LPCTSTR STR_APP_REG_VAL_ADB_ARG = _T("AdbArg");

LPCTSTR STR_APP_REG_VAL_ColNN = _T("ColNN");
LPCTSTR STR_APP_REG_VAL_ColApp = _T("ColApp");
LPCTSTR STR_APP_REG_VAL_ColPID = _T("ColPID");
LPCTSTR STR_APP_REG_VAL_ColTID = _T("ColTID");
LPCTSTR STR_APP_REG_VAL_ColThreadNN = _T("ColThreadNN");
LPCTSTR STR_APP_REG_VAL_ShowChildCount = _T("ShowChildCount");
LPCTSTR STR_APP_REG_VAL_ColFunc = _T("ColFunc");
LPCTSTR STR_APP_REG_VAL_ColLine = _T("ColLine");
LPCTSTR STR_APP_REG_VAL_ColTime = _T("ColTime");
LPCTSTR STR_APP_REG_VAL_ColCallAddr = _T("ColCallAddr");
LPCTSTR STR_APP_REG_VAL_FnCallLine = _T("FnCallLine");
LPCTSTR STR_APP_REG_VAL_UseAdb = _T("UseAdb");
LPCTSTR STR_APP_REG_VAL_RestartAdb = _T("RestartAdb");
LPCTSTR STR_APP_REG_VAL_ApplyLogcutFilter = _T("ApplyLogcutFilter");

LPCTSTR STR_APP_REG_VAL_FLOW_TRACE_HIGEN = _T("FlowTraceHiden");
LPCTSTR STR_APP_REG_VAL_TREE_VIEW_HIDEN = _T("TreeViewHiden");
LPCTSTR STR_APP_REG_VAL_INFO_HIDEN = _T("InfoHiden");
LPCTSTR STR_APP_REG_VAL_SHOW_APP_IP = _T("ShowAppIp");
LPCTSTR STR_APP_REG_VAL_SHOW_ELAPSED_TIME = _T("ShowElapsedTime");
LPCTSTR STR_APP_REG_VAL_RESOLVE_ADDR = _T("ResolveAddr");
LPCTSTR STR_APP_REG_VAL_FULL_SRC_PATH = _T("FullSrcPath");

LPCTSTR STR_APP_REG_VAL_UpdateStack = _T("UpdateStack");
LPCTSTR STR_APP_REG_VAL_EclipsePath = _T("EclipsePath");
LPCTSTR STR_APP_REG_VAL_LinuxHome = _T("LinuxHome");
LPCTSTR STR_APP_REG_VAL_MapOnWin = _T("MapOnWin");
LPCTSTR STR_APP_REG_VAL_AndroidStudio = _T("AndroidStudio");
LPCTSTR STR_APP_REG_VAL_AndroidProject = _T("AndroidProject");
LPCTSTR STR_APP_REG_VAL_PrefModulePath = _T("PrefModulePath");
LPCTSTR STR_APP_REG_VAL_ExternalCmd_1 = _T("ExternalCmd_1");
LPCTSTR STR_APP_REG_VAL_ExternalCmd_2 = _T("ExternalCmd_2");
LPCTSTR STR_APP_REG_VAL_ExternalCmd_3 = _T("ExternalCmd_3");
LPCTSTR STR_APP_REG_VAL_Modules = _T("Modules");
LPCTSTR STR_APP_REG_VAL_Filters = _T("Filters");
LPCTSTR STR_APP_REG_VAL_ApplyPorcessFilter = _T("ApplyPorcessFilter");
LPCTSTR STR_APP_REG_VAL_LogcutFilter = _T("LogcutFilter1");

//#define DefBkColor RGB(0,0,0)
//#define DefTextColor RGB(176,176,176)
//#define DefInfoTextColor RGB(128,128,128)
//#define DefSelColor  RGB(192,192,192)
//#define DefBkSelColor  RGB(64, 122, 255) //RGB(232,232,255) //RGB(64, 122, 255) //RGB(0xEC,0xEC,0xEC)
//#define DefSyncColor  RGB(0,255,0)
//#define DefSerachColor  RGB(0xA0,0xA9,0x3d)
//#define DefCurSerachColor  RGB(64,128,64)

#define DefUdpPort  8888
#define DefFontSize 11

static CHAR* DEF_FONT_NAME = _T("Consolas"); //Courier New //Consolas //Inconsolata

CSettings::CSettings() :
	CRegKeyExt(STR_APP_REG_KEY)
	, m_processFilterList(this, STR_APP_REG_VAL_Filters, 3*1024, 128, "\n")
	, m_adbFilterList(this, STR_APP_REG_VAL_LogcutFilter, 512, 48, " ")
	, m_Font(NULL)
{
	Read(STR_APP_REG_VAL_VERT_SPLITTER_POS, m_VertSplitterPos, 50);
	Read(STR_APP_REG_VAL_HORZ_SPLITTER_POS, m_HorzSplitterPos, 50);
	Read(STR_APP_REG_VAL_STACK_SPLITTER_POS, m_StackSplitterPos, 50);
	Read(STR_APP_REG_VAL_FLOW_TRACE_HIGEN, m_FlowTracesHiden, TRUE);
	Read(STR_APP_REG_VAL_TREE_VIEW_HIDEN, m_TreeViewHiden, FALSE);
	Read(STR_APP_REG_VAL_INFO_HIDEN, m_InfoHiden, FALSE);
	Read(STR_APP_REG_VAL_SHOW_APP_IP, m_ShowAppIp, FALSE);
	Read(STR_APP_REG_VAL_SHOW_ELAPSED_TIME, m_ShowElapsedTime, FALSE);
	Read(STR_APP_REG_VAL_RESOLVE_ADDR, m_ResolveAddr, TRUE);
	m_UsePrefModule = FALSE;
	Read(STR_APP_REG_VAL_FULL_SRC_PATH, m_FullSrcPath, FALSE);

	Read(STR_APP_REG_VAL_ColNN, m_ColNN);
	Read(STR_APP_REG_VAL_ColApp, m_ColApp, 1);
	Read(STR_APP_REG_VAL_ColPID, m_ColPID);
	Read(STR_APP_REG_VAL_ColTID, m_ColTID);
	Read(STR_APP_REG_VAL_ColThreadNN, m_ColThreadNN);
	Read(STR_APP_REG_VAL_ShowChildCount, m_ShowChildCount);
	Read(STR_APP_REG_VAL_ColFunc, m_ColFunc, 1);
	Read(STR_APP_REG_VAL_ColLine, m_ColLine, 1);
	Read(STR_APP_REG_VAL_ColTime, m_ColTime);
	Read(STR_APP_REG_VAL_ColCallAddr, m_ColCallAddr);
	Read(STR_APP_REG_VAL_FnCallLine, m_FnCallLine);
	Read(STR_APP_REG_VAL_UseAdb, m_UseAdb, 1);
	Read(STR_APP_REG_VAL_RestartAdb, m_RestartAdb, 1);
	Read(STR_APP_REG_VAL_ApplyLogcutFilter, m_ApplyLogcutFilter, 1);
	Read(STR_APP_REG_VAL_ApplyPorcessFilter, m_ApplyPorcessFilter, 1);

	Read(STR_APP_REG_VAL_EclipsePath, m_EclipsePath, sizeof(m_EclipsePath), "");
	Read(STR_APP_REG_VAL_ExternalCmd_1, m_ExternalCmd_1, sizeof(m_ExternalCmd_1), "");
	Read(STR_APP_REG_VAL_ExternalCmd_2, m_ExternalCmd_2, sizeof(m_ExternalCmd_2), "");
	Read(STR_APP_REG_VAL_ExternalCmd_3, m_ExternalCmd_3, sizeof(m_ExternalCmd_3), "");
	Read(STR_APP_REG_VAL_LinuxHome, m_LinuxHome, sizeof(m_LinuxHome), "");
	Read(STR_APP_REG_VAL_MapOnWin, m_MapOnWin, sizeof(m_MapOnWin), "");
	Read(STR_APP_REG_VAL_AndroidStudio, m_AndroidStudio, sizeof(m_AndroidStudio), "");
	Read(STR_APP_REG_VAL_AndroidProject, m_AndroidProject, sizeof(m_AndroidProject), "");

	//Read(STR_APP_REG_VAL_BK_COLOR, m_BkColor, DefBkColor);
	//Read(STR_APP_REG_VAL_TEXT_COLOR, m_TextColor, DefTextColor);
	//Read(STR_APP_REG_VAL_INFO_TEXT_COLOR, m_InfoTextColor, DefInfoTextColor);
	//Read(STR_APP_REG_VAL_SEL_COLOR, m_SelColor, DefSelColor);
	//Read(STR_APP_REG_VAL_BK_SEL_COLOR, m_BkSelColor, DefSyncColor);
	//Read(STR_APP_REG_VAL_SYNC_COLOR, m_BkSelColor, DefSyncColor);
	//Read(STR_APP_REG_VAL_BK_SEL_COLOR, m_BkSelColor, DefBkSelColor);
	//Read(STR_APP_REG_VAL_SEARCH_COLOR, m_SerachColor, DefSerachColor);
	//Read(STR_APP_REG_VAL_CUR_SEARCH_COLOR, m_CurSerachColor, DefCurSerachColor);

	Read(STR_APP_REG_VAL_UDP_PORT, m_UdpPort, DefUdpPort);
	Read(STR_APP_REG_VAL_ADB_ARG, m_AdbArg, sizeof(m_AdbArg), "");

	InitFont();
}

CSettings::~CSettings()
{
	DeleteFont();
	RemoveFontMemResourceEx(m_resourceFonthandle);
}

void CSettings::AddDefaultFont()
{
	HINSTANCE hResInstance = _Module.m_hInst;

	HRSRC res = FindResource(hResInstance, MAKEINTRESOURCE(IDR_FONT1), _T("FONTS"));
	if (res)
	{
		HGLOBAL mem = LoadResource(hResInstance, res);
		void *data = LockResource(mem);
		DWORD len = SizeofResource(hResInstance, res);

		DWORD nFonts;
		m_resourceFonthandle = AddFontMemResourceEx(
			data,       // font resource
			len,       // number of bytes in font resource 
			NULL,          // Reserved. Must be 0.
			&nFonts      // number of fonts installed
		);
	}
}

void CSettings::InitFont()
{
	DeleteFont();

	ZeroMemory(&m_logFont, sizeof(LOGFONT));

	Read(STR_APP_REG_VAL_FONTNAME, m_logFont.lfFaceName, LF_FACESIZE, DEF_FONT_NAME);
	Read(STR_APP_REG_VAL_FONTWIDTH, m_logFont.lfWeight, FW_NORMAL);
	Read(STR_APP_REG_VAL_FONTSIZE, m_FontSize, DefFontSize);

	m_FontName = m_logFont.lfFaceName;
	m_FontWeight = m_logFont.lfWeight;

	if (m_FontSize <= 4) m_FontSize = DefFontSize;

	m_logFont.lfWeight = (m_logFont.lfWeight <= FW_MEDIUM) ? FW_NORMAL : FW_BOLD;

	HDC hdc = CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);
	m_logFont.lfHeight = -MulDiv(m_FontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	m_logFont.lfQuality = CLEARTYPE_NATURAL_QUALITY; //ANTIALIASED_QUALITY

	m_Font = CreateFontIndirect(&m_logFont);

	TEXTMETRIC  tm;
	SelectObject(hdc, m_Font);
	GetTextMetrics(hdc, &tm);
	m_FontHeight = tm.tmHeight + tm.tmExternalLeading;
	m_FontWidth = tm.tmAveCharWidth;

	DeleteDC(hdc);
}

void CSettings::SetUIFont(CHAR* lfFaceName, LONG lfWeight, LONG fontSize)
{
	Write(STR_APP_REG_VAL_FONTNAME, lfFaceName);
	Write(STR_APP_REG_VAL_FONTWIDTH, lfWeight);
	Write(STR_APP_REG_VAL_FONTSIZE, fontSize);
	InitFont();
}

void CSettings::DeleteFont()
{
	if (m_Font) {
		DeleteObject(m_Font);
		m_Font = NULL;
	}
}

void CSettings::RestoreWindPos(HWND hWnd)
{
	WINDOWPLACEMENT wpl;
	if (Read(STR_APP_REG_VAL_WINDOWPOS, &wpl, sizeof(wpl))) {
		RECT rcWnd = wpl.rcNormalPosition;
		int cx, cy, x, y;
		cx = rcWnd.right - rcWnd.left;
		cy = rcWnd.bottom - rcWnd.top;
		x = rcWnd.left;
		y = rcWnd.top;

		// Get the monitor info
		MONITORINFO monInfo;
		HMONITOR hMonitor = ::MonitorFromPoint(CPoint(x, y), MONITOR_DEFAULTTONEAREST);
		monInfo.cbSize = sizeof(MONITORINFO);
		if (::GetMonitorInfo(hMonitor, &monInfo))
		{
			// Adjust for work area
			x += monInfo.rcWork.left - monInfo.rcMonitor.left;
			y += monInfo.rcWork.top - monInfo.rcMonitor.top;
			// Ensure top left point is on screen
			if (CRect(monInfo.rcWork).PtInRect(CPoint(x, y)) == FALSE)
			{
				x = monInfo.rcWork.left;
				y = monInfo.rcWork.top;
			}
		}
		else
		{
			RECT rcScreen;
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0);

			cx = min(rcScreen.right, (LONG)cx);
			cy = min(rcScreen.bottom, (LONG)cy);
			x = max(0L, min((LONG)x, rcScreen.right - cx));
			y = max(0L, min((LONG)y, rcScreen.bottom - cy));
		}

		::SetWindowPos(hWnd, 0, x, y, cx, cy, SWP_NOZORDER);

		if (wpl.flags & WPF_RESTORETOMAXIMIZED)
		{
			//::ShowWindow(hWnd, SW_SHOWMAXIMIZED);
			::PostMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
		}
	}
}

void CSettings::SaveWindPos(HWND hWnd)
{
	WINDOWPLACEMENT wpl = { sizeof(WINDOWPLACEMENT) };
	if (::GetWindowPlacement(hWnd, &wpl))
		Write(STR_APP_REG_VAL_WINDOWPOS, &wpl, sizeof(wpl));
}

bool CSettings::SetTraceColor(int color, DWORD& textColor, DWORD& bkColor)
{
	bool ret = true;
	//return ret;
	if (color >= 30 && color <= 37)
	{
		ret = true;
		//if (color == 30) {
		//	textColor = RGB(0, 0, 0);//30	Black
		//	bkColor = RGB(255, 255, 255);
		//}
		//else 
		if (color == 31)
			textColor = RGB(255, 0, 0);//31	Red
		else if (color == 32)
			textColor = RGB(0, 255, 0);//32	Green
		else if (color == 33)
			textColor = RGB(255, 255, 0);//33	Yellow
		else if (color == 34)
			textColor = RGB(0, 0, 255);//34	Blue
		else if (color == 35)
			textColor = RGB(255, 0, 255);//35	Magenta
		else if (color == 36)
			textColor = RGB(0, 255, 255);//36	Cyan
		else if (color == 37)
			textColor = RGB(128, 128, 128);//37	Light gray
	}
	else if (color >= 40 && color <= 47)
	{
		//if (color == 40) {
		//	bkColor = RGB(0, 0, 0);//40	Black
		//	textColor = RGB(255, 255, 255);
		//}
		//else 
		if (color == 41)
			textColor = RGB(255, 0, 0);//41	Red
		else if (color == 42)
			textColor = RGB(0, 255, 0);//42	Green
		else if (color == 43)
			textColor = RGB(255, 255, 0);//43	Yellow
		else if (color == 44)
			textColor = RGB(0, 0, 255);//44	Blue
		else if (color == 45)
			textColor = RGB(255, 0, 255);//45	Magenta
		else if (color == 46)
			textColor = RGB(0, 255, 255);//46	Cyan
		else if (color == 47)
			textColor = RGB(128, 128, 128);//47	Light gray
	}
	else
	{
		ret = false;
	}
	return ret;
}

static const int MAX_MODULES = 56 * 256;
static CHAR szModuls[MAX_MODULES + 1];
void CSettings::SetModules(const CHAR* szList)
{
	size_t n = _tcslen(szList);
	n = min((size_t)MAX_MODULES, n);
	memcpy(szModuls, szList, n);
	szModuls[n] = 0;
	Write(STR_APP_REG_VAL_Modules, szModuls);
}
CHAR* CSettings::GetModules()
{
#ifdef _DEBUG
	//ZeroMemory(szModuls, sizeof(szModuls));
#endif
	if (!Read(STR_APP_REG_VAL_Modules, szModuls, MAX_MODULES))
	{
		szModuls[0] = 0;
	}
	return szModuls;
}

static const int MAX_SEARCH_LIST = 2*1024;
static CHAR searchList[MAX_SEARCH_LIST + 1];
void CSettings::SetSearchList(CHAR* szList)
{
	size_t n = _tcslen(szList);
	n = min((size_t)MAX_SEARCH_LIST, n);
	memcpy(searchList, szList, n);
	searchList[n] = 0;
	Write(STR_APP_REG_VAL_SEARCH_LIST, searchList);
}
CHAR* CSettings::GetSearchList()
{
	if (!Read(STR_APP_REG_VAL_SEARCH_LIST, searchList, MAX_SEARCH_LIST))
	{
		searchList[0] = 0;
	}
	return searchList;
}
DWORD CSettings::InfoTextColor()
{
	return RGB(128, 128, 128);
}
DWORD CSettings::InfoTextColorNative()
{
	return RGB(0, 0, 0);
}
DWORD CSettings::InfoTextColorAndroid()
{
	return RGB(180, 140, 10);
}
DWORD CSettings::SerachColor()
{
	return RGB(0xA0, 0xA9, 0x3d);
}
DWORD CSettings::CurSerachColor()
{
	return RGB(64, 128, 64);
}
DWORD CSettings::LogListTxtColor()
{
	return RGB(176, 176, 176);
}
DWORD CSettings::LogListBkColor()
{
	return RGB(0, 0, 0);
}
DWORD CSettings::LogListInfoBkColor()
{
	return RGB(240, 240, 240);
}
DWORD CSettings::SelectionTxtColor()
{
	return RGB(255, 255, 255);
}
DWORD CSettings::SelectionBkColor()
{
	return RGB(64, 122, 255);
}

void CSettings::SetVertSplitterPos(int i) { m_VertSplitterPos = i;  Write(STR_APP_REG_VAL_VERT_SPLITTER_POS, m_VertSplitterPos); }
void CSettings::SetHorzSplitterPos(int i) { m_HorzSplitterPos = i;  Write(STR_APP_REG_VAL_HORZ_SPLITTER_POS, m_HorzSplitterPos); }
void CSettings::SetStackSplitterPos(int i) { m_StackSplitterPos = i;  Write(STR_APP_REG_VAL_STACK_SPLITTER_POS, m_StackSplitterPos); }

void CSettings::SetColNN(int i) { m_ColNN = i;  Write(STR_APP_REG_VAL_ColNN, m_ColNN); }
void CSettings::SetColApp(int i) { m_ColApp = i; Write(STR_APP_REG_VAL_ColApp, m_ColApp); }
void CSettings::SetColPID(int i) { m_ColPID = i; Write(STR_APP_REG_VAL_ColPID, m_ColPID); }
void CSettings::SetColTID(int i) { m_ColTID = i; Write(STR_APP_REG_VAL_ColTID, m_ColTID); }
void CSettings::SetColThreadNN(int i) { m_ColThreadNN = i; Write(STR_APP_REG_VAL_ColThreadNN, m_ColThreadNN); }
void CSettings::SetColFunc(int i) { m_ColFunc = i; Write(STR_APP_REG_VAL_ColFunc, m_ColFunc); }
void CSettings::SetColLine(int i) { m_ColLine = i; Write(STR_APP_REG_VAL_ColLine, m_ColLine); }
void CSettings::SetColTime(int i) { m_ColTime = i; Write(STR_APP_REG_VAL_ColTime, m_ColTime); }
void CSettings::SetColCallAddr(int i) { m_ColCallAddr = i; Write(STR_APP_REG_VAL_ColCallAddr, m_ColCallAddr); }
void CSettings::SetFnCallLine(int i) { m_FnCallLine = i; Write(STR_APP_REG_VAL_FnCallLine, m_FnCallLine); }
void CSettings::SetUseAdb(int i) { m_UseAdb = i; Write(STR_APP_REG_VAL_UseAdb, m_UseAdb); }
void CSettings::SetRestartAdb(int i) { m_RestartAdb = i; Write(STR_APP_REG_VAL_RestartAdb, m_RestartAdb); }
void CSettings::SetApplyLogcutFilter(int i) { m_ApplyLogcutFilter = i; Write(STR_APP_REG_VAL_ApplyLogcutFilter, m_ApplyLogcutFilter); }
void CSettings::SetApplyPorcessFilter(int i) { m_ApplyPorcessFilter = i; Write(STR_APP_REG_VAL_ApplyPorcessFilter, m_ApplyPorcessFilter); }
void CSettings::SetShowChildCount(int i) { m_ShowChildCount = i; Write(STR_APP_REG_VAL_ShowChildCount, m_ShowChildCount); }

//void CSettings::SetBkColor(DWORD i){ m_BkColor = i; Write(STR_APP_REG_VAL_BK_COLOR, m_BkColor); }
//void CSettings::SetTextColor(DWORD i){ m_TextColor = i; Write(STR_APP_REG_VAL_TEXT_COLOR, m_TextColor); }
//void CSettings::SetInfoTextColor(DWORD i){ m_InfoTextColor = i; Write(STR_APP_REG_VAL_INFO_TEXT_COLOR, m_InfoTextColor); }
//void CSettings::SetSelColor(DWORD i){ m_SelColor = i; Write(STR_APP_REG_VAL_SEL_COLOR, m_SelColor); }
//void CSettings::SetSerachColor(DWORD i){ m_SerachColor = i; Write(STR_APP_REG_VAL_SEARCH_COLOR, m_SerachColor); }
//void CSettings::SetCurSerachColor(DWORD i){ m_CurSerachColor = i; Write(STR_APP_REG_VAL_CUR_SEARCH_COLOR, m_CurSerachColor); }

void CSettings::SetFlowTracesHiden(DWORD i) { m_FlowTracesHiden = i; Write(STR_APP_REG_VAL_FLOW_TRACE_HIGEN, m_FlowTracesHiden); }
void CSettings::SetTreeViewHiden(DWORD i) { m_TreeViewHiden = i; Write(STR_APP_REG_VAL_TREE_VIEW_HIDEN, m_TreeViewHiden); }
void CSettings::SetInfoHiden(DWORD i) { m_InfoHiden = i; Write(STR_APP_REG_VAL_INFO_HIDEN, m_InfoHiden); }
void CSettings::SetShowAppIp(DWORD i) { m_ShowAppIp = i; Write(STR_APP_REG_VAL_SHOW_APP_IP, m_ShowAppIp); }
void CSettings::SetShowElapsedTime(DWORD i) { m_ShowElapsedTime = i; Write(STR_APP_REG_VAL_SHOW_ELAPSED_TIME, m_ShowElapsedTime); }
void CSettings::SetResolveAddr(DWORD i) { m_ResolveAddr = i; Write(STR_APP_REG_VAL_RESOLVE_ADDR, m_ResolveAddr); }
void CSettings::SetUsePrefModule(DWORD i) { m_UsePrefModule = i; }
void CSettings::SetFullSrcPath(DWORD i) { m_FullSrcPath = i; Write(STR_APP_REG_VAL_FULL_SRC_PATH, m_FullSrcPath); }

void CSettings::SetEclipsePath(const CHAR* EclipsePath) { int c = _countof(m_EclipsePath); _tcsncpy_s(m_EclipsePath, c, EclipsePath, c - 1); m_EclipsePath[c] = 0; Write(STR_APP_REG_VAL_EclipsePath, m_EclipsePath); }
void CSettings::SetExternalCmd_1(const CHAR* ExternalCmd) { int c = _countof(m_ExternalCmd_1); _tcsncpy_s(m_ExternalCmd_1, c, ExternalCmd, c - 1); m_ExternalCmd_1[c] = 0; Write(STR_APP_REG_VAL_ExternalCmd_1, m_ExternalCmd_1); }
void CSettings::SetExternalCmd_2(const CHAR* ExternalCmd) { int c = _countof(m_ExternalCmd_2); _tcsncpy_s(m_ExternalCmd_2, c, ExternalCmd, c - 1); m_ExternalCmd_2[c] = 0; Write(STR_APP_REG_VAL_ExternalCmd_2, m_ExternalCmd_2); }
void CSettings::SetExternalCmd_3(const CHAR* ExternalCmd) { int c = _countof(m_ExternalCmd_3); _tcsncpy_s(m_ExternalCmd_3, c, ExternalCmd, c - 1); m_ExternalCmd_3[c] = 0; Write(STR_APP_REG_VAL_ExternalCmd_3, m_ExternalCmd_3); }
void CSettings::SetLinuxHome(const CHAR* LinuxHome) { int c = _countof(m_LinuxHome); _tcsncpy_s(m_LinuxHome, c, LinuxHome, c - 1); m_LinuxHome[c] = 0; Write(STR_APP_REG_VAL_LinuxHome, m_LinuxHome); }
void CSettings::SetMapOnWin(const CHAR* MapOnWin) { int c = _countof(m_MapOnWin); _tcsncpy_s(m_MapOnWin, c, MapOnWin, c - 1); m_MapOnWin[c] = 0; Write(STR_APP_REG_VAL_MapOnWin, m_MapOnWin); }
void CSettings::SetAndroidStudio(const CHAR* AndroidStudio) { int c = _countof(m_AndroidStudio); _tcsncpy_s(m_AndroidStudio, c, AndroidStudio, c - 1); m_AndroidStudio[c] = 0; Write(STR_APP_REG_VAL_AndroidStudio, m_AndroidStudio); }
void CSettings::SetAndroidProject(const CHAR* AndroidProject) { int c = _countof(m_AndroidProject); _tcsncpy_s(m_AndroidProject, c, AndroidProject, c - 1); m_AndroidProject[c] = 0; Write(STR_APP_REG_VAL_AndroidProject, m_AndroidProject); }

void CSettings::SetUdpPort(DWORD i) { m_UdpPort = i; Write(STR_APP_REG_VAL_UDP_PORT, m_UdpPort); }
void CSettings::SetAdbArg(const CHAR* str) { int c = _countof(m_AdbArg); _tcsncpy_s(m_AdbArg, c, str, c - 1); m_AdbArg[c] = 0; Write(STR_APP_REG_VAL_ADB_ARG, m_AdbArg); }












