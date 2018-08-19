#pragma once

#include "LogData.h"

#define MAX_TRCAE_LEN (1024*2)
#define MAX_RECORD_LEN (MAX_TRCAE_LEN + 2 * sizeof(ROW_LOG_REC))
class Addr2LineThread;
struct ListedNodes;

#ifdef _BUILD_X64
const size_t MAX_BUF_SIZE = 12LL * 1024 * 1024 * 1024;
#else
const size_t MAX_BUF_SIZE = 1024 * 1024 * 1024;
#endif

#pragma pack(push,4)
typedef struct
{
    int len;
    WORD log_type;
    WORD log_flags;
    int nn;
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
    char data[1];

    char* appName() { return data; }
    char* moduleName() { return cb_module_name ? (appName() + cb_app_name) : appName(); }
    char* fnName() { return moduleName() + cb_module_name; }
    char* trace() { return fnName() + cb_fn_name; }
    int cbModuleName() { return cb_module_name ? cb_module_name : cb_app_name; }
    int cbData() { return cb_app_name + cb_module_name + cb_fn_name + cb_trace; }
    int size() { return sizeof(ROW_LOG_REC) + cbData(); }
    bool isValid() {
        int cb_data = cbData();
        int cb_size = size();
        return len >= cb_size && cb_app_name >= 0 && cb_fn_name >= 0 && cb_trace >= 0 && len > sizeof(ROW_LOG_REC) && cb_size < MAX_RECORD_LEN;
    }
    bool isFlow() { return log_type == LOG_TYPE_ENTER || log_type == LOG_TYPE_EXIT; }
    bool isTrace() { return log_type == LOG_TYPE_TRACE; }
}ROW_LOG_REC;

typedef struct
{
    int data_len;
    DWORD term_sec;
    DWORD term_msec;
}UDP_PACK;
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
    char* Alloc(DWORD cb) { return (char*)m_pTraceBuf->Alloc(cb, false); }
    bool append(ROW_LOG_REC* rec, sockaddr_in *p_si_other = NULL);
    bool IsEmpty() { return m_pNodes == nullptr || m_pNodes->Count() == 0; }
    DWORD64 index(LOG_NODE* pNode) { return pNode - getNode(0); }
    void resolveAddrAsync(LOG_NODE* pNode = NULL);
    void resolveAddr(LOG_NODE* pSelectedNode, bool loop);
    ListedNodes* getListedNodes() { return m_listedNodes; }
    ROOT_NODE* getRootNode() { return m_rootNode; }
    SNAPSHOT& getSNAPSHOT() { return m_snapshot; }
    static DWORD getArchiveNumber() { return archiveNumber; }
    BYTE getNewBookmarkNumber() { return ++bookmarkNumber; }
    BYTE resteBookmarkNumber() { return bookmarkNumber = 0; }
    size_t UsedMemory();
    DWORD getLost() { return m_lost; }

private:
    inline APP_NODE* addApp(ROW_LOG_REC* p, sockaddr_in *p_si_other);
    inline THREAD_NODE* addThread(ROW_LOG_REC* p, APP_NODE* pAppNode);
    inline LOG_NODE* addFlow(THREAD_NODE* pThreadNode, ROW_LOG_REC *pLogRec);
    inline LOG_NODE* addTrace(THREAD_NODE* pThreadNode, ROW_LOG_REC *pLogRec, int& prcessed);
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
    Addr2LineThread* m_pAddr2LineThread;
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

