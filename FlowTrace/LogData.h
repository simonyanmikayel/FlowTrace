#pragma once
#include "Buffer.h"

class Archive;
struct LOG_NODE;
struct ROOT_NODE;
struct APP_NODE;
struct THREAD_NODE;
struct INFO_NODE;
struct TRACE_NODE;
struct FLOW_NODE;
struct ListedNodes;
struct ADDR_INFO;

extern Archive  gArchive;

typedef enum {
    LOG_EMPTY_METHOD_ENTER_EXIT = 3,
    LOG_INFO_ENTER_FIRST,
    LOG_INFO_EXIT_LAST,
} LOG_INFO_TYPE;

typedef enum { LOG_TYPE_ENTER, LOG_TYPE_EXIT, LOG_TYPE_TRACE } ROW_LOG_TYPE;

#define LOG_FLAG_NEW_LINE 1
#define LOG_FLAG_JAVA 2

#pragma pack(push,1)
enum THREAD_NODE_CHILD { ROOT_CHILD, LATEST_CHILD };
enum LOG_DATA_TYPE { ROOT_DATA_TYPE, APP_DATA_TYPE, THREAD_DATA_TYPE, FLOW_DATA_TYPE, TRACE_DATA_TYPE };
enum LIST_COL { ICON_COL, LINE_NN_COL, NN_COL, APP_COLL, THREAD_COL, TIME_COL, FUNC_COL, CALL_LINE_COL, LOG_COL, MAX_COL };

struct ADDR_INFO
{
    DWORD addr;
    int   line;
    char* src;
    ADDR_INFO* pPrev;
};

struct LOG_NODE
{
    THREAD_NODE* threadNode;
    LOG_NODE* parent;
    LOG_NODE* firstChild;
    LOG_NODE* lastChild;
    LOG_NODE* prevSibling;
    LOG_NODE* nextSibling;
    struct {
        WORD hasNewLine : 1;
        WORD hiden : 1;
        WORD hasCheckBox : 1;
        WORD checked : 1;
        WORD selected : 1;
        WORD expanded : 1;
        WORD hasNodeBox : 1;
        WORD pathExpanded : 1;
    };
	BYTE bookmark;
	BYTE nextChankCounter;
    int cExpanded;
    int posInTree;
    int lineSearchPos;
    LOG_NODE* nextChankMarker;
    LOG_NODE* nextChank;

    LOG_DATA_TYPE data_type;
    DWORD lengthCalculated;
    DWORD childCount;
    bool isRoot() { return data_type == ROOT_DATA_TYPE; }
    bool isApp() { return data_type == APP_DATA_TYPE; }
    bool isThread() { return data_type == THREAD_DATA_TYPE; }
    bool isFlow() { return data_type == FLOW_DATA_TYPE; }
    bool isTrace() { return data_type == TRACE_DATA_TYPE; }
    bool isInfo() { return isFlow() || isTrace(); }
    bool isJava();
    bool CanShowInIDE();

    void CalcLines();
    int GetExpandCount() { return expanded ? cExpanded : 0; }
    void CollapseExpand(BOOL expand);
    int GetPosInTree() { return posInTree; }
    void CollapseExpandAll(bool expand);

    void add_child(LOG_NODE* pNode)
    {
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
        pNode->prevSibling = lastChild;
        pNode->parent = this;
        lastChild = pNode;
        LOG_NODE* p = pNode->parent;
        while (p)
        {
            p->childCount++;
            p = p->parent;
        }
    }

    bool isTreeNode() { return parent && !isTrace(); }
    bool isParent(LOG_NODE* pNode) { LOG_NODE* p = parent; while (p != pNode && p) p = p->parent; return p != NULL; }
    FLOW_NODE* getPeer();
    bool isSynchronized(LOG_NODE* pSyncNode);
    int getNN();
    LONG getTimeSec();
    LONG getTimeMSec();
    int   getPid();
	int   getThreadNN();
    CHAR* getTreeText(int* cBuf = NULL, bool extened = true);
    CHAR* getListText(int* cBuf, LIST_COL col, int iItem = 0);
    int getTreeImage();
    FLOW_NODE* getSyncNode();
    APP_NODE* getApp() { LOG_NODE* p = this; while (p && !p->isApp()) p = p->parent; return (APP_NODE*)p; }
    int getTraceText(char* pBuf, int max_cb_trace);
};

struct ROOT_NODE : LOG_NODE
{
};

struct APP_NODE : LOG_NODE
{
    int pid;
	int threadCount;
    DWORD lastNN;
    DWORD lost;
    char ip_address[66];
    int cb_app_name;
    int cb_short_app_name_offset;
    char appName[1];
};

struct THREAD_NODE : LOG_NODE
{
    APP_NODE* pAppNode;
    FLOW_NODE* curentFlow;
    TRACE_NODE* latestTrace;
    int emptLineColor;
    int tid;
    int threadNN;
    char COLOR_BUF[10];
    int  cb_color_buf;

    void add_thread_child(FLOW_NODE* pNode, THREAD_NODE_CHILD type)
    {
        LOG_NODE* pParent = NULL;
        if (type == ROOT_CHILD)
            pParent = this;
        else
            pParent = (LOG_NODE*)curentFlow;
        ATLASSERT(pParent != NULL);
        pParent->add_child((LOG_NODE*)pNode);
        curentFlow = pNode;
    }
    bool isHiden() { return hiden || pAppNode->hiden; }
};

struct INFO_NODE : LOG_NODE
{
    int  nn;
    WORD log_type;
	WORD log_flags;
    WORD cb_fn_name;
    WORD cb_short_fn_name_offset;
    WORD cb_module_name;
    union {
        WORD cb_trace;
        WORD cb_java_call_site; // for java we keep here caller class:method
    };
    int call_line;
    DWORD sec;
    DWORD msec;
    bool isEnter() { return log_type == LOG_TYPE_ENTER; }
    bool isTrace() { return log_type == LOG_TYPE_TRACE; }
    char* fnName();
    char* moduleName();
    int moduleNameLength();
    char* JavaCallSite() { return fnName() + cb_fn_name + cb_module_name; }
    char* shortFnName();
    int   callLine(bool resolve);

    //int cb_actual_module_name;
    //int cb_short_module_name_offset;
    //
    //char modulePath[MAX_PATH + 1];
    //char moduleName[1];

};

struct FLOW_NODE : INFO_NODE
{
    friend class AddrInfo;

    FLOW_NODE* peer;
    DWORD this_fn;
    DWORD call_site;
    int fn_line;
private:
    ADDR_INFO *p_func_info;
    ADDR_INFO *p_call_info;
public:
    ADDR_INFO *getFuncInfo(bool resolve);
    ADDR_INFO *getCallInfo(bool resolve);
    bool isOpenEnter() { return isEnter() && peer == 0; }
    void addToTree();
    char* getCallSrc(bool fullPath, bool resolve);
};

struct TRACE_CHANK
{
    TRACE_CHANK* next_chank;
    int len;
    char trace[1];
};

struct TRACE_NODE : INFO_NODE
{
    BYTE color;
    TRACE_CHANK* getFirestChank() { return (TRACE_CHANK*)(fnName() + cb_fn_name + cb_module_name); }
    TRACE_CHANK* getLastChank() { TRACE_CHANK* p = getFirestChank(); while (p->next_chank) p = p->next_chank; return p; }
    bool IsInContext();
    int getCallLine(bool bCallSiteInContext, bool resolve);
};

#pragma pack(pop)

