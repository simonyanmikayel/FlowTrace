#pragma once

#include "RegKeyExt.h"

#define DECL_GET(type, name) public: type Get##name () { return m_##name ;} private: type m_##name
#define DECL_SET(type, name) public: void Set##name ( type ); private: type m_##name
#define DECL_PROP(type, name) public: type Get##name () { return m_##name ;} void Set##name ( type ); private: type m_##name
#define DECL_STR_PROP(type, name, cb) public: type* Get##name () { return m_##name ;} void Set##name ( const type* ); private: type m_##name[cb];

class CSettings : public CRegKeyExt
{
public:
    CSettings();
    ~CSettings();

    void RestoreWindPos(HWND hWnd);
    void SaveWindPos(HWND hWnd);
    void SetConsoleColor(int consoleColor, DWORD& textColor, DWORD& bkColor);
    void SetUIFont(CHAR* lfFaceName, LONG lfWeight, LONG lfHeight);
    bool CheckUIFont(HDC hdc);

    void SetModules(const CHAR* szList);
    CHAR* GetModules();
    void SetSearchList(CHAR* szList);
    CHAR* GetSearchList();
    DWORD SelectionBkColor(bool haveFocus);
    DWORD SelectionTxtColor();
    DWORD LogListBkColor();
    DWORD LogListTxtColor();
    DWORD SerachColor();
    DWORD CurSerachColor();
    DWORD InfoTextColor();
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

    DECL_PROP(int, ColLineNN);
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

    DECL_PROP(DWORD, UdpPort);

    DECL_GET(DWORD, FontSize);
    DECL_GET(CHAR*, FontName);
    DECL_GET(DWORD, FontWeight);
    DECL_GET(CHAR*, ResFontName);

    DECL_STR_PROP(CHAR, EclipsePath, MAX_PATH);
	DECL_STR_PROP(CHAR, ExternalCmd, MAX_PATH);
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