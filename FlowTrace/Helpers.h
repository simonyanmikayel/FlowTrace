#pragma once

struct LOG_NODE;
struct FLOW_NODE;
extern HWND hwndMain;
extern FLOW_NODE*  gSyncronizedNode;

#define WM_UPDATE_FILTER               WM_USER + 1000
#define WM_INPORT_TASK                 WM_USER + 1001
#define WM_MOVE_SELECTION_TO_END       WM_USER + 1002
#define WM_SHOW_NGS                    WM_USER + 1003
#define WM_UPDATE_BACK_TRACE           WM_USER + 1004
#define WM_UPDATE_NC                   WM_USER + 1005
#define WM_UPDATE_TREE                 WM_USER + 1006
#define WM_NAVIGATE_TO_SEARCH          WM_USER + 1007
static const int ICON_LEN = 16;
static const int TEXT_MARGIN = 8;
static const int INFO_TEXT_MARGIN = 4;
static const int ICON_OFFSET = 16 + 4;
enum MENU_ICON { MENU_ICON_NON = -1, MENU_ICON_SYNC, MENU_ICON_FUNC_IN_ECLIPSE, MENU_ICON_CALL_IN_ECLIPSE, MENU_ICON_MAX };
namespace Helpers
{
	void CopyToClipboard(HWND hWnd, char* szText, int cbText);
	void SysErrMessageBox(CHAR* lpFormat, ...);
	void ErrMessageBox(CHAR* lpFormat, ...);
	CHAR* find_str(const CHAR *phaystack, const CHAR *pneedle, int bMatchCase);
	CHAR* str_format_int_grouped(__int64 num);
	void GetTime(DWORD &sec, DWORD& msec);
	DWORD  GetSec(int hour, int min, int sec);
	void SetMenuIcon(HMENU hMenu, UINT item, MENU_ICON icon);
	void AddCommonMenu(LOG_NODE* pNode, HMENU hMenu, int& cMenu);
    void AddMenu(HMENU hMenu, int& cMenu, int ID_MENU, LPCTCH str, bool disable = false, MENU_ICON ID_ICON = MENU_ICON_NON);
    void OnLButtonDoun(LOG_NODE* pNode, WPARAM wParam, LPARAM lParam);
	void UpdateStatusBar();
	void RedrawViews();
	void ShowInIDE(LOG_NODE* pNode, bool bShowCallSite);
	bool ShowInIDE(char* src, int line, bool IsAndroidLog);
	void ShowNodeDetails(LOG_NODE* pNode);
	const char* FindFile(const char* szDirName, const char* szFileName, bool isFile);
	size_t strCpy(char*& strDest, const char* strSource, size_t& count);

	inline void NextLine(char* buf, int buf_size, int& i)
	{
		while (buf[i] != 0 && buf[i] == '\r' || buf[i] == '\n' && i < buf_size)
			i++;
	}

	inline bool ToChar(char ch, char* buf, int buf_size, int& i)
	{
		while (buf[i] != 0 && buf[i] != ch && i < buf_size)
			i++;
		return buf[i] == ch;
	}

	inline void LineEnd(char* buf, int buf_size, int& i)
	{
		while (buf[i] != 0 && buf[i] != '\r' && buf[i] != '\n' && i < buf_size)
			i++;
	}

	inline void SkipWhite(char* buf, int buf_size, int& i)		
	{
		while (buf[i] != 0 && buf[i] <= ' ' && i < buf_size)
			i++;
	}

	inline int NextDigit(char* buf, int buf_size, int& i, bool& ok)
	{
		int ret = 0;
		if (ok) {
			while (buf[i] != 0 && !isdigit(buf[i]) && i < buf_size)
				i++;
			int i0 = i;
			ret = ok ? atoi(buf + i) : 0;
			while (isdigit(buf[i]) && i < buf_size)
				i++;
			ok = (i != i0);
		}
		return ret;
	}

};