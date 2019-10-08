#pragma once

#include "RegKeyExt.h"

enum _flow_LogPriority;

#define DECL_GET(type, name) public: type Get##name () { return m_##name ;} private: type m_##name
#define DECL_SET(type, name) public: void Set##name ( type ); private: type m_##name
#define DECL_PROP(type, name) public: type Get##name () { return m_##name ;} void Set##name ( type ); private: type m_##name
#define DECL_STR_PROP(type, name, cb) public: type* Get##name () { return m_##name ;} void Set##name ( const type* ); private: type m_##name[cb];

struct StringList
{
	StringList(CRegKeyExt* pReg, LPCTSTR key, int max_buf, int max_it) {
		pRegKey = pReg;
		max_buffer = max_buf;
		max_item = max_it;
		buffer = new CHAR[max_buffer + 1];
		items = new CHAR*[max_item];
		regKey = key;

		if (!pRegKey->Read(regKey, buffer, max_buffer))
		{
			buffer[0] = 0;
		}
		formatBuffer();
	}
	~StringList() {
		delete [] buffer;
		delete[] items;
	}
	LPCTSTR getKey() {
		return regKey;
	};
	void setList(const CHAR* szList) {
		size_t n = _tcslen(szList);
		n = min((size_t)max_buffer, n);
		memcpy(buffer, szList, n);
		buffer[n] = 0;
		pRegKey->Write(regKey, buffer);
		formatBuffer();
	}
	int getItemCount() {
		return itemCount;
	}
	CHAR* getItem(int item) {
		return items[item];
	}
private:
	CHAR* buffer;
	LPCTSTR regKey;
	int max_buffer;
	int max_item;
	int itemCount;
	CHAR** items;
	CRegKeyExt* pRegKey;

	void formatBuffer() {
		itemCount = 0;
		char *next_token = NULL;
		char *p = strtok_s(buffer, "\n", &next_token);
		while (p && itemCount < max_item) {
			items[itemCount] = p;
			itemCount++;
			p = strtok_s(NULL, "\n", &next_token);
		}
	}
};

class CSettings : public CRegKeyExt
{
public:
    CSettings();
    ~CSettings();

    void RestoreWindPos(HWND hWnd);
    void SaveWindPos(HWND hWnd);
	bool SetTraceColor(int color, DWORD& textColor, DWORD& bkColor);
	bool SetTracePriority(_flow_LogPriority priority, DWORD& textColor, DWORD& bkColor);
	void SetUIFont(CHAR* lfFaceName, LONG lfWeight, LONG lfHeight);

	StringList m_filterList;
	StringList& getFilterList() {
		return m_filterList;
	}

    void SetModules(const CHAR* szList);
	CHAR* GetModules();
	void SetSearchList(CHAR* szList);
    CHAR* GetSearchList();
    DWORD SelectionBkColor();
    DWORD SelectionTxtColor();
    DWORD LogListBkColor();
	DWORD LogListInfoBkColor();
    DWORD LogListTxtColor();
    DWORD SerachColor();
    DWORD CurSerachColor();
	DWORD InfoTextColor();
	DWORD InfoTextColorNative();
	DWORD InfoTextColorAndroid();
	bool CanShowInEclipse() { return *GetEclipsePath() != 0 && GetResolveAddr(); }
	bool CanShowInAndroidStudio() { return *GetAndroidStudio() != 0 && *GetAndroidProject() != 0; }

    DECL_PROP(int, VertSplitterPos);
    DECL_PROP(int, HorzSplitterPos);
    DECL_PROP(int, StackSplitterPos);
    DECL_PROP(HFONT, Font);
    DECL_PROP(int, FontHeight);
    DECL_PROP(int, FontWidth);

	//DECL_PROP(DWORD, TextColor);
    //DECL_PROP(DWORD, InfoTextColor);
    //DECL_PROP(DWORD, BkColor);
    //DECL_PROP(DWORD, SelColor);
    //DECL_PROP(DWORD, BkSelColor);
    //DECL_PROP(DWORD, SerachColor);
    //DECL_PROP(DWORD, CurSerachColor);
    //DECL_PROP(DWORD, SyncColor);

    DECL_PROP(DWORD, FlowTracesHiden);
    DECL_PROP(DWORD, TreeViewHiden);
    DECL_PROP(DWORD, InfoHiden);
    DECL_PROP(DWORD, ShowAppIp);
    DECL_PROP(DWORD, ShowElapsedTime);
    DECL_PROP(DWORD, ResolveAddr);
    DECL_PROP(DWORD, UsePrefModule);
    DECL_PROP(DWORD, FullSrcPath);

    DECL_PROP(int, ColNN);
    DECL_PROP(int, ColApp);
    DECL_PROP(int, ColPID);
    DECL_PROP(int, ColTID);
    DECL_PROP(int, ColThreadNN);
    DECL_PROP(int, ShowChildCount);
	DECL_PROP(int, ColFunc);
    DECL_PROP(int, ColLine);
    DECL_PROP(int, ColTime);
    DECL_PROP(int, ColCallAddr);
    DECL_PROP(int, FnCallLine);
	DECL_PROP(int, UseAdb);
	DECL_PROP(int, ApplyFilter);


    DECL_PROP(DWORD, UdpPort);

    DECL_GET(DWORD, FontSize);
    DECL_GET(CHAR*, FontName);
    DECL_GET(DWORD, FontWeight);
    DECL_GET(CHAR*, ResFontName);

    DECL_STR_PROP(CHAR, EclipsePath, MAX_PATH);
	DECL_STR_PROP(CHAR, ExternalCmd_1, MAX_PATH);
	DECL_STR_PROP(CHAR, ExternalCmd_2, MAX_PATH);
	DECL_STR_PROP(CHAR, ExternalCmd_3, MAX_PATH);
	DECL_STR_PROP(CHAR, LinuxHome, MAX_PATH);
    DECL_STR_PROP(CHAR, MapOnWin, MAX_PATH);
	DECL_STR_PROP(CHAR, AndroidStudio, MAX_PATH);
	DECL_STR_PROP(CHAR, AndroidProject, MAX_PATH);
    DECL_STR_PROP(CHAR, PrefModulePath, MAX_PATH);


private:
    LOGFONT   m_logFont;
    HANDLE m_resourceFonthandle;

    void AddDefaultFont();
	void InitFont();
	void DeleteFont();
};

extern CSettings gSettings;