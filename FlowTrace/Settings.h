#pragma once

#include "RegKeyExt.h"

enum _flow_LogPriority;

struct StringList
{
	StringList(CRegKeyExt* pReg, LPCTSTR key, int max_buf, int max_it, LPCSTR delim) {
		pRegKey = pReg;
		max_buffer = max_buf;
		max_item = max_it;
		buffer = new CHAR[max_buffer + 1];
		items = new CHAR*[max_item];
		regKey = key;
		delimeter = delim;
		origList = 0;

		if (!pRegKey->Read(regKey, buffer, max_buffer))
		{
			buffer[0] = 0;
		}
		formatBuffer();
	}
	~StringList() {
		delete [] buffer;
		delete[] items;
		free(origList);
	}
	LPCTSTR getKey() {
		return regKey;
	};
	void setList(const CHAR* szList) {
		size_t n = strlen(szList);
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
	CHAR* toString() {
		return origList ? origList : nullptr;
	}
private:
	CHAR* buffer;
	LPCTSTR regKey;
	LPCSTR delimeter;
	int max_buffer;
	int max_item;
	int itemCount;
	CHAR** items;
	CRegKeyExt* pRegKey;
	CHAR* origList;

	void formatBuffer() {
		free(origList);
		origList = _strdup(buffer);
		itemCount = 0;
		char *next_token = NULL;
		char *p = strtok_s(buffer, delimeter, &next_token);
		while (p && itemCount < max_item) {
			items[itemCount] = p;
			itemCount++;
			p = strtok_s(NULL, delimeter, &next_token);
		}
	}
};

struct RegExString
{
	RegExString(CRegKeyExt* pReg, LPCTSTR key, int max_buf, LPTSTR defVal = nullptr) {
		pRegKey = pReg;
		max_buffer = max_buf;
		buffer = new CHAR[max_buffer + 1];
		regKey = key;

		if (!pRegKey->Read(regKey, buffer, max_buffer, defVal))
		{
			buffer[0] = 0;
		}
	}
	~RegExString() {
		delete[] buffer;
	}
	void set(const CHAR* sz, size_t n) {
		n = min((size_t)max_buffer, n);
		memcpy(buffer, sz, n);
		buffer[n] = 0;
		pRegKey->Write(regKey, buffer);
	}
	void set(const CHAR* sz) {
		size_t n = strlen(sz);
		set(sz, n);
	}
	const CHAR* get() {
		return buffer;
	}
private:
	CHAR* buffer;
	LPCTSTR regKey;
	int max_buffer;
	CRegKeyExt* pRegKey;
};


template <typename T> struct ReadOnly
{
	friend class CSettings;
	ReadOnly(T def = 0) {
		val = def;
	}
	const T get() {
		return val;
	}
private:
	ReadOnly<T>& operator = (T v) {
		val = v;
		return *this;
	}
	operator T() {
		return val; 
	}
	T val;
};

template <typename T> struct RegExNum
{
	RegExNum(CRegKeyExt* pReg, LPCTSTR key, T def) {
		pRegKey = pReg;
		regKey = key;
		pRegKey->Read(regKey, val, (DWORD)def);
	}
	void set(T i) {
		val = (DWORD)i;
		pRegKey->Write(regKey, i);
	}
	const T get() {
		return (T)val;
	}

private:
	DWORD val;
	LPCTSTR regKey = 0;
	CRegKeyExt* pRegKey = 0;
};

#define PROP_STR(name) RegExString m_##name; const CHAR* Get##name() {return m_##name.get();} void Set##name(const CHAR* val){m_##name.set(val);}
#define PROP_NUM(type, name) RegExNum<type> m_##name; type Get##name(){return m_##name.get();} void Set##name( type val){m_##name.set(val);}
#define PROP_GET(type, name) ReadOnly<type> m_##name; type Get##name(){return m_##name.get();}

//#define DECL_PROP(type, name) public: type Get##name () { return m_##name ;} void Set##name ( type ); private: type m_##name
//#define DECL_STR_PROP(type, name, cb) public: const type* Get##name () { return m_##name ;} void Set##name ( const type* ); private: type m_##name[cb];

struct ComPort
{
	ComPort(CRegKeyExt* pReg, LPCTSTR name, LPCTSTR speed, LPCTSTR dataBits, LPCTSTR stopBits, LPCTSTR parity, LPCTSTR flowControl) :
		m_Name(pReg, name, MAX_PATH)
		, m_Speed(pReg, speed, 115200)
		, m_DataBits(pReg, dataBits, 8)
		, m_StopBits(pReg, stopBits, 1)
		, m_Parity(pReg, parity, 0)
		, m_FlowControl(pReg, flowControl, 0)
	{}
	PROP_STR(Name);
	PROP_NUM(DWORD, Speed);
	PROP_NUM(BYTE, DataBits);
	PROP_NUM(DWORD, StopBits);
	PROP_NUM(DWORD, Parity);
	PROP_NUM(DWORD, FlowControl);
	static char* GetParityName(int i) { if (i == 0) return "None"; if (i == 1) return "Odd"; if (i == 2) return "Even"; if (i == 3) return "Mark"; if (i == 4) return "Space"; return "???"; }
	static char* GetFlowControlName(int i) { if (i == 0) return "None"; if (i == 1) return "XON/XOFF"; if (i == 2) return "RTS/CTS"; if (i == 3) return "DSR/DTR"; return "???"; }
};

class CSettings : public CRegKeyExt
{
public:
    CSettings();
    ~CSettings();

    void RestoreWindPos(HWND hWnd);
    void SaveWindPos(HWND hWnd);
	bool SetTraceColor(int color, DWORD& textColor, DWORD& bkColor);
	void SetUIFont(CHAR* lfFaceName, LONG lfWeight, LONG lfHeight);

	StringList m_adbFilterList;
	StringList& getAdbFilterList() {
		return m_adbFilterList;
	}

	StringList m_processFilterList;
	StringList& getProcessFilterList() {
		return m_processFilterList;
	}

    void SetModules(const CHAR* szList);
	CHAR* GetModules();
	void SetSearchList(CHAR* szList);
    CHAR* GetSearchList();
	static DWORD CSettings::InfoTextColor() { return RGB(128, 128, 128); }
	static DWORD CSettings::InfoTextColorNative() { return RGB(0, 0, 0); }
	static DWORD CSettings::InfoTextColorAndroid() { return RGB(180, 140, 10); }
	static DWORD CSettings::InfoTextColorSerial() { return RGB(64, 172, 64); }
	static DWORD CSettings::SerachColor() { return RGB(0xA0, 0xA9, 0x3d); }
	static DWORD CSettings::CurSerachColor() { return RGB(64, 128, 64); }
	static DWORD CSettings::LogListTxtColor() { return RGB(176, 176, 176); }
	static DWORD CSettings::LogListBkColor() { return RGB(0, 0, 0); }
	static DWORD CSettings::LogListInfoBkColor() { return RGB(240, 240, 240); }
	static DWORD CSettings::CurSelectionTxtColor() { return RGB(255, 255, 255); }
	static DWORD CSettings::SelectionBkColor() { return RGB(64, 64, 64); }
	static DWORD CSettings::CurSelectionBkColor() { return RGB(64, 122, 255); }
	bool CanShowInEclipse() { return *GetEclipsePath() != 0 && GetResolveAddr(); }
	bool CanShowInAndroidStudio() { return *GetAndroidStudio() != 0 && *GetAndroidProject() != 0; }

	PROP_NUM(int, VertSplitterPos);
	PROP_NUM(int, HorzSplitterPos);
	PROP_GET(DWORD, fontSize);

	PROP_NUM(int, StackSplitterPos);
	PROP_GET(HFONT, Font);
	PROP_GET(int, FontHeight);
	PROP_GET(int, FontWidth);

	PROP_NUM(DWORD, FlowTracesHiden);
	PROP_NUM(DWORD, TreeViewHiden);
	PROP_NUM(DWORD, InfoHiden);
	PROP_NUM(DWORD, ShowAppIp);
	PROP_NUM(DWORD, ShowElapsedTime);
	PROP_NUM(DWORD, ResolveAddr);
	PROP_NUM(DWORD, FullSrcPath);

	PROP_NUM(int, ColNN);
	PROP_NUM(int, ColApp);
	PROP_NUM(int, ColPID);
	//PROP_NUM(int, ColTID);
	//PROP_NUM(int, ColThreadNN);
	PROP_NUM(int, ShowChildCount);
	PROP_NUM(int, ColFunc);
	PROP_NUM(int, ColLine);
	PROP_NUM(int, ColTime);
	PROP_NUM(int, ColCallAddr);
	PROP_NUM(int, FnCallLine);
	PROP_NUM(int, UseAdb);
	PROP_NUM(int, RestartAdb);
	PROP_NUM(int, ApplyLogcutFilter);
	PROP_NUM(int, ApplyPorcessFilter);

	PROP_NUM(DWORD, UdpPort);
	PROP_NUM(DWORD, RawTcpPort);
	PROP_STR(AdbArg);
	PROP_STR(LogcatArg);

	PROP_GET(CHAR*, FontName);
	PROP_GET(DWORD, FontWeight);

	PROP_STR(EclipsePath);
	PROP_STR(ExternalCmd_1);
	PROP_STR(ExternalCmd_2);
	PROP_STR(ExternalCmd_3);
	PROP_STR(LinuxHome);
	PROP_STR(MapOnWin);
	PROP_STR(AndroidStudio);
	PROP_STR(AndroidProject);

	PROP_NUM(int, UseComPort_1);
	PROP_NUM(int, UseComPort_2);
	ComPort comPort_1;
	ComPort comPort_2;

private:
    LOGFONT   m_logFont;
    HANDLE m_resourceFonthandle;

    void AddDefaultFont();
	void InitFont();
	void DeleteFont();
};

extern CSettings gSettings;