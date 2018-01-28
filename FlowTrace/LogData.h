#pragma once
class Archive;
struct LOG_NODE;
struct ROOT_NODE;
struct APP_NODE;
struct PROC_NODE;
struct TRACE_NODE;
struct FLOW_NODE;
struct INFO_DATA;
struct LOG_DATA;
struct TRACE_DATA;
struct FLOW_DATA;
struct LIST_NODES;

extern ROOT_NODE* rootNode;
extern LIST_NODES* listNodes;
extern Archive  gArchive;

typedef enum { LOG_TYPE_ENTER, LOG_TYPE_EXIT, LOG_TYPE_TRACE } ROW_LOG_TYPE;

#pragma pack(push,1)
enum PROCESS_NODE_CHILD { ROOT_CHILD, LATEST_CHILD };
enum LOG_DATA_TYPE { APP_DATA_TYPE, PROC_DATA_TYPE, FLOW_DATA_TYPE, TRACE_DATA_TYPE };
enum LIST_COL { ICON_COL, LINE_NN_COL, NN_COL, APP_COLL, PROC_COL, TIME_COL, FUNC_COL, CALL_LINE_COL, LOG_COL, MAX_COL };

struct ADDR_INFO
{
  DWORD addr;
  DWORD line;
  char* src;
  ADDR_INFO* pPrev;
};

struct LOG_DATA
{
  LOG_DATA_TYPE data_type;
  DWORD lengthCalculated;
  void init() { lengthCalculated = 0; }
  bool isApp() { return data_type == APP_DATA_TYPE; }
  bool isProc() { return data_type == PROC_DATA_TYPE; }
  bool isFlow() { return data_type == FLOW_DATA_TYPE; }
  bool isTrace() { return data_type == TRACE_DATA_TYPE; }
  bool isInfo() { return isFlow() || isTrace(); }
};

struct APP_DATA : LOG_DATA
{
  DWORD app_sec;
  DWORD app_msec;
  DWORD local_sec;
  DWORD local_msec;
  int threadCount;
  DWORD NN;
  DWORD lost;
  char ip_address[66];
  int cb_app_path;
  int cb_app_name;
  ADDR_INFO *p_addr_info;
  DWORD cb_addr_info;
  char* appPath() { return ((char*)(this)) + sizeof(APP_DATA); }
  char* appName() { return appPath() + (cb_app_path - cb_app_name); }
  void Log() { stdlog("name: %s sec: %d msec: %d\n", "appName()", app_sec, app_msec); }
};

struct PROC_DATA : LOG_DATA
{
  APP_NODE* pAppNode;
  FLOW_NODE* curentFlow;
  TRACE_NODE* latestTrace;
  int emptLineColor;
  int tid;
  int threadNN;
  char COLOR_BUF[10];
  int  cb_color_buf;
};

struct INFO_DATA : LOG_DATA
{
  int  nn;
  int log_type;
  int call_line;
  int cb_fn_name;
  DWORD pc_sec;
  DWORD pc_msec;
  DWORD term_sec;
  DWORD term_msec;
  bool isEnter() { return log_type == LOG_TYPE_ENTER; }
  bool isTrace() { return log_type == LOG_TYPE_TRACE; }
};

struct FLOW_DATA : INFO_DATA
{
  FLOW_NODE* peer;
  void* this_fn;
  void* call_site;
  char* fnName() { return ((char*)(this)) + sizeof(FLOW_DATA); }
};

struct TRACE_CHANK
{
  TRACE_CHANK* next_chank;
  int len;
  char trace[1];
};

struct TRACE_DATA : INFO_DATA
{
  BYTE color;
  int cb_trace;
  char* fnName() { return ((char*)(this)) + sizeof(TRACE_DATA); }
  TRACE_CHANK* getFirestChank(){ return (TRACE_CHANK*)(fnName() + cb_fn_name); }
  TRACE_CHANK* getLastChank() {TRACE_CHANK* p = getFirestChank(); while (p->next_chank) p = p->next_chank; return p; }
};

struct LOG_NODE
{
#ifdef _DEBUG
  int index;
#endif
  LOG_DATA* data;
  PROC_NODE* proc;
  LOG_NODE* parent;
  LOG_NODE* lastChild;
  LOG_NODE* prevSibling;
  ADDR_INFO *p_addr_info;
  struct {
    WORD hasNewLine : 1;
    WORD hasSearchResult : 1;
    WORD hiden : 1;
    WORD hasCheckBox : 1;
    WORD checked : 1;
    WORD selected : 1;
    WORD expanded : 1;
    WORD hasNodeBox : 1;
    WORD pathExpanded : 1;
    WORD bookmark : 1;
  };
#ifdef NATIVE_TREE
  HTREEITEM htreeitem;
#else
  BYTE nextChankCounter;
  int cExpanded;
  int line;
  LOG_NODE* nextChankMarker;
  LOG_NODE* nextChank;
  LOG_NODE* firstChild;
  LOG_NODE* nextSibling;
  void CalcLines();
  int GetExpandCount() { return expanded ? cExpanded : 0; }
  void CollapseExpand(BOOL expand);
  int GetPosInTree() { return line; }
  void CollapseExpandAll(bool expand);
#endif

  void init(LOG_DATA*  p) { ZeroMemory(this, sizeof(LOG_NODE)); data = p; }

  void add_child(LOG_NODE* pNode)
  {
#ifndef NATIVE_TREE
    if (!firstChild)
      firstChild = pNode;
    if (lastChild)
      lastChild->nextSibling = pNode;

    nextChankCounter++;
    if (nextChankCounter == 255) // 255 is max number of nextChankCounter. then it will start from 0 
    {
      if (!nextChankMarker)
        firstChild->nextChank = pNode;
      else
        nextChankMarker->nextChank = pNode;
      nextChankMarker = pNode;
    }
#endif
    pNode->prevSibling = lastChild;
    pNode->parent = this;
    lastChild = pNode;
  }

  bool isRoot() { return ((void*)this) == ((void*)rootNode); }
  bool isApp() { return data && data->isApp(); }
  bool isProc() { return data && data->isProc(); }
  bool isFlow() { return data && data->isFlow(); }
  bool isTrace() { return data && data->isTrace(); }
  bool isInfo() { return data && data->isInfo(); }
  bool isTreeNode() { return parent && !isTrace(); }
  bool isParent(LOG_NODE* pNode){ LOG_NODE* p = parent; while (p != pNode && p) p = p->parent; return p != NULL; }
  bool isSynchronized(LOG_NODE* pSyncNode);
  FLOW_NODE* getPeer() { return isFlow() ? (((FLOW_DATA*)data)->peer) : 0; }
  int getNN();
  LONG getTimeSec();
  LONG getTimeMSec();
  int   getPid();
  void* getCallAddr();
  int   getCallLine();
  TCHAR* getTreeText(int* cBuf = NULL, bool extened = true);
  TCHAR* getListText(int* cBuf, LIST_COL col, int iItem = 0);
  int getTreeImage();
  LOG_NODE* getSyncNode();
  APP_NODE* getApp() { LOG_NODE* p = this; while (p && !p->isApp()) p = p->parent; return (APP_NODE*)p; }
  char* getFnName();
  int getFnNameSize();
  bool PendingToResolveAddr();
  char* getSrcName(bool fullPath);
  int getTraceText(char* pBuf, int max_cb_trace);
};

struct ROOT_NODE : LOG_NODE
{
};

struct APP_NODE : LOG_NODE
{
  APP_DATA* getData() { ATLASSERT(data->isApp()); return (APP_DATA*)data; }
};

struct FLOW_NODE : LOG_NODE
{
  FLOW_DATA* getData() { ATLASSERT(data->isFlow()); return (FLOW_DATA*)data; }
  bool isOpenEnter() { ATLASSERT(data->isFlow()); return ( (FLOW_DATA*)data)->isEnter() && ((FLOW_DATA*)data)->peer == 0; }
  void addToTree();
};

struct PROC_NODE : LOG_NODE
{
  PROC_DATA* getData() { ATLASSERT(data->isProc()); return (PROC_DATA*)data; }

  void add_proc_child(FLOW_NODE* pNode, PROCESS_NODE_CHILD type)
  {
    LOG_NODE* pParent = NULL;
    if (type == ROOT_CHILD)
      pParent = this;
    else
      pParent = getData()->curentFlow;
    ATLASSERT(pParent != NULL);
    pParent->add_child(pNode);
    getData()->curentFlow = pNode;
  }
  bool isHiden(){ return hiden || ((PROC_DATA*)data)->pAppNode->hiden; }
};

struct TRACE_NODE : LOG_NODE
{
  TRACE_DATA* getData() { ATLASSERT(data->isTrace()); return (TRACE_DATA*)data; }
};

#pragma pack(pop)

