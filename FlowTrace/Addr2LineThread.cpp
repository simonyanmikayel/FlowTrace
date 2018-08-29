#include "stdafx.h"
#include "Addr2LineThread.h"
#include "addr2line.h"
#include "Archive.h"
#include "Helpers.h"
#include "Settings.h"

extern HWND       hwndMain;

Addr2LineThread::Addr2LineThread()
{
    m_pNode = NULL;
    m_hParseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hResolveEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}
 
void Addr2LineThread::Terminate()
{
    SetEvent(m_hParseEvent);
    CloseHandle(m_hParseEvent);
    CloseHandle(m_hResolveEvent);
}

void Addr2LineThread::ResolveAsync(LOG_NODE* pNode)
{
    if (pNode)
    {
        m_pNode = pNode;
        SetEvent(m_hResolveEvent);
    }
    else
    {
        SetEvent(m_hParseEvent);
    }
}

void Addr2LineThread::Resolve(LOG_NODE* pSelectedNode, bool loop)
{
    _Resolve(pSelectedNode, true, loop);
    _Resolve(pSelectedNode, false, loop);
}

void Addr2LineThread::_Resolve(LOG_NODE* pSelectedNode, bool bNested, bool loop)
{
    LOG_NODE* pNode = pSelectedNode;
    if (!pNode)
        return;
    if (pNode->isTrace())
        pNode = pNode->getSyncNode();
    if (pNode && pNode->parent == NULL)
        pNode = pNode->getPeer();
    if (!pNode || !pNode->isFlow())
        return;
    THREAD_NODE* threadNode = pNode->threadNode;
    if (threadNode->cb_addr_info == INFINITE || threadNode->cb_addr_info == 0 || threadNode->p_addr_info == NULL)
        return;
    bool firstNodeResolved = false;
    while (pNode && pNode->isFlow() && IsWorking())
    {
        FLOW_NODE* flowNode = (FLOW_NODE*)pNode;
        if (flowNode->this_fn == 0xC6D10)
            int iiiii = 0;
        if (flowNode->p_call_addr_info == 0)
        {

            DWORD call_addr = flowNode->call_site;
            DWORD func_addr = flowNode->this_fn;
            const int max_delta = 128;
            DWORD call_addr_delta = max_delta;
            DWORD func_addr_delta = max_delta;
            ADDR_INFO *p_addr_info = threadNode->p_addr_info;
            flowNode->p_call_addr_info = p_addr_info; //initial bad value
            flowNode->p_call_addr_info = p_addr_info;
            char* fn = flowNode->fnName();
            while (p_addr_info && IsWorking())
            {
                if (call_addr >= p_addr_info->addr && (call_addr - p_addr_info->addr) < call_addr_delta)
                {
                    call_addr_delta = call_addr - p_addr_info->addr;
                    flowNode->p_call_addr_info = p_addr_info;
                }
                else if (call_addr < p_addr_info->addr && (p_addr_info->addr - call_addr) < call_addr_delta)
                {
                    call_addr_delta = p_addr_info->addr - call_addr;
                    flowNode->p_call_addr_info = p_addr_info;
                }
                 
                if (func_addr >= p_addr_info->addr && (func_addr - p_addr_info->addr) < func_addr_delta)
                {
                    func_addr_delta = func_addr - p_addr_info->addr;
                    flowNode->p_func_addr_info = p_addr_info;
                }
                else if (func_addr < p_addr_info->addr && (p_addr_info->addr - func_addr) < func_addr_delta)
                {
                    func_addr_delta = p_addr_info->addr - func_addr;
                    flowNode->p_func_addr_info = p_addr_info;
                }

                p_addr_info = p_addr_info->pPrev;
            }

            if (call_addr_delta == max_delta)
                flowNode->p_call_addr_info = threadNode->p_addr_info;
            if (func_addr_delta == max_delta)
                flowNode->p_func_addr_info = threadNode->p_addr_info;
        }


        if (!loop)
            break;

        if (bNested)
        {
            if (!firstNodeResolved)
            {
                firstNodeResolved = true;
                pNode = pNode->firstChild;
            }
            else
                pNode = pNode->nextSibling;
        }
        else
            pNode = pNode->parent;
    }
}

void Addr2LineThread::Work(LPVOID pWorkParam)
{
    HANDLE handles[2] = { m_hParseEvent, m_hResolveEvent };
    DWORD obj;
    while (TRUE)
    {
        obj = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
        if (!IsWorking())
            break;
        if (obj == WAIT_OBJECT_0)
        {
            APP_NODE* appNode = (APP_NODE*)gArchive.getRootNode()->lastChild;
            while (appNode && IsWorking())
            {
                THREAD_NODE* threadNode = (THREAD_NODE*)appNode->lastChild;
                while (threadNode && IsWorking())
                {
                    if (threadNode->cb_addr_info == INFINITE)
                        ReadAdresses(threadNode);
                    threadNode = (THREAD_NODE*)threadNode->prevSibling;
                }
                appNode = (APP_NODE*)appNode->prevSibling;
            }
        }
        else if (obj == WAIT_OBJECT_0 + 1)
        {
            if (m_pNode)
            {
                LOG_NODE* pNode = m_pNode;
                LOG_NODE* pSelectedNode = m_pNode;
                m_pNode = 0;
                Resolve(pSelectedNode, true);
                if (IsWorking())
                    ::PostMessage(hwndMain, WM_UPDATE_BACK_TRACE, 0, (LPARAM)pSelectedNode);
            }
        }
        else
        {
            break;
        }
    }
}

void Addr2LineThread::CkeckAdresses(THREAD_NODE* curThreadNode)
{
    if (curThreadNode->cb_addr_info != INFINITE)
        return;

    APP_NODE* appNode = curThreadNode->pAppNode;
    THREAD_NODE* threadNode = (THREAD_NODE*)appNode->lastChild;
    while (threadNode && IsWorking())
    {
        if (curThreadNode != threadNode && threadNode->cb_addr_info != INFINITE && threadNode->cb_addr_info != 0)
        {
            if (0 == strcmp(threadNode->modulePath, curThreadNode->modulePath))
            {
                curThreadNode->cb_addr_info = threadNode->cb_addr_info;
                curThreadNode->p_addr_info = threadNode->p_addr_info;
                break;
            }
        }
        threadNode = (THREAD_NODE*)threadNode->prevSibling;
    }
    if (!IsWorking())
        curThreadNode->cb_addr_info = 0;
}

struct WORK_CTXT {
    Addr2LineThread* This;
    DWORD maxAddr;
    int total;
    ADDR_INFO* last_info;
    THREAD_NODE* threadNode;
};
static WORK_CTXT workCtxt;

static int line_info(
    unsigned long address,
    unsigned char op_index,
    char *src,
    unsigned int line,
    unsigned int column,
    unsigned int discriminator,
    int end_sequence)
{
    if (!workCtxt.This->IsWorking())
        return 0;

    int cbSrc = (int)strlen(src);
    ADDR_INFO* p_addr_info = NULL;
    if (workCtxt.last_info && 0 == strcmp(workCtxt.last_info->src, src))
    {
        if (p_addr_info = (ADDR_INFO*)gArchive.Alloc(sizeof(ADDR_INFO)))
            p_addr_info->src = workCtxt.last_info->src;
    }
    else
    {
        if (p_addr_info = (ADDR_INFO*)gArchive.Alloc(sizeof(ADDR_INFO) + cbSrc + 1))
        {
            p_addr_info->src = ((char*)p_addr_info) + sizeof(ADDR_INFO);
            memcpy(p_addr_info->src, src, cbSrc + 1);
        }
    }
    if (p_addr_info == NULL)
        return 0;

    workCtxt.maxAddr = max(workCtxt.maxAddr, address);
    p_addr_info->addr = address;
    p_addr_info->line = line;
    p_addr_info->pPrev = workCtxt.last_info;
    workCtxt.last_info = p_addr_info;
    workCtxt.threadNode->p_addr_info = p_addr_info;
    workCtxt.total++;
    return 1;
}

void Addr2LineThread::ReadAdresses(THREAD_NODE* threadNode)
{
    if (threadNode->cb_addr_info != INFINITE)
    {
        return;
    }
    if (threadNode->cb_module_name == 0)
    {
        threadNode->cb_addr_info = 0;
        return;
    }

    CHAR* szModules = gSettings.GetModules();
    CHAR* modulePath = strstr(szModules, threadNode->moduleName);
    while (modulePath && modulePath[threadNode->cb_module_name] != '\n')
        modulePath = strstr(modulePath+1, threadNode->moduleName);
    if (!modulePath || modulePath[threadNode->cb_module_name] != '\n')
    {
        threadNode->cb_addr_info = 0;
        return;
    }
    modulePath[threadNode->cb_module_name] = 0;
    char* p = strrchr(szModules, '\n');
    if (p)
        modulePath = p + 1;
    else
        modulePath = szModules;

    strncpy_s(threadNode->modulePath, modulePath, MAX_PATH);
    threadNode->modulePath[MAX_PATH] = 0;

    CkeckAdresses(threadNode);
    if (threadNode->cb_addr_info != INFINITE)
    {
        return;
    }
     
    ZeroMemory(&workCtxt, sizeof(workCtxt));
    workCtxt.This = this;
    workCtxt.threadNode = threadNode;

    enum_file_addresses(threadNode->modulePath, line_info);

    // add a fake info for unknown
    if (threadNode->p_addr_info)
    {
        ADDR_INFO *p = (ADDR_INFO*)gArchive.Alloc(sizeof(ADDR_INFO));
        if (p)
        {
            p->addr = workCtxt.maxAddr + 1000;
            p->line = -1;
            p->src = "?";
            p->pPrev = NULL;
            p->pPrev = threadNode->p_addr_info;
            threadNode->p_addr_info = p;
            workCtxt.total++;
        }
    }
    threadNode->cb_addr_info = workCtxt.total;
}

