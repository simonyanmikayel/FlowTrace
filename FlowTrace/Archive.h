#pragma once

#include "LogData.h"

#define MAX_JAVA_TAG_NAME_LEN 500
#define MAX_LOG_LEN (1600)
#define MAX_RECORD_LEN (MAX_LOG_LEN + 2 * sizeof(LOG_REC_BASE_DATA))
#define MAX_NET_BUF 1416  //max UDP datagam is 65515 Bytes

struct ListedNodes;

#ifdef _BUILD_X64
const size_t MAX_BUF_SIZE = 12LL * 1024 * 1024 * 1024;
#else
const size_t MAX_BUF_SIZE = 1024 * 1024 * 1024;
#endif

struct PS_INFO {
	int pid;
	int ppid;
	char name[MAX_APP_NAME + 1];
	int cName;
};
#define maxPsInfo 32*1024 


#pragma pack(push,4)
struct LOG_REC_BASE_DATA
{
	int len;
	BYTE log_type;
	BYTE log_flags;
	BYTE color;
	BYTE priority;
	unsigned int nn;
	WORD cb_app_name;
	WORD cb_module_name;
	WORD cb_fn_name;
	WORD cb_trace;
	int tid;
	int pid;
	DWORD sec;
	DWORD msec;
	DWORD this_fn;
	DWORD call_site;
	int fn_line;
	int call_line;

	int cbData() { return cb_app_name + cb_module_name + cb_fn_name + cb_trace; }
	int size() { return sizeof(LOG_REC_BASE_DATA) + cbData(); }
	bool isValid() {
		int cb_data = cbData();
		int cb_size = size();
#ifdef DEBUG
		bool b1 = len >= cb_size, b2 = len > sizeof(LOG_REC_BASE_DATA), b3 = (cb_size < MAX_RECORD_LEN || log_type == LOG_TYPE_TRACE);
		if (!b1 || !b2 || !b3)
			ATLASSERT(0);
#endif
		return len >= cb_size && cb_app_name >= 0 && cb_fn_name >= 0 && cb_trace >= 0 && len > sizeof(LOG_REC_BASE_DATA) && (cb_size < MAX_RECORD_LEN || log_type == LOG_TYPE_TRACE);
	}
	bool isFlow() { return log_type == LOG_TYPE_ENTER || log_type == LOG_TYPE_EXIT; }
	bool isTrace() { return log_type == LOG_TYPE_TRACE; }
};

struct LOG_REC_NET_DATA : public LOG_REC_BASE_DATA
{
	const char* appName() { return data; }
	const char* moduleName() { return cb_module_name ? (appName() + cb_app_name) : appName(); }
	const char* fnName() { return moduleName() + cbModuleName(); }
	const char* trace() { return fnName() + cb_fn_name; }
	int cbModuleName() { return cb_module_name ? cb_module_name : cb_app_name; }

	char data[1];
};

struct LOG_REC_SERIAL_DATA : public LOG_REC_BASE_DATA
{
	const char* appName() { return p_app_name; }
	const char* moduleName() { return p_module_name; }
	const char* fnName() { return p_fn_name; }
	const char* trace() { return p_trace; }
	int cbModuleName() { return cb_module_name; }
	void reset() {
		ZeroMemory(this, sizeof(*this)); resetFT();
	}
	bool ok() {
		return pid != 0 && tid != 0;
	}
	void resetFT() { log_type = LOG_TYPE_TRACE; log_flags = LOG_FLAG_SERIAL;  p_trace = p_app_name = p_module_name = p_fn_name = ""; }

	const char* p_app_name;
	const char* p_module_name;
	const char* p_fn_name;
	const char* p_trace;
	char tag[MAX_JAVA_TAG_NAME_LEN];
};

struct LOG_REC_ADB_DATA : public LOG_REC_BASE_DATA
{
	const char* appName() { return p_app_name; }
	const char* moduleName() { return p_module_name; }
	const char* fnName() { return p_fn_name; }
	const char* trace() { return p_trace; }
	int cbModuleName() { return cb_module_name; }
	void reset() { 
		ZeroMemory(this, sizeof(*this)); resetFT(); 
	}
	bool ok() {
		return pid != 0 && tid != 0;
	}
	void resetFT() { log_type = LOG_TYPE_TRACE; log_flags = LOG_FLAG_ADB;  p_trace = p_app_name = p_module_name = p_fn_name = ""; }

	const char* p_app_name;
	const char* p_module_name;
	const char* p_fn_name;
	const char* p_trace;
	char tag[MAX_JAVA_TAG_NAME_LEN];
};

struct LOG_REC
{
	int cbData() { return getLogData()->cbData(); }
	int size() { return getLogData()->size(); }
	bool isValid() { return getLogData()->isValid(); }
	bool isFlow() { return getLogData()->isFlow(); }
	bool isTrace() { return getLogData()->isTrace(); }
	virtual LOG_REC_BASE_DATA* getLogData() = 0;
	virtual const char* appName() = 0;
	virtual const char* moduleName() = 0;
	virtual const char* fnName() = 0;
	virtual const char* trace() = 0;
	virtual int cbModuleName() = 0;
};

struct LOG_REC_NET : public LOG_REC
{
	LOG_REC_NET(LOG_REC_NET_DATA* p) { pLogData = p; };
	LOG_REC_BASE_DATA* getLogData() { return pLogData; };
	const char* appName() { return pLogData->appName(); }
	const char* moduleName() { return pLogData->moduleName(); }
	const char* fnName() { return pLogData->fnName(); }
	const char* trace() { return pLogData->trace(); }
	int cbModuleName() { return pLogData->cbModuleName(); }
private:
	LOG_REC_NET_DATA* pLogData;
};

struct LOG_REC_ADB : public LOG_REC
{
	LOG_REC_ADB(LOG_REC_ADB_DATA* p) { pLogData = p; };
	LOG_REC_BASE_DATA* getLogData() { return pLogData; };
	const char* appName() { return pLogData->appName(); }
	const char* moduleName() { return pLogData->moduleName(); }
	const char* fnName() { return pLogData->fnName(); }
	const char* trace() { return pLogData->trace(); }
	int cbModuleName() { return pLogData->cbModuleName(); }
private:
	LOG_REC_ADB_DATA* pLogData;
};

struct LOG_REC_SERIAL : public LOG_REC
{
	LOG_REC_SERIAL(LOG_REC_SERIAL_DATA* p) { pLogData = p; };
	LOG_REC_BASE_DATA* getLogData() { return pLogData; };
	const char* appName() { return pLogData->appName(); }
	const char* moduleName() { return pLogData->moduleName(); }
	const char* fnName() { return pLogData->fnName(); }
	const char* trace() { return pLogData->trace(); }
	int cbModuleName() { return pLogData->cbModuleName(); }
private:
	LOG_REC_SERIAL_DATA* pLogData;
};

struct NET_PACK_INFO
{
    int data_len;
    int pack_nn;
	int retry_nn;
	int buff_nn;
	int retry_delay;
	int retry_count;
};

enum NET_PACK_STATE { NET_PACK_FREE, NET_PACK_BUZY, NET_PACK_READY };
struct NET_PACK
{
	NET_PACK_INFO info;
	char data[MAX_NET_BUF];
	sockaddr_in si_other;
	static int PackSize() { return sizeof(NET_PACK_INFO) + MAX_NET_BUF; }
	NET_PACK_STATE state;
};
#pragma pack(pop)

class Archive
{
public:
    Archive();
    ~Archive();

    void clearArchive(bool closing = false);
    void onPaused();
	DWORD getNodeCount() { return m_pNodes ? m_pNodes->Count() : 0; }
	//DWORD getRecCount() { return m_pRecords ? m_pRecords->Count() : 0; }
	LOG_NODE* getNode(DWORD i) { return (m_pNodes && i < m_pNodes->Count()) ? (LOG_NODE*)m_pNodes->Get(i) : 0; }
    char* Alloc(DWORD cb, bool zero = false) { return (char*)m_pTraceBuf->Alloc(cb, zero); }
    bool IsEmpty() { return m_pNodes == nullptr || m_pNodes->Count() == 0; }
    DWORD64 index(LOG_NODE* pNode) { return pNode - getNode(0); }
    ListedNodes* getListedNodes() { return m_listedNodes; }
    ROOT_NODE* getRootNode() { return m_rootNode; }
    static DWORD getArchiveNumber() { return archiveNumber; }
    BYTE getNewBookmarkNumber() { return ++bookmarkNumber; }
    BYTE resteBookmarkNumber() { return bookmarkNumber = 0; }
    size_t AllocMemory();
    DWORD getLost() { return m_lost; }
    void Log(LOG_REC* rec);
	int appendSerial(LOG_REC_SERIAL_DATA* pLogData);
	int appendAdb(LOG_REC_ADB_DATA* pLogData, bool fromImport = false);
	int appendNet(LOG_REC_NET_DATA* pLogData, sockaddr_in *p_si_other = NULL, bool fromImport = false, int bookmark = 0, NET_PACK_INFO* pack = 0);
	bool setPsInfo(PS_INFO* p, int c);
	//void updateNodes();
	DWORD m_SkipedLogcat;

private:
	void appendRec(LOG_REC* rec, sockaddr_in *p_si_other, bool fromImport, int bookmark, NET_PACK_INFO* pack);
	inline APP_NODE* addApp(LOG_REC* p, sockaddr_in *p_si_other);
    inline THREAD_NODE* addThread(LOG_REC* p, APP_NODE* pAppNode);
    inline LOG_NODE* addFlow(THREAD_NODE* pThreadNode, LOG_REC *pLogRec, int bookmark, bool fromImport);
	inline LOG_NODE* addTrace(THREAD_NODE* pThreadNode, LOG_REC *pLogRec, int bookmark, bool fromImport);
	inline LOG_NODE* addTraceLoop(THREAD_NODE* pThreadNode, LOG_REC_BASE_DATA* pLogData, int bookmark, bool fromImport, char* trace, int cb_trace, const char* fnName, const char* moduleName);
	inline LOG_NODE* addTrace(THREAD_NODE* pThreadNode, LOG_REC_BASE_DATA* pLogData, int bookmark, bool fromImport, char* trace, int cb_trace, const char* fnName, const char* moduleName);
	inline APP_NODE*   getApp(LOG_REC* p, sockaddr_in *p_si_other);
	inline THREAD_NODE*   getThread(APP_NODE* pAppNode, LOG_REC* p);
	APP_NODE* setAppName(int pid, char* szName, int cbName, bool& updateViews);
	THREAD_NODE* setThreadName(APP_NODE*  app, int tid, char* szName, int cbName, bool& updateViews);
	bool resolveAppName(LOG_REC* p, APP_NODE* app);
	DWORD m_lost;
	int m_psNN;
	static DWORD archiveNumber;
    BYTE bookmarkNumber;
    APP_NODE* curApp;
    THREAD_NODE* curThread;
    ListedNodes* m_listedNodes;
    ROOT_NODE* m_rootNode;
	MemBuf* m_pTraceBuf;
	//MemBuf* m_pRecBuf;
	PtrArray<LOG_NODE>* m_pNodes;
	//PtrArray<LOG_REC_BASE_DATA>* m_pRecords;
	PS_INFO psInfo[maxPsInfo + 1];
	int cPsInfo;
};

struct ListedNodes
{
    ListedNodes() {
        ZeroMemory(this, sizeof(*this));
        m_pListBuf = new MemBuf(MAX_BUF_SIZE, 64 * 1024 * 1024);
    }
    ~ListedNodes() {
        delete m_pListNodes;
        delete m_pListBuf;
    }
    LOG_NODE* getNode(DWORD i) {
        return m_pListNodes->Get(i);
    }
    void Free()
    {
        m_pListBuf->Free();
        delete m_pListNodes;
        archiveCount = 0;
        m_pListNodes = new PtrArray<LOG_NODE>(m_pListBuf);
    }
    size_t AllocMemory() {
        return m_pListBuf->AllocMemory();
    }
    void addNode(LOG_NODE* pNode, BOOL flowTraceHiden) {
        DWORD64 ndx = gArchive.index(pNode);
		if (pNode->threadNode && pNode->isInfo() && !pNode->threadNode->isHiden() && (pNode->isTrace() || !flowTraceHiden))
		{
		    m_pListNodes->AddPtr(pNode);
		}
	}
    DWORD Count() { return m_pListNodes->Count(); }
    void applyFilter(BOOL flowTraceHiden);
	void updateList(BOOL flowTraceHiden);
private:
    DWORD archiveCount;
    PtrArray<LOG_NODE>* m_pListNodes;
    MemBuf* m_pListBuf;
};

