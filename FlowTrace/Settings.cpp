#include "StdAfx.h"
#include "Settings.h"
#include "resource.h"
#include "LogData.h"

CSettings gSettings;

// registry keys and values
LPCTSTR STR_APP_REG_KEY = _T("Software\\TermianlTools\\FlowTrace");
LPCTSTR STR_APP_REG_VAL_WINDOWPOS = _T("WindowPos");
LPCTSTR STR_APP_REG_VAL_FONTNAME = _T("FontName_1");
LPCTSTR STR_APP_REG_VAL_FONTWIDTH = _T("FontWidth");
LPCTSTR STR_APP_REG_VAL_FONTSIZE = _T("FontSize");
LPCTSTR STR_APP_REG_VAL_SEARCH_LIST = _T("SearchList");
LPCTSTR STR_APP_REG_VAL_Modules = _T("Modules");

#define DefUdpPort  8888
#define DefFontSize 11

static CHAR* DEF_FONT_NAME = _T("Consolas"); //Courier New //Consolas //Inconsolata

CSettings::CSettings() :
	CRegKeyExt(STR_APP_REG_KEY)
	, comPort_1(this, _T("ComPort_Name_1"),_T("ComPort_Speed_1"), _T("ComPort_DataBits_1"),_T("ComPort_StopBits_1"),_T("ComPort_Parity_1"), _T("ComPort_FlowControl_1"))
	, comPort_2(this, _T("ComPort_Name_2"), _T("ComPort_Speed_2"), _T("ComPort_DataBits_2"), _T("ComPort_StopBits_2"), _T("ComPort_Parity_2"), _T("ComPort_FlowControl_2"))
	, m_UseComPort_1(this, _T("UseComPort_1"), FALSE)
	, m_UseComPort_2(this, _T("UseComPort_2"), FALSE)
	, m_processFilterList(this, _T("Filters"), 3*1024, 128, "\n")
	, m_adbFilterList(this, _T("LogcutFilter1"), 512, 48, " ")
	, m_VertSplitterPos(this, _T("VertSplitterPos"), 50)
	, m_HorzSplitterPos(this, _T("HorzSplitterPos"), 50)
	, m_StackSplitterPos(this, _T("StackSplitterPos"), 5)
	, m_FlowTracesHiden(this, _T("FlowTraceHiden"), TRUE)	
	, m_TreeViewHiden(this, _T("TreeViewHiden"), FALSE)
	, m_InfoHiden(this, _T("InfoHiden"), FALSE)
	, m_ShowAppIp(this, _T("ShowAppIp"), FALSE)
	, m_ShowElapsedTime(this, _T("ShowElapsedTime"), FALSE)
	, m_ResolveAddr(this, _T("ResolveAddr"), TRUE)
	, m_FullSrcPath(this, _T("FullSrcPath"), FALSE)
	, m_ColNN(this, _T("ColNN"), 0)
	, m_ColApp(this, _T("ColApp"), 1)
	, m_ColPID(this, _T("ColPID"), 0)
	//, m_ColTID(this, _T("ColTID"), 0)
	//, m_ColThreadNN(this, _T("ColThreadNN"), 0)
	, m_ShowChildCount(this, _T("ShowChildCount"), 0)
	, m_ColFunc(this, _T("ColFunc"), 1)
	, m_ColLine(this, _T("ColLine"), 1)
	, m_ColTime(this, _T("ColTime"), 0)
	, m_ColCallAddr(this, _T("ColCallAddr"), 0)
	, m_FnCallLine(this, _T("FnCallLine"), 0)
	, m_UseAdb(this, _T("UseAdb"), 1)
	, m_RestartAdb(this, _T("RestartAdb"), 1)
	, m_ApplyLogcutFilter(this, _T("ApplyLogcutFilter"), 1)
	, m_ApplyPorcessFilter(this, _T("ApplyPorcessFilter"), 1)
	, m_UdpPort(this, _T("UdpPort"), DefUdpPort)
	, m_RawTcpPort(this, _T("RawTcpPort"), 0)
	, m_AdbArg(this, _T("AdbArg"), MAX_PATH)
	, m_LogcatArg(this, _T("LogcatArg"), MAX_PATH)
	, m_EclipsePath(this, _T("EclipsePath"), MAX_PATH)
	, m_ExternalCmd_1(this, _T("ExternalCmd_1"), MAX_PATH)
	, m_ExternalCmd_2(this, _T("ExternalCmd_2"), MAX_PATH)
	, m_ExternalCmd_3(this, _T("ExternalCmd_3"), MAX_PATH)
	, m_LinuxHome(this, _T("LinuxHome"), MAX_PATH)
	, m_MapOnWin(this, _T("MapOnWin"), MAX_PATH)
	, m_AndroidStudio(this, _T("AndroidStudio"), MAX_PATH)
	, m_AndroidProject(this, _T("AndroidProject"), MAX_PATH)
	//, m_(this, _T(""), )
{
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
	Read(STR_APP_REG_VAL_FONTSIZE, m_fontSize.val, DefFontSize);

	m_FontName = m_logFont.lfFaceName;
	m_FontWeight = m_logFont.lfWeight;

	if (m_fontSize.val <= 4) m_fontSize.val = DefFontSize;

	m_logFont.lfWeight = (m_logFont.lfWeight <= FW_MEDIUM) ? FW_NORMAL : FW_BOLD;

	HDC hdc = CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);
	m_logFont.lfHeight = -MulDiv(m_fontSize.val, GetDeviceCaps(hdc, LOGPIXELSY), 72);
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















