#include "stdafx.h"
#include "Archive.h"
#include "Settings.h"
#include "Helpers.h"
#include "AddrInfo.h"

bool LOG_NODE::isSynchronized(LOG_NODE* pSyncNode)
{
    FLOW_NODE* pPeer = getPeer();
    return pSyncNode && 
		(this == pSyncNode || pPeer == pSyncNode || isParent(pSyncNode) || (pPeer && pPeer->isParent(pSyncNode)));
}

FLOW_NODE* LOG_NODE::getPeer()
{
    if (isFlow())
        return ((FLOW_NODE*)this)->peer;
    else
        return nullptr;
}

char* FLOW_NODE::getCallSrc(bool fullPath, bool resolve)
{
    char* src = "";
    if (!isJava()) 
    {
        ADDR_INFO *p_call_addr_info = getCallInfo(resolve);
        if (p_call_addr_info != 0)
        {
            src = p_call_addr_info->src;
            if (!fullPath)
            {
                char* name = strrchr(src, '/');
                if (name)
                    src = name + 1;
            }
        }
    }
    return src;
}
int INFO_NODE::callLine(bool resolve)
{
    int line = 0;
    if (isJava() || isTrace())
    {
        line = call_line;
    }
    else
    {
        FLOW_NODE* This = (FLOW_NODE*)this;
        ADDR_INFO *p_call_addr_info = This->getCallInfo(resolve);
        if (p_call_addr_info != 0)
        {
            line = p_call_addr_info->line;
        }
    }
    return line;
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

char* INFO_NODE::fnName()
{ 
    char* name = ((char*)(this)) + (isFlow() ? sizeof(FLOW_NODE) : sizeof(TRACE_NODE));
    if (cb_short_fn_name_offset == 0xFFFF)
    {
        normilizeFnName(name);
    }
    return name;
}

char* INFO_NODE::moduleName()
{
    if (cb_module_name)
        return fnName() + cb_fn_name;
    else
        return threadNode->pAppNode->appName;
}

int INFO_NODE::moduleNameLength()
{
    if (cb_module_name)
        return cb_module_name;
    else
        return threadNode->pAppNode->cb_app_name;
}

void INFO_NODE::normilizeFnName(char* name)
{
    if (cb_short_fn_name_offset == 0xFFFF)
    {
        cb_short_fn_name_offset = 0;
        if (log_flags & LOG_FLAG_JAVA)
        {
            int dot1 = 0, dot2 = 0;
            for (int i = 0; i < cb_fn_name; i++)
            {
                if (name[i] == '/')
                    name[i] = '.';
                if (name[i] == '.')
                {
                    if (dot1 == dot2)
                        dot2 = i;
                    else {
                        dot1 = dot2;
                        dot2 = i;
                    }
                }
            }
            if (dot1 < dot2)
                cb_short_fn_name_offset = dot1 + 1;
        }
    }
}

char* INFO_NODE::shortFnName()
{
    char* name = fnName();
    if (cb_short_fn_name_offset == 0xFFFF)
    {
        normilizeFnName(name);
    }
    return name + cb_short_fn_name_offset;
}

void FLOW_NODE::addToTree()
{
    if (threadNode->curentFlow == NULL)
    {
		threadNode->add_thread_child(this, ROOT_CHILD);
    }
    else
    {
        FLOW_NODE* lastFlowNode = threadNode->curentFlow;
        if (isEnter())
        {
            if (lastFlowNode->peer || !lastFlowNode->isEnter())
            {
				threadNode->add_thread_child(this, ROOT_CHILD);
            }
            else
            {
				threadNode->add_thread_child(this, LATEST_CHILD);
            }
        }
        else
        {
			while ((void*)lastFlowNode != (void*)threadNode)
			{
				if (lastFlowNode->peer == NULL && lastFlowNode->isEnter() && lastFlowNode->this_fn == this_fn)// && lastFlowNode->call_site == call_site)
				{
					lastFlowNode->peer = this;
					peer = lastFlowNode;
                    if (lastFlowNode->fn_line == 0)
                        lastFlowNode->fn_line = this->fn_line;
					if (lastFlowNode->parent != threadNode) {
						threadNode->curentFlow = (FLOW_NODE*)(lastFlowNode->parent);
					}
					break;
				}
				lastFlowNode = (FLOW_NODE*)(lastFlowNode->parent);
			}
            if ((void*)lastFlowNode == (void*)threadNode)
            {
                if (lastFlowNode->isEnter())
                {
					threadNode->add_thread_child(this, ROOT_CHILD);
                }
                else
                {
					threadNode->add_thread_child(this, LATEST_CHILD);
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
    else if (isThread())
    {
        return 2;//IDI_ICON_TREE_THREAD
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

FLOW_NODE* LOG_NODE::getSyncNode()
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
    if (pNode && pNode->isFlow() || pNode->isThread())
        return (FLOW_NODE*)pNode;
    else
        return NULL;
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
        memcpy(pBuf, This->appName, cb);
        pBuf[cb] = 0;
        if (This->lost)
        {
            cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT(" (Lost: %d)"), This->lost);
        }
        if (gSettings.GetShowAppIp() && This->ip_address[0])
        {
            cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT(" (%s)"), This->ip_address);
        }
        if (gSettings.GetColPID())
        {
            cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT("[%d]"), This->pid);
        }
    }
    else if (isThread())
    {
        if (gSettings.GetColPID() && gSettings.GetColTID())
            cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT("[%d-%d-%d]"), getThreadNN(), getPid(), getTid());
        else if (gSettings.GetColPID())
            cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT("[%d-%d]"), getThreadNN(), getPid());
        else if (gSettings.GetColTID())
			cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT("[%d-%d]"), getThreadNN(), getTid());
		else
			cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT("[%d]"), getThreadNN());
        if (gSettings.GetShowChildCount())
            cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT("[%d]"), childCount);

	}
    else if (isFlow())
    {
        FLOW_NODE* This = (FLOW_NODE*)this;
        int cb_fn_name = This->cb_fn_name;
        char* name = This->shortFnName();
        memcpy(pBuf + cb, name, This->cb_fn_name - This->cb_short_fn_name_offset);
        cb += This->cb_fn_name - This->cb_short_fn_name_offset;
        pBuf[cb] = 0;
        if (extened)
        {
            if (gSettings.GetColNN() && NN)
                cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT(" (%d)"), NN); //gArchive.index(this) NN
            if (gSettings.GetShowElapsedTime() && This->getPeer())
            {
                _int64 sec1 = This->getTimeSec();
                _int64 msec1 = This->getTimeMSec();
                _int64 sec2 = (This->getPeer())->getTimeSec();
                _int64 msec2 = (This->getPeer())->getTimeMSec();
                cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT(" (%lldms)"), (sec2 - sec1) * 1000 + (msec2 - msec1));
            }
            if (gSettings.GetColCallAddr())
            {
                DWORD p = This->call_site;
                cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT(" (%X)"), p);
            }
            if (gSettings.GetFnCallLine())
            {
                ADDR_INFO *p_call_addr_info = This->getCallInfo(false);
                if (p_call_addr_info != 0)
                    cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT(" (%d)"), p_call_addr_info->line);
            }
            if (gSettings.GetShowChildCount())
            {
                cb += _sntprintf_s(pBuf + cb, cMaxBuf - cb, cMaxBuf - cb, TEXT("[%d]"), childCount);
            }
        }
        pBuf[cb] = 0;
    }
    else
    {
        ATLASSERT(FALSE);
    }
	if (cb > cMaxBuf)
		cb = cMaxBuf;
	pBuf[cb] = 0;
    if (cBuf)
        *cBuf = cb;
    return ret;
}


CHAR* LOG_NODE::getListText(int *cBuf, LIST_COL col, int iItem)
{
    const int MAX_BUF_LEN = 2 * MAX_LOG_LEN - 1;
    static CHAR pBuf[MAX_BUF_LEN + 1];
    int& cb = *cBuf;
    CHAR* ret = pBuf;

    cb = 0;
    pBuf[0] = 0;

    if (col == NN_COL)
    {
		int NN = getNN(); // gArchive.index(this); // getNN();
#ifdef _NN_TEST
		int prev_nn = isInfo() ? ((INFO_NODE*)this)->prev_nn : 0;
		int  pack_nn = isInfo() ? ((INFO_NODE*)this)->pack_nn : 0;
		int  retry_nn = isInfo() ? ((INFO_NODE*)this)->retry_nn : 0;
		int  buff_nn = isInfo() ? ((INFO_NODE*)this)->buff_nn : 0;
		if (NN == prev_nn + 1)
			cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("   %d [prev_nn=%d pack_nn=%d retry_nn=%d buff_nn=%d]"), NN, prev_nn, pack_nn, retry_nn, buff_nn);
		else
			cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("?> %d [prev_nn=%d pack_nn=%d retry_nn=%d buff_nn=%d]"), NN, prev_nn, pack_nn, retry_nn, buff_nn);
#else
		if (NN)
			cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("%d"), NN);
#endif
		int prev_NN = prevSibling ? prevSibling->getNN() : 0;
	}
    else if (col == APP_COLL)
    {
        if (isInfo())
        {
            INFO_NODE* This = (INFO_NODE*)this;
            if (This->cb_module_name)
            {
                char* name = This->moduleName();
                memcpy(pBuf + cb, This->moduleName(), This->cb_module_name);
                cb += This->cb_module_name;
            }
			else
			{
				APP_NODE* pAppNode = This->threadNode->pAppNode;
				memcpy(pBuf, pAppNode->appName + pAppNode->cb_short_app_name_offset, pAppNode->cb_app_name - pAppNode->cb_short_app_name_offset);
				cb += pAppNode->cb_app_name - pAppNode->cb_short_app_name_offset;
			}
            pBuf[cb] = 0;
        }
    }
    else if (col == THREAD_COL)
    {
        if (gSettings.GetColPID() && gSettings.GetColTID())
            cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("[%d-%d-%d]"), getThreadNN(), getPid(), getTid());
        else if (gSettings.GetColPID())
            cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("[%d-%d]"), getThreadNN(), getPid());
        else if (gSettings.GetColTID())
            cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("[%d-%d]"), getThreadNN(), getTid());
		else
			cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("[%d]"), getThreadNN());
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
        if (isInfo())
        {
            INFO_NODE* This = (INFO_NODE*)this;
            char* name = This->shortFnName();
            cb += This->cb_fn_name - This->cb_short_fn_name_offset;
            memcpy(pBuf, name, This->cb_fn_name - This->cb_short_fn_name_offset);
            pBuf[cb] = 0;
        }
    }
    else if (col == CALL_LINE_COL)
    {
        if (isInfo())
        {
            INFO_NODE* This = (INFO_NODE*)this;
            int n = This->callLine(false);
            if (n)
                cb += _sntprintf_s(pBuf, MAX_BUF_LEN, MAX_BUF_LEN, TEXT("%d"), n);
        }
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

	if (cb > MAX_BUF_LEN)
		cb = MAX_BUF_LEN;
	pBuf[cb] = 0;
    return ret;
}

int LOG_NODE::getNN()
{
    return isInfo() ? ((INFO_NODE*)this)->nn : 0;
}
LONG LOG_NODE::getTimeSec()
{
    return isInfo() ? (((INFO_NODE*)this)->sec) : 0LL;
}
LONG LOG_NODE::getTimeMSec()
{
    return isInfo() ? (((INFO_NODE*)this)->msec) : 0LL;
}
int LOG_NODE::getPid()
{
    return  isThread() ? ((THREAD_NODE*)this)->pAppNode->pid : (isInfo() ? threadNode->pAppNode->pid : 0);
}
int LOG_NODE::getTid()
{ 
	return  isThread() ? ((THREAD_NODE*)this)->tid : (isInfo() ? threadNode->tid : 0);
}
int LOG_NODE::getThreadNN()
{
	return  isThread() ? ((THREAD_NODE*)this)->threadNN : (isInfo() ? threadNode->threadNN : 0);
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
        pNode->posInTree = ++l;
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

//return true if check changed
bool LOG_NODE::CheckAll(bool check)
{
	LOG_NODE* pNode = firstChild;
	bool checkChanged = false;
	if (hasCheckBox)
	{
		if (checked != check)
			checkChanged = true;
		checked = check;
		hiden = !check;
	}
	while (pNode && pNode->hasCheckBox)
	{
		if (pNode->checked != check)
			checkChanged = true;
		pNode->checked = check;
		pNode->hiden = !check;
		pNode = pNode->nextSibling;
	}
	return checkChanged;
}

bool LOG_NODE::CanShowInIDE()
{
	if (!isInfo())
		return false;
	else if (isJava())
		return gSettings.CanShowInAndroidStudio();
	else
		return gSettings.CanShowInEclipse();
}

int TRACE_NODE::getCallLine(bool bCallSiteInContext, bool resolve)
{
    int line = 0;
    if (bCallSiteInContext)
    {
        LOG_NODE* pContextNode = getSyncNode();
        bool inContext = pContextNode == gSyncronizedNode;
        if (inContext)
        {
            line = call_line;
        }
        else
        {
            while (pContextNode && pContextNode->parent != gSyncronizedNode)
                pContextNode = pContextNode->parent;
            if (pContextNode && pContextNode->isFlow())
            {
                FLOW_NODE* This = (FLOW_NODE*)pContextNode;
                ADDR_INFO *p_call_addr_info = This->getCallInfo(resolve);
                if (p_call_addr_info != 0)
                    line = p_call_addr_info->line;
            }
        }
    }
    else
    {
        line = call_line;
    }
    return line;
}

ADDR_INFO * FLOW_NODE::getFuncInfo(bool resolve)
{
    if (p_func_info == pInvalidAddrInfo)
        return 0;
    if (p_func_info == NULL && resolve)
        AddrInfo::Resolve(this);
    return p_func_info;
}
ADDR_INFO * FLOW_NODE::getCallInfo(bool resolve)
{
    if (p_call_info == pInvalidAddrInfo)
        return 0;
    if (p_call_info == NULL && resolve)
        AddrInfo::Resolve(this);
    return p_call_info;
}

