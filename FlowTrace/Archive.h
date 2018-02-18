#pragma once

#include "LogData.h"

#define MAX_TRCAE_LEN (1024*2)
#define MAX_RECORD_LEN (MAX_TRCAE_LEN + 2 * sizeof(ROW_LOG_REC))
class Addr2LineThread;
struct ListedNodes;

#pragma pack(push,4)
typedef struct
{
    int len;
    int log_type;
    int nn;
    int cb_app_path;
    int cb_fn_name;
    int cb_trace;
    int tid;
    DWORD app_sec;
    DWORD app_msec;
    DWORD sec;
    DWORD msec;
    DWORD this_fn;
    DWORD call_site;
    int call_line;
    char data[1];

    char* appPath() { return data; }
    char* fnName() { return data + cb_app_path; }
    char* trace() { return data + cb_app_path + cb_fn_name; }
    int cbData() { return cb_app_path + cb_fn_name + cb_trace; }
    int size() { return sizeof(ROW_LOG_REC) + cbData(); }
    bool isValid() {
        return cb_app_path >= 0 && cb_fn_name >= 0 && cb_trace >= 0 && len > sizeof(ROW_LOG_REC) && len >= size() && size() < MAX_RECORD_LEN;
    }
    bool isFlow() { return log_type == LOG_TYPE_ENTER || log_type == LOG_TYPE_EXIT; }
    bool isTrace() { return log_type == LOG_TYPE_TRACE; }
    void Log() { stdlog("name: %s sec: %d msec: %d\n", "appName()", app_sec, app_msec); }
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
    DWORD getCount();
    LOG_NODE* getNode(DWORD i) { return (m_pNodes && i < m_pNodes->Count()) ? (LOG_NODE*)m_pNodes->Get(i) : 0; }
    char* Alloc(DWORD cb) { return (char*)m_pMemBuf->Alloc(cb); }
    bool append(ROW_LOG_REC* rec, DWORD pc_sec, DWORD pc_msec, sockaddr_in *p_si_other = NULL);
    bool IsEmpty() { return m_pNodes ==nullptr || m_pNodes->Count() == 0; }
    DWORD64 index(LOG_NODE* pNode) { return pNode - getNode(0); }
    void resolveAddrAsync(LOG_NODE* pNode = NULL);
    void resolveAddr(LOG_NODE* pSelectedNode, bool loop);
    ListedNodes* getListedNodes() { return m_listedNodes; }
    ROOT_NODE* getRootNode() { return m_rootNode; }
    SNAPSHOT& getSNAPSHOT() { return m_snapshot; }
    static DWORD getArchiveNumber() { return archiveNumber; }

private:
    inline APP_NODE* addApp(char* app_path, int cb_app_path, DWORD app_sec, DWORD app_msec, DWORD nn, sockaddr_in *p_si_other);
    inline PROC_NODE* addProc(APP_NODE* pAppNode, int tid);
    inline LOG_NODE* addFlow(PROC_NODE* pProcNode, ROW_LOG_REC *pLogRec, DWORD pc_sec, DWORD pc_msec);
    inline LOG_NODE* addTrace(PROC_NODE* pProcNode, ROW_LOG_REC *pLogRec, int& prcessed, DWORD pc_sec, DWORD pc_msec);
    inline APP_NODE*   getApp(ROW_LOG_REC* p, sockaddr_in *p_si_other);
    inline PROC_NODE*   getProc(APP_NODE* pAppNode, ROW_LOG_REC* p);

    static DWORD archiveNumber;
    APP_NODE* curApp;
    PROC_NODE* curProc;
    SNAPSHOT m_snapshot;
    ListedNodes* m_listedNodes;
    ROOT_NODE* m_rootNode;
    Addr2LineThread* m_pAddr2LineThread;
    MemBuf* m_pMemBuf;
    PtrArray* m_pNodes;
};

struct ListedNodes
{
    ListedNodes(MemBuf* pMemBuf, unsigned long maxCount) {
        m_maxCount = maxCount;
        m_pNodes = (LOG_NODE**)pMemBuf->Alloc(maxCount * sizeof(LOG_NODE**));
        init();
    }
    LOG_NODE* getNode(DWORD i) {
        return (i < m_listedCount) ? m_pNodes[i] : 0;
    }
    void addNode(LOG_NODE* pNode, BOOL flowTraceHiden) {
        if (m_listedCount < m_maxCount)
        {
            DWORD64 ndx = gArchive.index(pNode);
            if (ndx >= gArchive.getSNAPSHOT().first && ndx <= gArchive.getSNAPSHOT().last && pNode->isInfo() && !pNode->proc->isHiden() && (pNode->isTrace() || !flowTraceHiden))
            {
                m_pNodes[m_listedCount++] = pNode;
            }
        }
    }
    void init() {
        m_listedCount = 0; archiveCount = 0;
    }
    DWORD Count() { return m_listedCount; }
    void applyFilter(BOOL flowTraceHiden);
    void updateList(BOOL flowTraceHiden);
private:
    DWORD m_listedCount;
    DWORD archiveCount;
    DWORD m_maxCount;
    LOG_NODE** m_pNodes;
};

