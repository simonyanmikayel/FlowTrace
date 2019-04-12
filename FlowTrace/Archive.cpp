#include "stdafx.h"
#include "Archive.h"
#include "Helpers.h"
#include "AddrInfo.h"
#include "Settings.h"

Archive    gArchive;
DWORD Archive::archiveNumber = 0;

#ifdef _NN_TEST
int  g_prev_nn;
int  g_pack_nn;
int  g_retry_nn;
int  g_buff_nn;
#endif

Archive::Archive()
{
    ZeroMemory(this, sizeof(*this));
    m_pTraceBuf = new MemBuf(MAX_BUF_SIZE, 256 * 2 * 1024 * 1024);
    m_listedNodes = new ListedNodes();
    clearArchive();
}

Archive::~Archive()
{
    //we intentionaly do not free memory for this single instance application quick exit
    //delete m_pMemBuf;
}

void Archive::onPaused()
{
	APP_NODE* pApp = (APP_NODE*)gArchive.getRootNode()->lastChild;
	while (pApp)
	{
		pApp->lastRecNN = INFINITE;
		pApp->lastPackNN = -1;
		pApp = (APP_NODE*)pApp->prevSibling;
	}
}

void Archive::clearArchive(bool closing)
{
    archiveNumber++;
	bookmarkNumber = 0;
	m_lost = 0;
    curApp = 0;
    curThread = 0;
    gArchive.getSNAPSHOT().clear();

    delete m_pNodes;
    m_pNodes = nullptr;
    m_rootNode = nullptr;
    m_pTraceBuf->Free();
    m_listedNodes->Free();
    AddrInfo::Reset();
    if (!closing)
    {
        m_pNodes = new PtrArray<LOG_NODE>(m_pTraceBuf);
        m_rootNode = (ROOT_NODE*)m_pNodes->Add(sizeof(ROOT_NODE), true);
        m_rootNode->data_type = ROOT_DATA_TYPE;
        ATLASSERT(m_pNodes && m_rootNode);
    }
}

size_t Archive::UsedMemory() 
{
    return m_pTraceBuf->UsedMemory() + m_listedNodes->UsedMemory(); 
}

APP_NODE* Archive::addApp(ROW_LOG_REC* p, sockaddr_in *p_si_other)
{
    APP_NODE* pNode = (APP_NODE*)m_pNodes->Add(sizeof(APP_NODE), true);
    if (!pNode)
        return nullptr;
	const char* appName = p->appName();
	WORD cb_app_name = p->cb_app_name;
	if (cb_app_name == 0) {
		cb_app_name = 1;
		appName = "?";
	}
    pNode->data_type = APP_DATA_TYPE;
    pNode->pid = p->pid;
    pNode->cb_app_name = std::min(cb_app_name, (WORD)MAX_APP_NAME);
	pNode->lastRecNN = INFINITE;
	pNode->lastPackNN = -1;
	
    memcpy(pNode->appName, appName, pNode->cb_app_name);
    pNode->appName[pNode->cb_app_name] = 0;
    char* dotdot = strrchr(pNode->appName, ':');
    if (dotdot) {
        *dotdot = 0;
        char* dot = strrchr(pNode->appName, '.');
        if (dot) {
            pNode->cb_short_app_name_offset = int(dot - pNode->appName + 1);
        }
        *dotdot = ':'; //restore
    }    

    if (p_si_other)
    {
        char* str_ip = inet_ntoa(p_si_other->sin_addr);
        if (str_ip)
        {
            strncpy_s(pNode->ip_address, sizeof(pNode->ip_address), str_ip, sizeof(pNode->ip_address) - 1);
            pNode->ip_address[sizeof(pNode->ip_address) - 1] = 0;
        }
    }

    gArchive.getRootNode()->add_child(pNode);
    pNode->hasCheckBox = 1;
    pNode->checked = 1;
    return pNode;
}

THREAD_NODE* Archive::addThread(ROW_LOG_REC* p, APP_NODE* pAppNode)
{
    THREAD_NODE* pNode = (THREAD_NODE*)m_pNodes->Add(sizeof(THREAD_NODE) + p->cbModuleName() + 1, true);
    if (!pNode)
        return nullptr;

    pNode->threadNN = ++(pAppNode->threadCount);
    pNode->data_type = THREAD_DATA_TYPE;
    pNode->pAppNode = pAppNode;
    pNode->tid = p->tid;
    
    pAppNode->add_child(pNode);
    pNode->hasCheckBox = 1;
    pNode->checked = 1;

    return pNode;
}

bool Archive::setAppName(int pid, char* szName, int cbName)
{
	char* appName = szName;
	WORD cb_app_name = cbName;
	if (cbName == 1 && appName[0] == '?') {
		cb_app_name = 2;
		appName = "??";
	}
	APP_NODE* app = (APP_NODE*)gArchive.getRootNode()->lastChild;
	while (app)
	{
		if ((app->pid == pid) && app->cb_app_name == 1 && app->appName[0] == '?') {
			cbName = std::min(cbName, MAX_APP_NAME);
			memcpy(app->appName, szName, cbName);
			app->appName[cbName] = 0;
			app->cb_app_name = cbName;
			return true;
		}
		app = (APP_NODE*)app->prevSibling;
	}
	return false;
}

APP_NODE* Archive::getApp(ROW_LOG_REC* p, sockaddr_in *p_si_other)
{
    if (curApp)
    {
        if (curApp && (curApp->pid == p->pid)) //&& (0 == memcmp(curApp->appPath(), p->appPath(), p->cb_app_path))
            return curApp;
    }

    curApp = (APP_NODE*)gArchive.getRootNode()->lastChild;
    //stdlog("curApp 1 %p\n", curApp);
    while (curApp)
    {
        if ((curApp->pid == p->pid)) //&& (0 == memcmp(curApp->appPath(), p->appPath(), p->cb_app_path))
            break;
        curApp = (APP_NODE*)curApp->prevSibling;
    }
    //stdlog("curApp 2 %p\n", curApp);
    if (!curApp)
    {
        curApp = addApp(p, p_si_other);
        //curApp->getData()->Log();
    }

    return curApp;
}

THREAD_NODE* Archive::getThread(APP_NODE* pAppNode, ROW_LOG_REC* p)
{
    if (curThread && curThread->tid == p->tid && pAppNode->pid == p->tid)
        return curThread;

    curThread = (THREAD_NODE*)pAppNode->lastChild;
    while (curThread)
    {
        if (curThread->tid == p->tid)
            break;
        curThread = (THREAD_NODE*)curThread->prevSibling;
    }

    if (!curThread)
    {
        curThread = addThread(p, pAppNode);
    }

    return curThread;
}

LOG_NODE* Archive::addFlow(THREAD_NODE* pThreadNode, ROW_LOG_REC *pLogRec, int bookmark)
{
    int cb_fn_name = pLogRec->cb_fn_name;
    const char* fnName = pLogRec->fnName();
    if (fnName[0] == '^')
    {
        fnName++;
        cb_fn_name--;
    }
    if (0 == (pLogRec->log_flags & LOG_FLAG_JAVA))
        pLogRec->cb_java_call_site = 0;
	else {
		const char* trace = pLogRec->trace();
		while (pLogRec->cb_java_call_site &&  (trace[pLogRec->cb_java_call_site - 1] == '\r' || trace[pLogRec->cb_java_call_site - 1] == '\n'))
			pLogRec->cb_java_call_site--;
	}

    FLOW_NODE* pNode = (FLOW_NODE*)m_pNodes->Add(sizeof(FLOW_NODE) + cb_fn_name + pLogRec->cb_module_name + pLogRec->cb_java_call_site + 1, true);
    if (!pNode)
        return nullptr;

#ifdef _NN_TEST
	pNode->prev_nn = g_prev_nn;
	pNode->pack_nn = g_pack_nn;
	pNode->retry_nn = g_retry_nn;
	pNode->buff_nn = g_buff_nn;
#endif
	pNode->data_type = FLOW_DATA_TYPE;
    pNode->nn = pLogRec->nn;
    pNode->log_type = pLogRec->log_type;
	pNode->log_flags = pLogRec->log_flags;
	pNode->sec = pLogRec->sec;
	pNode->msec = pLogRec->msec;
    pNode->this_fn = pLogRec->this_fn;
    pNode->call_site = pLogRec->call_site;
    pNode->fn_line = pLogRec->fn_line;
    pNode->call_line = pLogRec->call_line;
    if (bookmark) {
        bookmarkNumber++;
        pNode->bookmark = bookmark;
    }

    pNode->cb_fn_name = cb_fn_name;
    memcpy(pNode->fnName(), fnName, cb_fn_name);
    pNode->cb_short_fn_name_offset = 0xFFFF;

    if (pLogRec->cb_module_name)
    {
        pNode->cb_module_name = pLogRec->cb_module_name;
        memcpy(pNode->fnName() + cb_fn_name, pLogRec->moduleName(), pNode->cb_module_name);
    }

    if (pLogRec->cb_java_call_site && (pLogRec->log_flags & LOG_FLAG_JAVA))
    {
        pNode->cb_java_call_site = pLogRec->cb_java_call_site;
        memcpy(pNode->JavaCallSite(), pLogRec->trace(), pLogRec->cb_java_call_site);
    }

    pNode->threadNode = pThreadNode;

    ((FLOW_NODE*)pNode)->addToTree();

    return pNode;

}

LOG_NODE* Archive::addTrace(THREAD_NODE* pThreadNode, ROW_LOG_REC *pLogRec, int bookmark)
{
	unsigned char* trace = (unsigned char*)pLogRec->trace();
	BYTE color = pLogRec->color;
	int cb_trace = pLogRec->cb_trace;
	trace[cb_trace] = 0;

	bool endsWithNewLine = (trace[cb_trace - 1] == '\n' || trace[cb_trace - 1] == '\r');
	if (endsWithNewLine)
		cb_trace--;
#ifdef _DEBUG
    //if (221 == pLogRec->call_line)
    //if (0 != strstr((char*)trace, "tsc.c;3145"))
    //    int k = 0;
#endif

    bool newChank = false;
    // check if we can append to previous trace
    if (pThreadNode->latestTrace)
    {
        if (pThreadNode->latestTrace->hasNewLine == 0)
        {
            if (pThreadNode->latestTrace->cb_trace + cb_trace < MAX_TRCAE_LEN)
                newChank = true;
            else
                pThreadNode->latestTrace->hasNewLine = 1;
        }
        if (endsWithNewLine && newChank)
            pThreadNode->latestTrace->hasNewLine = 1;
        if (cb_trace == 0 && pLogRec->call_line == pThreadNode->latestTrace->call_line)
            newChank = true;
    }

    if (newChank)
    {
		if (cb_trace > 0)
		{
			TRACE_CHANK* pLastChank = pThreadNode->latestTrace->getLastChank();
			pLastChank->next_chank = (TRACE_CHANK*)Alloc(sizeof(TRACE_CHANK) + cb_trace);
			if (!pLastChank->next_chank)
				return nullptr;
			TRACE_CHANK* pChank = pLastChank->next_chank;
			pChank->len = cb_trace;
			pChank->next_chank = 0;
			memcpy(pChank->trace, trace, cb_trace);
			pChank->trace[cb_trace] = 0;
			pThreadNode->latestTrace->cb_trace += cb_trace;
		}
	}
    else
    {
        //add new trace
        int cb_fn_name = pLogRec->cb_fn_name;
        const char* fnName = pLogRec->fnName();
        if (fnName[0] == '^')
        {
            fnName++;
            cb_fn_name--;
        }
        TRACE_NODE* pNode = (TRACE_NODE*)m_pNodes->Add(sizeof(TRACE_NODE) + cb_fn_name + pLogRec->cb_module_name + sizeof(TRACE_CHANK) + cb_trace, true);
        if (!pNode)
            return nullptr;

        pNode->data_type = TRACE_DATA_TYPE;

#ifdef _NN_TEST
		pNode->prev_nn = g_prev_nn;
		pNode->pack_nn = g_pack_nn;
		pNode->retry_nn = g_retry_nn;
		pNode->buff_nn = g_buff_nn;
#endif
		pNode->color = color;
		pNode->priority = pLogRec->priority;
		pNode->nn = pLogRec->nn;
        pNode->log_type = pLogRec->log_type;
		pNode->log_flags = pLogRec->log_flags;
		pNode->sec = pLogRec->sec;
		pNode->msec = pLogRec->msec;
		pNode->call_line = pLogRec->call_line;
        if (bookmark) {
            bookmarkNumber++;
            pNode->bookmark = bookmark;
        }

        pNode->cb_fn_name = cb_fn_name;
        memcpy(pNode->fnName(), fnName, cb_fn_name);
        pNode->cb_short_fn_name_offset = 0xFFFF;

        if (pLogRec->cb_module_name)
        {
            pNode->cb_module_name = pLogRec->cb_module_name;
            memcpy(pNode->fnName() + cb_fn_name, pLogRec->moduleName(), pNode->cb_module_name);
        }

        TRACE_CHANK* pChank = pNode->getFirestChank();
        pChank->len = cb_trace;
        pChank->next_chank = 0;
        memcpy(pChank->trace, trace, cb_trace);
        pChank->trace[cb_trace] = 0;

        pNode->threadNode = pThreadNode;
        if (endsWithNewLine)
            pNode->hasNewLine = 1;

        if (pThreadNode->curentFlow && pThreadNode->curentFlow->isOpenEnter())
        {
            pNode->parent = pThreadNode->curentFlow;
        }
        else
        {
            pNode->parent = pThreadNode;
        }

        pThreadNode->latestTrace = pNode;
    }
	//at this point latestTrace is not null.
	if (!pThreadNode->latestTrace->color)
		pThreadNode->latestTrace->color = color;
	if (pThreadNode->latestTrace->priority != 0)
		pThreadNode->latestTrace->priority = pLogRec->priority;

	pThreadNode->latestTrace->lengthCalculated = 0;
    return pThreadNode->latestTrace;
}

void Archive::Log(ROW_LOG_REC* rec)
{
  stdlog(" rec-> len: %d, type: %d, flags: %d, nn: %d, "
    "cb_app: %d, cb_module: %d, cb_fn: %d, cb_trace: %d, "
    "pid: %d, tid: %d, sec: %d, msec: %u, "
    "this_fn: %x, call_site: %x, fn_line: %d, call_line: %d, data: %s\n\n",
    rec->len, rec->log_type, rec->log_flags, rec->nn,
    rec->cb_app_name, rec->cb_module_name, rec->cb_fn_name, rec->cb_trace,
    rec->pid, rec->tid, rec->sec, rec->msec,
    rec->this_fn, rec->call_site, rec->fn_line, rec->call_line, rec->appName());
}

int Archive::append(ROW_LOG_REC* rec, sockaddr_in *p_si_other, bool fromImport, int bookmark, NET_PACK_INFO* pack)
{
	//Log(rec);
#ifdef _LOSE_TEST
	static DWORD archiveNN = INFINITE;
	static DWORD lstNN = INFINITE;
	static DWORD lstApp = INFINITE;
	if (archiveNN != archiveNumber || lstApp != rec->tid)
	{
		lstNN = INFINITE;
		lstApp = rec->tid;
		m_lost = 0;
		archiveNN = archiveNumber;
	}
	if (lstNN != rec->nn && lstNN != INFINITE && !fromImport)
	{
		int lost = (rec->nn - lstNN > 0) ? rec->nn - lstNN : lstNN - rec->nn;
		m_lost += lost;
		Helpers::UpdateStatusBar();
	}
	lstNN = rec->nn + 1;
	return true;
#endif

	if (!rec->isValid())
        return 0;

	//if (rec->log_flags & LOG_FLAG_JAVA)
	//	return 1;

//	static APP_NODE* curApp0 = curApp;
	APP_NODE* pAppNode = getApp(rec, p_si_other);
    if (!pAppNode)
        return 1;

	if (pack && pack->pack_nn > 0)
	{
		if (pack->pack_nn <= pAppNode->lastPackNN)
		{
			return 1;
		}
		pAppNode->lastPackNN = pack->pack_nn;
		pack->pack_nn = -pack->pack_nn; //stop checking package number
	}

    //if (rec->nn == 110586)
    //    int iiii = 0;
    if (rec->nn && pAppNode->lastRecNN != INFINITE && !fromImport)
    {
		if ((pAppNode->lastRecNN != rec->nn - 1) && (pAppNode->lastRecNN != rec->nn)) {
			DWORD lost = (rec->nn > pAppNode->lastRecNN) ? rec->nn - pAppNode->lastRecNN : pAppNode->lastRecNN - rec->nn;
			pAppNode->lost += lost;
			m_lost += lost;
			Helpers::UpdateStatusBar();
		}
	}
#ifdef _NN_TEST
	g_prev_nn = (pAppNode->lastRecNN != INFINITE) ? (pAppNode->lastRecNN) : (rec->nn - 1);
	if (pack) {
		g_pack_nn = -pack->pack_nn;
		g_retry_nn = pack->retry_nn;
		g_buff_nn = pack->buff_nn-1;
	}
#endif
	pAppNode->lastRecNN = rec->nn;
	
	THREAD_NODE* pThreadNode = getThread(pAppNode, rec);
    if (!pThreadNode)
        return 1;

    bool ignore = false;
    if (rec->log_type != LOG_TYPE_TRACE && (rec->log_flags & LOG_FLAG_JAVA))
    {
        FLOW_NODE* lastFlowNode = pThreadNode->curentFlow;
        if (rec->log_type == LOG_TYPE_ENTER)
        {
            if (lastFlowNode &&
                (lastFlowNode->log_flags & LOG_FLAG_OUTER_LOG) &&
                !(rec->log_flags & LOG_FLAG_OUTER_LOG) &&
                (lastFlowNode->log_type == LOG_TYPE_ENTER) &&
                lastFlowNode->this_fn == rec->this_fn &&
                !lastFlowNode->peer)
            {
                lastFlowNode->fn_line = rec->fn_line;
                ignore = true;
            }
        }
        else if (rec->log_type == LOG_TYPE_EXIT)
        {
            FLOW_NODE* pLastFlow = NULL;
            if (lastFlowNode &&
                lastFlowNode->lastChild &&
                lastFlowNode->lastChild->isFlow())
            {
                pLastFlow = (FLOW_NODE*)lastFlowNode->lastChild;
            }
            if (pLastFlow &&
                pLastFlow->peer &&
                !(pLastFlow->peer->log_flags & LOG_FLAG_OUTER_LOG) &&
                pLastFlow->peer->this_fn == rec->this_fn)
            {
                ignore = true;
            }
        }
    }
    if (ignore)
        return 1;

	LOG_NODE* pNode = nullptr;
    if (rec->log_type == LOG_TYPE_TRACE)
    {
		pNode = addTrace(pThreadNode, rec, bookmark);
	}
    else
    {
		pNode = addFlow(pThreadNode, rec, bookmark);
	}
	//if (rec->log_type == LOG_TYPE_TRACE)//if (curApp0 != curApp)
	//{
	//	stdlog("curApp %p, pThreadNode %p, pNode %p nodePid %d recPid %d nodeTid %d recTid %d\n", 
	//		curApp, pThreadNode, pNode, pNode->getPid(), rec->pid, pNode->getTid(), rec->tid);
	//	curApp0 = curApp;
	//}
	return pNode != nullptr;
}

DWORD Archive::getCount()
{
    return m_pNodes ? m_pNodes->Count() : 0;
}

void ListedNodes::updateList(BOOL flowTraceHiden)
{
    DWORD count = gArchive.getCount();
    for (DWORD i = archiveCount; i < count; i++)
    {
        addNode(gArchive.getNode(i), flowTraceHiden);
    }
    archiveCount = count;
}

void ListedNodes::applyFilter(BOOL flowTraceHiden)
{
    Free();
    archiveCount = gArchive.getCount();
    //stdlog("%u\n", GetTickCount());
    for (DWORD i = 0; i < archiveCount; i++)
    {
        addNode(gArchive.getNode(i), flowTraceHiden);
    }
    //stdlog("%u\n", GetTickCount());
}

void SNAPSHOT::update()
{
    first = 0, last = INFINITE;
    if (curSnapShot)
    {
        if (curSnapShot == 1)
        {
            size_t c = snapShots.size();
            first = snapShots[c - 1].pos;
        }
        else
        {
            first = (curSnapShot == 2) ? 0 : snapShots[curSnapShot - 3].pos;
            last = snapShots[curSnapShot - 2].pos;
        }
    }
}