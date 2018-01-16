#pragma once

#include "LogData.h"

#define MAX_TRCAE_LEN (1024*2)
#define MAX_RECORD_LEN (MAX_TRCAE_LEN + 2 * sizeof(ROW_LOG_REC))
class Addr2LineThread;

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
  void* this_fn;
  void* call_site;
  int call_line;
  char data[1];

  char* appPath() { return data; }
  char* fnName() { return data + cb_app_path; }
  char* trace() { return data + cb_app_path + cb_fn_name; }
  int cbData() { return cb_app_path + cb_fn_name + cb_trace; }
  int size() { return sizeof(ROW_LOG_REC) + cbData(); }
  bool isValid() { 
    return cb_app_path >= 0 &&  cb_fn_name >= 0 && cb_trace >= 0 && len > sizeof(ROW_LOG_REC) && len >= size() && size() < MAX_RECORD_LEN;
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

class Archive
{
public:
  Archive();
  ~Archive();

  void clear();
  DWORD getCount();
  LOG_NODE* getNode(DWORD i)  { return (i < c_rec) ? node_array - i : 0; }
  LOG_DATA* getData(DWORD i)  { return (i < c_rec) ? (node_array - i)->data : 0; }
  bool append(ROW_LOG_REC* rec, DWORD pc_sec, DWORD pc_msec, sockaddr_in *p_si_other = NULL);
  bool IsEmpty() { return c_rec == 0; }
  DWORD index(LOG_NODE* pNode) { return node_array - pNode; }

private:
  inline APP_NODE* addApp(char* app_path, int cb_app_path, DWORD app_sec, DWORD app_msec, DWORD nn, sockaddr_in *p_si_other);
  inline PROC_NODE* addProc(APP_NODE* pAppNode, int tid);
  inline LOG_NODE* addFlow(PROC_NODE* pProcNode, ROW_LOG_REC *pLogRec, DWORD pc_sec, DWORD pc_msec);
  inline LOG_NODE* addTrace(PROC_NODE* pProcNode, ROW_LOG_REC *pLogRec, int& prcessed, DWORD pc_sec, DWORD pc_msec);
  inline APP_NODE*   getApp(ROW_LOG_REC* p, sockaddr_in *p_si_other);
  inline PROC_NODE*   getProc(APP_NODE* pAppNode, ROW_LOG_REC* p);

  //CRITICAL_SECTION cs;
  DWORD c_alloc;
  DWORD c_rec;
  DWORD c_max_rec;
  char* buf;
  char* rec_end;
  LOG_NODE* node_array;
  APP_NODE* curApp;
  PROC_NODE* curProc;
  Addr2LineThread* m_pAddr2LineThread;
};

struct SNAPSHOT_INFO
{
  int pos;
  TCHAR descr[32];
};

struct SNAPSHOT
{
  void clear() { curSnapShot = 0;  snapShots.clear(); update(); }
  void update();
  int curSnapShot;
  std::vector<SNAPSHOT_INFO> snapShots;
  DWORD first, last;
};
extern SNAPSHOT snapshot;

struct LIST_NODES
{
  LOG_NODE* getNode(DWORD i)  { return (i < listedCount) ? gArchive.getNode(index[i]) : 0; }
  void addNode(LOG_NODE* pNode, BOOL flowTraceHiden){
    DWORD ndx = gArchive.index(pNode);
    if (ndx >= snapshot.first && ndx <= snapshot.last && pNode->isInfo() && !pNode->proc->isHiden() && (pNode->isTrace() || !flowTraceHiden))
    { 
      index[listedCount++] = ndx;
    }
  }
  DWORD Count() { return listedCount; }
  void applyFilter(BOOL flowTraceHiden);
  void updateList(BOOL flowTraceHiden);
  void init() { listedCount = 0; archiveCount = 0; }
private:
  DWORD listedCount;
  DWORD archiveCount;
  DWORD index[1]; //index in node_array
};

