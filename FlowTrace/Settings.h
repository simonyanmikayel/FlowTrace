#pragma once

//#define USE_FONT_RES

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
    void SetUIFont(TCHAR* lfFaceName, LONG lfWeight, LONG lfHeight);
    bool CheckUIFont(HDC hdc);

    void SetSearchList(TCHAR* szList);
    TCHAR* GetSearchList();
    DWORD SelectionBkColor(bool haveFocus);
    DWORD SelectionTxtColor();
    DWORD LogListBkColor();
    DWORD LogListTxtColor();
    DWORD SerachColor();
    DWORD CurSerachColor();
    DWORD InfoTextColor();

    DECL_PROP(int, VertSplitterPos);
    DECL_PROP(int, HorzSplitterPos);
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
    DECL_PROP(DWORD, UsePcTime);
    DECL_PROP(DWORD, CompactView);
    DECL_PROP(DWORD, ShowAppIp);
    DECL_PROP(DWORD, ShowAppTime);
    DECL_PROP(DWORD, ShowElapsedTime);
    DECL_PROP(DWORD, ResolveAddr);
    DECL_PROP(DWORD, FullSrcPath);

    DECL_PROP(int, ColLineNN);
    DECL_PROP(int, ColNN);
    DECL_PROP(int, ColApp);
    DECL_PROP(int, ColPID);
    DECL_PROP(int, ColFunc);
    DECL_PROP(int, ColLine);
    DECL_PROP(int, ColTime);
    DECL_PROP(int, ColCallAddr);

    DECL_PROP(DWORD, UdpPort);

    DECL_GET(DWORD, FontSize);
    DECL_GET(TCHAR*, FontName);
    DECL_GET(DWORD, FontWeight);
    DECL_GET(TCHAR*, ResFontName);

    DECL_PROP(DWORD, UpdateStack);
    DECL_STR_PROP(TCHAR, EclipsePath, MAX_PATH);
    DECL_STR_PROP(TCHAR, LinuxHome, MAX_PATH);
    DECL_STR_PROP(TCHAR, MapOnWin, MAX_PATH);

private:
    LOGFONT   m_logFont;
    HANDLE m_resourceFonthandle;

    void AddDefaultFont();
    void InitFont();
    void DeleteFont();
};

extern CSettings gSettings;