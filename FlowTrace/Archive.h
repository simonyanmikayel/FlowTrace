#pragma once

#include "LogData.h"

#define MAX_JAVA_FUNC_NAME_LEN 1200
#define MAX_TRCAE_LEN (1024*2)
#define MAX_RECORD_LEN (MAX_TRCAE_LEN + 2 * sizeof(ROW_LOG_REC))
struct ListedNodes;

#define PARCE_COLOR

#ifdef _BUILD_X64
const size_t MAX_BUF_SIZE = 12LL * 1024 * 1024 * 1024;
#else
const size_t MAX_BUF_SIZE = 1024 * 1024 * 1024;
#endif

#pragma pack(push,4)
struct LOG_REC_BASE
{
	int len;
	BYTE log_type;
	BYTE log_flags;
	BYTE color;
	BYTE severity;
	unsigned int nn;
	WORD cb_app_name;
	WORD cb_module_name;
	WORD cb_fn_name;
	union {
		WORD cb_trace;
		WORD cb_java_call_site; // for java we keep here caller class:method
	};
	int tid;
	int pid;
	DWORD sec;
	DWORD msec;
	DWORD this_fn;
	DWORD call_site;
	int fn_line;
	int call_line;
};
#ifdef _USE_ADB
struct ROW_LOG_REC : LOG_REC_BASE
{
	const char* appName() { return p_app_name; }
	const char* moduleName() { return p_module_name; }
	const char* fnName() { return p_fn_name; }
	const char* trace() { return p_trace; }
	int cbModuleName() { return cb_module_name; }
	int cbData() { return cb_app_name + cb_module_name + cb_fn_name + cb_trace; }
	int size() { return sizeof(LOG_REC_BASE) + cbData(); }
	bool isValid() {
		int cb_data = cbData();
		int cb_size = size();
		return len >= cb_size && cb_app_name >= 0 && cb_fn_name >= 0 && cb_trace >= 0 && len > sizeof(ROW_LOG_REC) && cb_size < MAX_RECORD_LEN;
	}
	bool isFlow() { return log_type == LOG_TYPE_ENTER || log_type == LOG_TYPE_EXIT; }
	bool isTrace() { return log_type == LOG_TYPE_TRACE; }
	void reset() { ZeroMemory(this, sizeof(ROW_LOG_REC)); resetFT(); }
	void resetFT() { ftChecked = false; log_type = LOG_TYPE_TRACE; p_app_name = p_module_name = p_fn_name = ""; }
	
	bool ftChecked; //is adb sent flow trace
	const char* p_app_name;
	const char* p_module_name;
	const char* p_fn_name;
	const char* p_trace;
};
#else
struct ROW_LOG_REC : LOG_REC_BASE
{
	const char* appName() { return data; }
	const char* moduleName() { return cb_module_name ? (appName() + cb_app_name) : appName(); }
	const char* fnName() { return moduleName() + cbModuleName(); }
	const char* trace() { return fnName() + cb_fn_name; }
	int cbModuleName() { return cb_module_name ? cb_module_name : cb_app_name; }
	int cbData() { return cb_app_name + cb_module_name + cb_fn_name + cb_trace; }
	int size() { return sizeof(LOG_REC_BASE) + cbData(); }
	bool isValid() {
		int cb_data = cbData();
		int cb_size = size();
		return len >= cb_size && cb_app_name >= 0 && cb_fn_name >= 0 && cb_trace >= 0 && len > sizeof(ROW_LOG_REC) && cb_size < MAX_RECORD_LEN;
	}
	bool isFlow() { return log_type == LOG_TYPE_ENTER || log_type == LOG_TYPE_EXIT; }
	bool isTrace() { return log_type == LOG_TYPE_TRACE; }

private:
	char data[1];

};
#endif //_USE_ADB

typedef struct
{
    int data_len;
    int pack_nn;
	int retry_nn;
	int buff_nn;
	int retry_delay;
	int retry_count;
}NET_PACK_INFO;
#pragma pack(pop)

struct SNAPSHOT_INFO
{
    int pos;
    CHAR descr[32];
};

struct SNAPSHOT
{
    void clear() { curSnapShot = 0;  snapShots.clear(); update(); }
    void update();
    int curSnapShot;
    std::vector<SNAPSHOT_INFO> snapShots;
    DWORD first, last;
};

class Archive
{
public:
    Archive();
    ~Archive();

    void clearArchive(bool closing = false);
    void onPaused();
    DWORD getCount();
    LOG_NODE* getNode(DWORD i) { return (m_pNodes && i < m_pNodes->Count()) ? (LOG_NODE*)m_pNodes->Get(i) : 0; }
    char* Alloc(DWORD cb, bool zero = false) { return (char*)m_pTraceBuf->Alloc(cb, zero); }
    int append(ROW_LOG_REC* rec, sockaddr_in *p_si_other = NULL, bool fromImport = false, int bookmark = 0, NET_PACK_INFO* pack = 0);
    bool IsEmpty() { return m_pNodes == nullptr || m_pNodes->Count() == 0; }
    DWORD64 index(LOG_NODE* pNode) { return pNode - getNode(0); }
    ListedNodes* getListedNodes() { return m_listedNodes; }
    ROOT_NODE* getRootNode() { return m_rootNode; }
    SNAPSHOT& getSNAPSHOT() { return m_snapshot; }
    static DWORD getArchiveNumber() { return archiveNumber; }
    BYTE getNewBookmarkNumber() { return ++bookmarkNumber; }
    BYTE resteBookmarkNumber() { return bookmarkNumber = 0; }
    size_t UsedMemory();
    DWORD getLost() { return m_lost; }
    void Log(ROW_LOG_REC* rec);
	bool setAppName(int pid, char* szName, int cbName);

private:
    inline APP_NODE* addApp(ROW_LOG_REC* p, sockaddr_in *p_si_other);
    inline THREAD_NODE* addThread(ROW_LOG_REC* p, APP_NODE* pAppNode);
    inline LOG_NODE* addFlow(THREAD_NODE* pThreadNode, ROW_LOG_REC *pLogRec, int bookmark);
	inline LOG_NODE* addTrace(THREAD_NODE* pThreadNode, ROW_LOG_REC *pLogRec, unsigned char* trace, int cb_trace, int color, int bookmark);
#ifdef PARCE_COLOR
	inline LOG_NODE* addTrace(THREAD_NODE* pThreadNode, ROW_LOG_REC *pLogRec, int bookmark);
#endif //PARCE_COLOR
	inline APP_NODE*   getApp(ROW_LOG_REC* p, sockaddr_in *p_si_other);
	inline THREAD_NODE*   getThread(APP_NODE* pAppNode, ROW_LOG_REC* p);
    DWORD m_lost;
    static DWORD archiveNumber;
    BYTE bookmarkNumber;
    APP_NODE* curApp;
    THREAD_NODE* curThread;
    SNAPSHOT m_snapshot;
    ListedNodes* m_listedNodes;
    ROOT_NODE* m_rootNode;
    MemBuf* m_pTraceBuf;
    PtrArray<LOG_NODE>* m_pNodes;
};

struct ListedNodes
{
    ListedNodes() {
        ZeroMemory(this, sizeof(*this));
        m_pListBuf = new MemBuf(MAX_BUF_SIZE, 64 * 1024 * 1024);
    }
    ~ListedNodes() {
        delete m_pNodes;
        delete m_pListBuf;
    }
    LOG_NODE* getNode(DWORD i) {
        return m_pNodes->Get(i);
    }
    void Free()
    {
        m_pListBuf->Free();
        delete m_pNodes;
        archiveCount = 0;
        m_pNodes = new PtrArray<LOG_NODE>(m_pListBuf);
    }
    size_t UsedMemory() {
        return m_pListBuf->UsedMemory();
    }
    void addNode(LOG_NODE* pNode, BOOL flowTraceHiden) {
        DWORD64 ndx = gArchive.index(pNode);
        if (ndx >= gArchive.getSNAPSHOT().first && ndx <= gArchive.getSNAPSHOT().last && pNode->isInfo() && !pNode->threadNode->isHiden() && (pNode->isTrace() || !flowTraceHiden))
        {
            m_pNodes->AddPtr(pNode);
        }
    }
    DWORD Count() { return m_pNodes->Count(); }
    void applyFilter(BOOL flowTraceHiden);
    void updateList(BOOL flowTraceHiden);
private:
    DWORD archiveCount;
    PtrArray<LOG_NODE>* m_pNodes;
    MemBuf* m_pListBuf;
};

