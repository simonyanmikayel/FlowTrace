#include "stdafx.h"
#include "Helpers.h"
#include "Resource.h"
#include "MainFrm.h"
#include "Settings.h"
#include "DlgInfo.h"

namespace Helpers
{

	void OnLButtonDoun(LOG_NODE* pNode, WPARAM wParam, LPARAM lParam)
	{
		bool bAltPressed = (GetKeyState(VK_MENU) & 0x8000) != 0;
		bool bCtrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool bMbuttonPressed = (GetKeyState(VK_MBUTTON) & 0x8000) != 0;
        if (bCtrlPressed || bMbuttonPressed)
        {
            //if (pNode && !pNode->isSynchronized(gSyncronizedNode))
            //    gMainFrame->SyncViews();
            ShowInIDE(pNode, true);
        }
        else if (bAltPressed)
        {
            ShowInIDE(pNode, false);
        }
    }

	bool ShowInIDE(char* src, int line, bool IsAndroidLog)
	{
		if (!src || !src[0] || line <= 0)
			return false;

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		const int max_cmd = 2 * MAX_PATH;
		char cmd[max_cmd + 1];
		char *process;

		if (IsAndroidLog)
		{
            char src2[MAX_PATH + 1];
            char fileName2[MAX_PATH + 1];
            _sntprintf_s(src2, MAX_PATH, MAX_PATH, "src\\main\\java\\%s", src);
            src2[MAX_PATH] = 0;
            char* fileName = strrchr(src2, '\\');
            if (!fileName)
                return false;
            *fileName = 0;
            fileName++;
            char* srcPath = FindFile(gSettings.GetAndroidProject(), src2, false);
            if (!srcPath)
                return false;
            _sntprintf_s(fileName2, MAX_PATH, MAX_PATH, "%s.java", fileName);
            fileName2[MAX_PATH] = 0;
            _sntprintf_s(src2, MAX_PATH, MAX_PATH, "%s", srcPath);
            srcPath = FindFile(src2, fileName2, true);
            src2[MAX_PATH] = 0;
            if (!srcPath)
                return false;
		    _sntprintf_s(cmd, max_cmd, max_cmd, "\"%s\" --line %d \"%s\"", gSettings.GetAndroidStudio(), line, srcPath);
			process = NULL;
		}
		else
		{
			char* szLinuxHome = gSettings.GetLinuxHome();
			if (strstr(src, szLinuxHome))
				src = strstr(src, szLinuxHome) + strlen(szLinuxHome);
			_sntprintf_s(cmd, max_cmd, max_cmd, " -name Eclipse --launcher.openFile %s\\%s:%d", gSettings.GetMapOnWin(), src, line);
			//D:\Programs\eclipse\eclipse-cpp-neon-M4a-win32-x86_64\eclipsec.exe -name Eclipse --launcher.openFile X:\prj\c\c\ctap\kernel\CTAPparameters\src\CTAP_parameters.c:50
			process = gSettings.GetEclipsePath();
		}
		CreateProcess(process, cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
        return true;
	}

	void ShowInIDE(LOG_NODE* pSelectedNode, bool bShowCallSite)
	{
		if (!pSelectedNode || !pSelectedNode->CanShowInIDE() || !pSelectedNode->isInfo())
			return;
        bool IsAndroidLog = pSelectedNode->isJava();
		char* src = 0;
		int line = 0;
		char src2[MAX_PATH + 1];
        char src3[MAX_PATH + 1];
        if (IsAndroidLog)
		{
			INFO_NODE* pNode = (INFO_NODE*)pSelectedNode;
			int cb = 0;
			if ((!bShowCallSite || pNode->isTrace()))
			{
                if (pNode->isTrace()) // && (LOG_FLAG_EXCEPTION & pNode->log_flags)
                {
                    TRACE_CHANK* pTraceChank = ((TRACE_NODE*)pNode)->getFirestChank();
                    cb = min(pTraceChank->len, MAX_PATH);
                    strncpy_s(src3, pTraceChank->trace, cb);
                    src3[cb] = 0;
                    
                    char* at = strstr(src3, "at ");
                    if (at == 0)
                        at = strstr(src3, "<- ");
                    if (at)
                    {
                        at += 3;
                        char* ch = strrchr(at, '(');
                        if (ch)
                        {
                            *ch = 0;
                            char* java = ch + 1;
                            char* colon = strrchr(ch + 1, ':');
                            if (colon && strrchr(colon, ')'))
                            {
                                *colon = 0;
                                ch = strstr(java, ".java");
                                if (ch)
                                {
                                    *ch = 0;
                                    ch = strstr(at, java);
                                    if (ch)
                                    {
                                        line = atoi(colon + 1);
                                        if (line)
                                            src = at;
                                    }
                                }
                            }
                        }
                    }
                }

                if(line == 0 || src ==0) 
                {
                    src = pNode->fnName();
                    cb = pNode->cb_fn_name;
                    if (pSelectedNode->isTrace()) {
                        line = pNode->call_line;
                    }
                    else if (pSelectedNode->isFlow()) {
                        FLOW_NODE* flowNode = (FLOW_NODE*)pNode;
                        line = flowNode->fn_line;
                    }
                }
			}
            else if(pNode->parent && pNode->parent->isFlow())
            {
                src = pNode->JavaCallSite();
                cb = pNode->cb_java_call_site;
                line = ((INFO_NODE*)pSelectedNode)->callLine(true);
            }
            if (src && src[0] && cb && line)
            {
                cb = min(cb, MAX_PATH);
                strncpy_s(src2, src, cb);
                src2[cb] = 0;
                for (int i = 0; i < cb; i++)
                {
                    if (src2[i] == '/')
                        src2[i] = '.';
                }

                char* dot = strrchr(src2, '.');
                if (dot)
                    *dot = 0;
                dot = src2;
                while (dot = strchr(dot, '.'))
                    *dot = '\\';
                dot = strchr(src2, '$');
                if (dot)
                    *dot = 0;
                src = src2;
            }
		}
		else
		{
            FLOW_NODE* flowNode = pSelectedNode->getSyncNode();
            if (flowNode)
            {
                ADDR_INFO* p_addr_info = NULL;
                p_addr_info = pSelectedNode->isTrace() ? flowNode->getFuncInfo(true) : (bShowCallSite ? flowNode->getCallInfo(true) : flowNode->getFuncInfo(true));
                if (!p_addr_info)
                {
                    p_addr_info = pSelectedNode->isTrace() ? flowNode->getFuncInfo(true) : (bShowCallSite ? flowNode->getCallInfo(true) : flowNode->getFuncInfo(true));
                }                
                if (p_addr_info && p_addr_info->line > 0)
                {
                    src = p_addr_info->src;
                    line = 0;
                    if (pSelectedNode->isTrace())
                    {
                        line = ((TRACE_NODE*)pSelectedNode)->call_line;
                    }
                    else
                    {
                        line = p_addr_info->line;
                    }
                }
            }
		}
		ShowInIDE(src, line, IsAndroidLog);
	}

	void ShowNodeDetails(LOG_NODE* pNode)
	{
		if (pNode == NULL || !pNode->isInfo())
			return;
		const int cMax = 2048;
		char buf[cMax + 1];
		int cb = 0;
		INFO_NODE* pInfoNode = (INFO_NODE*)pNode;
        FLOW_NODE* pFlowNode = pNode->isFlow() ? (FLOW_NODE*)pNode : 0;

        if (cb < cMax - pInfoNode->cb_fn_name) {
            memcpy(buf + cb, pInfoNode->fnName(), pInfoNode->cb_fn_name);
            cb += pInfoNode->cb_fn_name;
            if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "\r\n");
        }
        if (pInfoNode->isJava() && cb < cMax - pInfoNode->cb_java_call_site) {
            memcpy(buf + cb, pInfoNode->JavaCallSite(), pInfoNode->cb_java_call_site);
            cb += pInfoNode->cb_java_call_site;
            if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "\r\n");
        }
        if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "nn: %d\r\n", pInfoNode->nn);
		if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "log_type: %d\r\n", pInfoNode->log_type);
		if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "log_flags: %d\r\n", pInfoNode->log_flags);
        if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "data_type: %d\r\n", pInfoNode->data_type);
        if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "call_line: %d\r\n", pInfoNode->call_line);
		if (pFlowNode) {
			if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "this_fn: %X\r\n", pFlowNode->this_fn);
			if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "call_site: %X\r\n", pFlowNode->call_site);
            if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "fn_line: %d\r\n", pFlowNode->fn_line);
        }
		if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "----------------------\r\n");
		if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "getApp: %p threadNode: %p\r\n", pNode->getApp() , pNode->threadNode);
		if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "----------------------\r\n");
		LOG_NODE* pNode0 = pNode;
		while (pNode0) {
			if (cb < cMax) cb += snprintf(buf + cb, cMax - cb, "[ %p ]", pNode0);
			pNode0 = pNode0->parent;
		}
		buf[cMax] = 0;
		DlgInfo dlg(buf);
		dlg.DoModal();
	}

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

	void UpdateStatusBar()
	{
		gMainFrame->UpdateStatusBar();
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

	DWORD  GetSec(int hour, int min, int sec)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);

		tm local;
		memset(&local, 0, sizeof(local));
		local.tm_year = st.wYear - 1900;
		local.tm_mon = st.wMonth - 1;
		local.tm_mday = st.wDay;
		local.tm_wday = st.wDayOfWeek;
		local.tm_hour = hour;// st.wHour;
		local.tm_min = min;  // st.wMinute;
		local.tm_sec = sec;  // st.wSecond;

		return (DWORD)(mktime(&local));
	}

    void AddMenu(HMENU hMenu, int& cMenu, int ID_MENU, LPCTCH str, bool disable, MENU_ICON ID_ICON)
    {
        DWORD dwFlags;
        dwFlags = MF_BYPOSITION | MF_STRING;
        if (disable)
            dwFlags |= MF_DISABLED;
        InsertMenu(hMenu, cMenu++, dwFlags, ID_MENU, str);
        if (ID_ICON >= 0)
            Helpers::SetMenuIcon(hMenu, cMenu - 1, ID_ICON);
    }
     
    void AddCommonMenu(LOG_NODE* pNode, HMENU hMenu, int& cMenu)
	{
        bool disable;
        disable = (pNode == NULL || !pNode->isInfo());
        AddMenu(hMenu, cMenu, ID_SYNC_VIEWES, _T("Synchronize views\tTab"), disable, MENU_ICON_SYNC);

        disable = (pNode == NULL || !pNode->CanShowInIDE() || !pNode->isInfo());
        AddMenu(hMenu, cMenu, ID_SHOW_CALL_IN_FUNCTION, _T("Show Call Line\tCtrl+Click"), disable);

        disable = (pNode == NULL || !pNode->CanShowInIDE() || !pNode->isInfo() || pNode->isTrace());
        AddMenu(hMenu, cMenu, ID_SHOW_FUNCTION, _T("Show Function\tAlt+Click"), disable, MENU_ICON_FUNC_IN_ECLIPSE);

        disable = (pNode == NULL || !pNode->isInfo());
        AddMenu(hMenu, cMenu, ID_VIEW_NODE_DATA, _T("Details..."), disable);

		InsertMenu(hMenu, cMenu++, MF_BYPOSITION | MF_SEPARATOR, 0, _T(""));
	}
	void SetMenuIcon(HMENU hMenu, UINT item, MENU_ICON icon)
	{
		static HBITMAP hbmpItem[MENU_ICON_MAX] = { 0 };

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

    static char szFindFilePath[2*MAX_PATH];
    bool fileFound = false;
    void _FindFile(char* szDirName, char* szFileName, bool isFile, size_t cbFileName)
    {
        HANDLE hFind;
        WIN32_FIND_DATA wfd;
        char path[MAX_PATH];
        sprintf_s(path, "%s\\*", szDirName);
        if (cbFileName == 0)
            cbFileName = strlen(szFileName);

        //fprintf(stdout, "In Directory \"%s\"\n\n", szDirName);
        if ((hFind = FindFirstFile(path, &wfd)) == INVALID_HANDLE_VALUE)
        {
            //fprintf(stderr, "FindFirstFIle failed on path = \"%s\"\n", path);
            return;
        }

        BOOL cont = TRUE;
        while (cont == TRUE && !fileFound)
        {
            //if ((strncmp(".", wfd.cFileName, 1) != 0) && (strncmp("..", wfd.cFileName, 2) != 0))
            if (wfd.cFileName[0] != '.' && 0 != strcmp(wfd.cFileName, "build"))
            {
                if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    sprintf_s(path, "%s\\%s", szDirName, wfd.cFileName);
                    if (!isFile) 
                    {
                        size_t cbFindFilePath = sprintf_s(szFindFilePath, "%s\\%s", szDirName, wfd.cFileName);
                        if (cbFindFilePath > cbFileName)
                        {
                            if (0 == _stricmp(szFindFilePath + (cbFindFilePath - cbFileName), szFileName))
                            {
                                fileFound = true;
                                break;
                            }
                        }
                    }
                    if (fileFound)
                        break;
                    else
                        _FindFile(path, szFileName, isFile, cbFileName);
                }
                else if (isFile)
                {  
                    size_t cbFindFilePath = sprintf_s(szFindFilePath, "%s\\%s", szDirName, wfd.cFileName);
                    if (cbFindFilePath > cbFileName)
                    {
                        //if (strstr(szFindFilePath, szFileName))
                        //stdlog("%s  %s\n", szFindFilePath, szFileName);
                        if (0 == _stricmp(szFindFilePath + (cbFindFilePath - cbFileName), szFileName))
                        {
                            fileFound = true;
                            break;
                        }
                    }
                }
            }
            cont = FindNextFile(hFind, &wfd);
        }
        FindClose(hFind);
    }
    char* FindFile(char* szDirName, char* szFileName, bool isFile)
    {
        fileFound = false;
        szFindFilePath[0] = 0;
        _FindFile(szDirName, szFileName, isFile, 0);
        if (fileFound)
            return szFindFilePath;
        else
            return 0;
    }
};

