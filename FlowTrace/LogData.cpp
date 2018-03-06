#include "stdafx.h"
#include "Archive.h"
#include "Settings.h"

bool LOG_NODE::isSynchronized(LOG_NODE* pSyncNode)
{
    FLOW_NODE* pPeer = getPeer();
    return pSyncNode && (this == pSyncNode || pPeer == pSyncNode || isParent(pSyncNode) || (pPeer && pPeer->isParent(pSyncNode)));
}

FLOW_NODE* LOG_NODE::getPeer()
{
    if (isFlow())
        return ((FLOW_NODE*)this)->peer;
    else
        return nullptr;
}

char* LOG_NODE::getFnName()
{
    if (isFlow())
    {
        return  ((FLOW_NODE*)this)->fnName();
    }
    else if (isTrace())
    {
        return  ((TRACE_NODE*)this)->fnName();
    }
    else
    {
        return "";
    }
}
char* LOG_NODE::getSrcName(bool fullPath)
{
    char* src = "";
    if (p_call_addr)
    {
        src = p_call_addr->src;
        if (!fullPath)
        {
            char* name = strrchr(src, '/');
            if (name)
                src = name + 1;
        }
    }
    return src;
}
int LOG_NODE::getTraceText(char* pBuf, int max_cb_trace)
{
    int cb = 0;
    int max_cb_trace_2 = max_cb_trace - 5; //space for elipces and zero terminator
    if (isTrace())
    {
        TRACE_CHANK* chank = ((TRACE_NODE*)this)->getFirestChank();
        bool truncated = false;
        while (chank && cb < max_cb_trace_2 && !truncated)
        {
            int cb_chank_trace = chank->len;
            if (max_cb_trace_2 - cb < chank->len)
            {
                truncated = true;
                cb_chank_trace = max_cb_trace_2 - cb;
            }
            memcpy(pBuf + cb, chank->trace, cb_chank_trace);
            cb += cb_chank_trace;
            pBuf[cb] = 0;
            chank = chank->next_chank;
        }
        if (truncated)
            cb += _sntprintf_s(pBuf + cb, max_cb_trace - cb, max_cb_trace - cb, "...");
    }
    return cb;
}

bool LOG_NODE::PendingToResolveAddr(bool bNested)
{
    bool pendingToResolveAddr = false;
    LOG_NODE* pNode = getSyncNode();
    if (pNode)
    {
        APP_NODE* appNode = pNode->getApp();
        if (pNode->isFlow() && gSettings.GetResolveAddr() && appNode)
        {
            pendingToResolveAddr = (appNode->cb_addr_info == INFINITE);
            if (!pendingToResolveAddr)
            {
                pendingToResolveAddr = (pNode->p_call_addr == NULL);
                if (!pendingToResolveAddr && bNested)
                {
                    pNode = pNode->firstChild;
                    while (!pendingToResolveAddr && pNode)
                    {
                        pendingToResolveAddr = (pNode->p_call_addr == NULL);
                        pNode = pNode->nextSibling;
                    }
                }
            }
        }
    }
    return pendingToResolveAddr;
}
int LOG_NODE::getFnNameSize()
{
    if (isFlow())
    {
        return  ((FLOW_NODE*)this)->cb_fn_name;
    }
    else if (isTrace())
    {
        return  ((TRACE_NODE*)this)->cb_fn_name;
    }
    else
    {
        return 0;
    }

}
void FLOW_NODE::addToTree()
{
    if (proc->curentFlow == NULL)
    {
        proc->add_proc_child(this, ROOT_CHILD);
    }
    else
    {
        FLOW_NODE* pLatestFlowData = proc->curentFlow;
        if (isEnter())
        {
            if (pLatestFlowData->peer || !pLatestFlowData->isEnter())
            {
                proc->add_proc_child(this, ROOT_CHILD);
            }
            else
            {
                proc->add_proc_child(this, LATEST_CHILD);
            }
        }
        else
        {
            if (pLatestFlowData->peer == NULL && pLatestFlowData->isEnter() && pLatestFlowData->this_fn == this_fn && pLatestFlowData->call_site == call_site)
            {
                pLatestFlowData->peer = this;
                peer = proc->curentFlow;
                if (proc->curentFlow->parent != proc) {
                    proc->curentFlow = (FLOW_NODE*)(proc->curentFlow->parent);
                }
            }
            else
            {
                if (pLatestFlowData->isEnter())
                {
                    proc->add_proc_child(this, ROOT_CHILD);
                }
                else
                {
                    proc->add_proc_child(this, LATEST_CHILD);
                }
            }
        }
    }
}

int LOG_NODE::getTreeImage()
{
    if (isRoot())
    {
        return 0;//IDI_ICON_TREE_ROOT
    }
    else if (isApp())
    {
        return 1;//IDI_ICON_TREE_APP
    }
    else if (isProc())
    {
        return 2;//IDI_ICON_TREE_PROC
    }
    else if (isFlow())
    {
        FLOW_NODE* This = (FLOW_NODE*)this;
        return This->peer ? 3 : (This->isEnter() ? 4 : 5);//IDI_ICON_TREE_PAIRED, IDI_ICON_TREE_ENTER, IDI_ICON_TREE_EXIT
    }
    else
    {
        ATLASSERT(FALSE);
        return 0;//IDI_ICON_TREE_ROOT
    }
}

LOG_NODE* LOG_NODE::getSyncNode()
{
    LOG_NODE* pNode = this;
    while (pNode && !pNode->isTreeNode() && (pNode->getPeer() || pNode->parent))
    {
        if (pNode->getPeer())
        {
            pNode = pNode->getPeer();
            break;
        }
        else
        {
            pNode = pNode->parent;
        }
    }
    if (pNode && !pNode->isTreeNode())
    {
        pNode = NULL;
    }
    return pNode;
}

CHAR* LOG_NODE::getTreeText(int* cBuf, bool extened)
{
    const int cMaxBuf = 255;
    static CHAR pBuf[cMaxBuf + 1];
    int cb = 0;
    CHAR* ret = pBuf;
    int NN = getNN();
#ifdef _DEBUG
    //cb += _sntprintf_s(pBuf + cb, cMaxBuf, cMaxBuf, TEXT("[%d %d %d]"), GetExpandCount(), line, lastChild ? lastChild->index : 0);
#endif
    if (this == gArchive.getRootNode())
    {
        cb += _sntprintf_s(pBuf + cb, cMaxBuf, cMaxBuf, TEXT("..."));
    }
    else if (isApp())
    {
        APP_NODE* This = (APP_NODE*)this;
        cb = This->cb_app_name;
        memcpy(pBuf, This->appName(), cb);
        pBuf[cb] = 0;
        if (This->lost)
        {
            cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT(" (Lost: %d)"), This->lost);
        }
        if (gSettings.GetShowAppIp() && This->ip_address[0])
        {
            cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT(" (%s)"), This->ip_address);
        }
        if (gSettings.GetShowAppTime())
        {
            struct tm timeinfo;
            DWORD sec = This->app_sec;
            DWORD msec = This->app_msec;
            time_t rawtime = sec;
            cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT(" ("));
            if (0 == localtime_s(&timeinfo, &rawtime))
            {
                cb += (int)strftime(pBuf + cb, cMaxBuf - cb, "%H:%M:%S", &timeinfo);
            }
            cb += (int)_sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT(".%03d)"), msec);
        }
    }
    else if (isProc())
    {
        cb += _sntprintf_s(pBuf + cb, cMaxBuf, cMaxBuf, TEXT("[%d]"), getPid());
    }
    else if (isFlow())
    {
        FLOW_NODE* This = (FLOW_NODE*)this;
        int cb_fn_name = This->cb_fn_name;
        memcpy(pBuf + cb, This->fnName(), cb_fn_name);
        cb += cb_fn_name;
        if (extened)
        {
            if (gSettings.GetColNN() && NN)
                cb += _sntprintf_s(pBuf + cb, cMaxBuf, cMaxBuf, TEXT(" (%d)"), NN); //gArchive.index(this) NN
            if (gSettings.GetShowElapsedTime() && This->getPeer())
            {
                _int64 sec1 = This->getTimeSec();
                _int64 msec1 = This->getTimeMSec();
                _int64 sec2 = (This->getPeer())->getTimeSec();
                _int64 msec2 = (This->getPeer())->getTimeMSec();
                cb += _sntprintf_s(pBuf + cb, cMaxBuf, cMaxBuf, TEXT(" (%lldms)"), (sec2 - sec1) * 1000 + (msec2 - msec1));
            }
        }
        if (gSettings.GetColCallAddr())
        {
            DWORD p = This->call_site;
            cb += _sntprintf_s(pBuf + cb, cMaxBuf, cMaxBuf, TEXT(" (%d)"), p);
        }
        if (gSettings.GetFnCallLine())
        {
            if (p_call_addr)
                cb += _sntprintf_s(pBuf + cb, cMaxBuf, cMaxBuf, TEXT(" (%d)"), p_call_addr->line);
        }
        pBuf[cb] = 0;
    }
    else
    {
        ATLASSERT(FALSE);
    }
    pBuf[cMaxBuf] = 0;
    if (cBuf)
        *cBuf = cb;
    return ret;
}


CHAR* LOG_NODE::getListText(int *cBuf, LIST_COL col, int iItem)
{
    const int MAX_BUF_LEN = 2 * MAX_TRCAE_LEN - 1;
    static CHAR pBuf[MAX_BUF_LEN + 1];
    int& cb = *cBuf;
    CHAR* ret = pBuf;

    cb = 0;
    pBuf[0] = 0;

    if (col == LINE_NN_COL)
    {
        cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("%d"), iItem);
    }
    else if (col == NN_COL)
    {
        int NN = getNN(); // gArchive.index(this); // getNN();
        if (NN)
            cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("%d"), NN);
    }
    else if (col == APP_COLL)
    {
        cb += proc->pAppNode->cb_app_name;
        memcpy(pBuf, proc->pAppNode->appName(), cb);
        pBuf[cb] = 0;
    }
    else if (col == PROC_COL)
    {
        cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("[%d]"), getPid());
    }
    else if (col == TIME_COL)
    {
        if (isInfo())
        {
            struct tm timeinfo;
            DWORD sec = getTimeSec();
            DWORD msec = getTimeMSec();
            time_t rawtime = sec;
            if (0 == localtime_s(&timeinfo, &rawtime))
                cb += (int)strftime(pBuf, MAX_BUF_LEN, "%H:%M:%S", &timeinfo);

            //struct tm * timeinfo2;
            //time_t rawtime2 = _time32(NULL);
            //timeinfo2 = localtime(&rawtime2);
            //cb += strftime(pBuf + cb, MAX_BUF_LEN, " %H.%M.%S", timeinfo2);
            //int i = 4294967290 / 10000000;
            //        6553500000
            cb += _sntprintf_s(pBuf + cb, MAX_BUF_LEN - cb, MAX_BUF_LEN - cb, TEXT(".%03d"), msec);
        }
    }
    else if (col == FUNC_COL)
    {
        if (isTrace())
        {
            TRACE_NODE* This = (TRACE_NODE*)this;
            cb += This->cb_fn_name;
            memcpy(pBuf, This->fnName(), cb);
            pBuf[cb] = 0;
        }
        else if (isFlow())
        {
            FLOW_NODE* This = (FLOW_NODE*)this;
            cb += This->cb_fn_name;
            memcpy(pBuf, This->fnName(), cb);
            pBuf[cb] = 0;
        }
    }
    else if (col == CALL_LINE_COL)
    {
        int n = getCallLine();
        if (n)
            cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("%d"), n);
    }
    else if (col == LOG_COL)
    {
        if (isFlow())
        {
            FLOW_NODE* This = (FLOW_NODE*)this;
            cb += This->cb_fn_name;
            memcpy(pBuf, This->fnName(), cb);
            pBuf[cb] = 0;
        }
        else if (isTrace())
        {
            cb = 0;
            TRACE_NODE* This = (TRACE_NODE*)this;
            TRACE_CHANK* pChank = This->getFirestChank();
            if (pChank->next_chank)
            {
                while (pChank) {
                    int c = pChank->len;
                    if ((c + cb) > MAX_BUF_LEN)
                    {
                        c -= (MAX_BUF_LEN - c - cb);
                        if (c <= 0)
                            break;
                    }

                    memcpy(pBuf + cb, pChank->trace, c);
                    cb += c;
                    pChank = pChank->next_chank;
                }
                pBuf[cb] = 0;
            }
            else
            {
                cb = pChank->len;
                ret = pChank->trace;
            }
            //ret = "  2";
            //cb = 3;
        }
    }

    pBuf[MAX_BUF_LEN] = 0;
    return ret;
}

int LOG_NODE::getNN()
{
    return isInfo() ? ((INFO_NODE*)this)->nn : 0;
}
LONG LOG_NODE::getTimeSec()
{
    return isInfo() ? (gSettings.GetUsePcTime() ? ((INFO_NODE*)this)->pc_sec : ((INFO_NODE*)this)->term_sec) : 0LL;
}
LONG LOG_NODE::getTimeMSec()
{
    return isInfo() ? (gSettings.GetUsePcTime() ? ((INFO_NODE*)this)->pc_msec : ((INFO_NODE*)this)->term_msec) : 0LL;
}
int LOG_NODE::getPid()
{ 
    return  isProc() ? ((PROC_NODE*)this)->threadNN : (isInfo() ? proc->threadNN : 0);
}
DWORD LOG_NODE::getCallAddr()
{
    return isFlow() ? ((FLOW_NODE*)this)->call_site : 0;
}
int LOG_NODE::getCallLine()
{
    return isInfo() ? ((INFO_NODE*)this)->call_line : 0;
}

void LOG_NODE::CollapseExpand(BOOL expand)
{
    expanded = expand;
    CalcLines();
}

void LOG_NODE::CalcLines()
{
    LOG_NODE* pNode = gArchive.getRootNode();
    int l = -1;
    //stdlog("CalcLines %d\n", GetTickCount());
    do
    {
        pNode->cExpanded = 0;
        pNode->pathExpanded = (!pNode->parent || (pNode->parent->pathExpanded && pNode->parent->expanded));
        if (pNode->parent)
            pNode->parent->cExpanded++;
        pNode->line = ++l;
        //stdlog("index = %d e = %d l = %d %s\n", pNode->index, pNode->cExpanded, pNode->line, pNode->getTreeText());

        if (pNode->firstChild && pNode->expanded)
        {
            pNode = pNode->firstChild;
        }
        else if (pNode->nextSibling)
        {
            pNode = pNode->nextSibling;
        }
        else
        {
            while (pNode->parent)
            {
                pNode = pNode->parent;
                //stdlog("\t index = %d e = %d l = %d %s\n", pNode->index, pNode->cExpanded, pNode->line, pNode->getTreeText());
                if (pNode->parent && pNode->cExpanded)
                {
                    pNode->parent->cExpanded += pNode->cExpanded;
                    //stdlog("\t\t index = %d e = %d l = %d %s\n", pNode->parent->index, pNode->parent->cExpanded, pNode->parent->line, pNode->parent->getTreeText());
                }
                if (pNode == gArchive.getRootNode())
                    break;
                if (pNode->nextSibling)
                {
                    pNode = pNode->nextSibling;
                    break;
                }
            }
        }
    } while (gArchive.getRootNode() != pNode);
    //stdlog("CalcLines %d\n", GetTickCount());
}

void LOG_NODE::CollapseExpandAll(bool expand)
{
    //stdlog("CollapseExpandAll %d\n", GetTickCount());
    LOG_NODE* pNode = this;
    LOG_NODE* pNode0 = pNode;
    do
    {
        pNode->expanded = (pNode->lastChild && expand) ? 1 : 0;

        if (pNode->firstChild)
        {
            pNode = pNode->firstChild;
        }
        else if (pNode->nextSibling)
        {
            pNode = pNode->nextSibling;
        }
        else
        {
            while (pNode->parent)
            {
                pNode = pNode->parent;
                if (pNode == pNode0)
                    break;
                if (pNode->nextSibling)
                {
                    pNode = pNode->nextSibling;
                    break;
                }
            }
        }
    } while (pNode0 != pNode);
    //stdlog("CollapseExpandAll %d\n", GetTickCount());
    CalcLines();
}


