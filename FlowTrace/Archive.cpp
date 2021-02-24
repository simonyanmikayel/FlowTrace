#include "stdafx.h"
#include "Archive.h"
#include "Helpers.h"
#include "AddrInfo.h"
#include "Settings.h"
#include "LogReceiver.h"
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
	//m_pRecBuf = new MemBuf(MAX_BUF_SIZE / 2, 256 * 2 * 1024 * 1024);
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
	m_SkipedLogcat = 0;
	m_psNN = 0;
    curApp = 0;
    curThread = 0;

    delete m_pNodes;
	//delete m_pRecords;
    m_pNodes = nullptr;
	//m_pRecords = nullptr;
    m_rootNode = nullptr;
    m_pTraceBuf->Free();
	//m_pRecBuf->Free();
    m_listedNodes->Free();
    AddrInfo::Reset();
    if (!closing)
    {
		m_pNodes = new PtrArray<LOG_NODE>(m_pTraceBuf);
		//m_pRecords = new PtrArray<LOG_REC_BASE_DATA>(m_pRecBuf);
		m_rootNode = (ROOT_NODE*)m_pNodes->Add(sizeof(ROOT_NODE), true);
		m_rootNode->expanded = 1;
		m_rootNode->data_type = ROOT_DATA_TYPE;
        ATLASSERT(m_pNodes && m_rootNode);
    }
}

size_t Archive::UsedMemory() 
{
    return m_pTraceBuf->UsedMemory() + m_listedNodes->UsedMemory(); // + m_pRecBuf->UsedMemory()
}

APP_NODE* Archive::addApp(LOG_REC* p, sockaddr_in *p_si_other)
{
    APP_NODE* pNode = (APP_NODE*)m_pNodes->Add(sizeof(APP_NODE), true);
	LOG_REC_BASE_DATA* pLogData = p->getLogData();
    if (!pNode)
        return nullptr;
	const char* appName = p->appName();
	WORD cb_app_name = pLogData->cb_app_name;
	bool nameIsKnown = true;
	if (cb_app_name == 0) {
		cb_app_name = 1;
		appName = UNKNOWNP_APP_NAME;
		nameIsKnown = false;
	}
    pNode->data_type = APP_DATA_TYPE;
    pNode->pid = pLogData->pid;
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

    gArchive.getRootNode()->add_child(pNode);
    pNode->hasCheckBox = 1;
    pNode->checked = 1;

	if (!nameIsKnown)
		nameIsKnown = resolveAppName(pNode);

	if (nameIsKnown)
	{
		pNode->applyFilter();
	}
    return pNode;
}

static PS_INFO psInfo[maxPsInfo + 1];
static int cPsInfo;
bool  Archive::setPsInfo(PS_INFO* p, int c)
{
	gLogReceiver.lock();
	c = std::min(c, maxPsInfo);
	memcpy(psInfo, p, c * sizeof(PS_INFO));
	cPsInfo = c;
	gLogReceiver.unlock();

	m_psNN++;
	bool updateViews = false;
	APP_NODE* app;

	for (int i = 0; i < cPsInfo; i++) {
		app = setAppName(psInfo[i].pid, psInfo[i].name, psInfo[i].cName, updateViews);
		if (app)
		{
			setThreadName(app, psInfo[i].pid, psInfo[i].name, psInfo[i].cName, updateViews); //main thread

			int ppid = psInfo[i].pid;
			while (i < cPsInfo - 1 && ppid == psInfo[i + 1].ppid)
			{
				i++;
				setThreadName(app, psInfo[i].pid, psInfo[i].name, psInfo[i].cName, updateViews);
			}
		}
			
	}

	app = (APP_NODE*)gArchive.getRootNode()->lastChild;
	while (app)
	{
		if (app->pid != 0 && app->psNN >= 0 && app->psNN != m_psNN)
		{
			app->psNN = -m_psNN;
			updateViews = true;
		}
		THREAD_NODE* thread = (THREAD_NODE*)app->lastChild;
		while (thread)
		{
			if (thread->tid > 0 && thread->psNN >= 0 && thread->psNN != m_psNN)
			{
				//stdlog("~pid [ %d - %d ] - %d != %d\n", app->pid, thread->tid, thread->psNN, m_psNN);
				thread->psNN = -m_psNN;
				updateViews = true;
			}
			thread = (THREAD_NODE*)thread->prevSibling;
		}

		app = (APP_NODE*)app->prevSibling;
	}

	//stdlog("setPsInfo %d update %d  ==================================\n", c, updateViews);
	return updateViews;
}

APP_NODE* Archive::setAppName(int pid, char* szName, int cbName, bool& updateViews)
{
	APP_NODE* app = (APP_NODE*)gArchive.getRootNode()->lastChild;
	while (app)
	{
		if (app->pid == pid)
		{
			if (app->isUnknown() || app->isPreInitialized() || cbName != app->cb_app_name)
			{
				cbName = std::min(cbName, MAX_APP_NAME);
				app->cb_app_name = cbName;
				memcpy(app->appName, szName, cbName);
				app->appName[cbName] = 0;
				app->applyFilter();
				updateViews = true; //we need refresh views
			}
			if (app->psNN < 0)
				updateViews = true; //we need refresh views
			app->psNN = m_psNN;
			break;
		}
		app = (APP_NODE*)app->prevSibling;
	}
	return app;
}

THREAD_NODE* Archive::setThreadName(APP_NODE* app, int tid, char* szName, int cbName, bool& updateViews)
{
	THREAD_NODE* thread = (THREAD_NODE*)app->lastChild;
	while (thread)
	{
		if (thread->tid == tid)
		{
			//stdlog("pid [ %d - %d ] - %d\n", app->pid, thread->tid, thread->psNN);
			if (thread->threadName[0] == 0 || cbName != thread->cb_thread_name)
			{
				cbName = std::min(cbName, MAX_APP_NAME);
				thread->cb_thread_name = cbName;
				memcpy(thread->threadName, szName, cbName);
				thread->threadName[cbName] = 0;
				updateViews = true; //we need refresh views
			}
			if (thread->psNN < 0)
				updateViews = true; //we need refresh views
			thread->psNN = m_psNN;
			break;
		}
		thread = (THREAD_NODE*)thread->prevSibling;
	}
	return thread;
}

bool Archive::resolveAppName(APP_NODE* app)
{
	for (int i = 0; i < cPsInfo; i++) {
		if ((app->pid == psInfo[i].pid)) {
			int cbName = std::min(psInfo[i].cName, MAX_APP_NAME);
			memcpy(app->appName, psInfo[i].name, cbName);
			app->appName[cbName] = 0;
			app->cb_app_name = cbName;
			return true;
		}
	}
	return false;
}

THREAD_NODE* Archive::addThread(LOG_REC* p, APP_NODE* pAppNode)
{
    THREAD_NODE* pNode = (THREAD_NODE*)m_pNodes->Add(sizeof(THREAD_NODE) + p->cbModuleName() + 1, true);
	LOG_REC_BASE_DATA* pLogData = p->getLogData();
    if (!pNode)
        return nullptr;

    pNode->threadNN = ++(pAppNode->threadCount);
    pNode->data_type = THREAD_DATA_TYPE;
    pNode->pAppNode = pAppNode;
    pNode->tid = pLogData->tid;
    
    pAppNode->add_child(pNode);
    pNode->hasCheckBox = 1;
    pNode->checked = 1;

    return pNode;
}

APP_NODE* Archive::getApp(LOG_REC* p, sockaddr_in *p_si_other)
{
	LOG_REC_BASE_DATA* pLogData = p->getLogData();
    if (curApp)
    {
		if (curApp && (curApp->pid == pLogData->pid)) //&& (0 == memcmp(curApp->appPath(), p->appPath(), p->cb_app_path))
		{
			return curApp;
		}
    }

    curApp = (APP_NODE*)gArchive.getRootNode()->lastChild;
    //stdlog("curApp 1 %p\n", curApp);
    while (curApp)
    {
		if ((curApp->pid == pLogData->pid))
		{
			break;
		}
        curApp = (APP_NODE*)curApp->prevSibling;
    }
    //stdlog("curApp 2 %p\n", curApp);
    if (!curApp)
    {
        curApp = addApp(p, p_si_other);
        //curApp->getData()->Log();
    }

	if (curApp && p_si_other && !curApp->ip_address[0])
	{
		char* str_ip = inet_ntoa(p_si_other->sin_addr);
		if (str_ip)
		{
			strncpy_s(curApp->ip_address, sizeof(curApp->ip_address), str_ip, sizeof(curApp->ip_address) - 1);
			curApp->ip_address[sizeof(curApp->ip_address) - 1] = 0;
		}
		else
		{
			curApp->ip_address[0] = '?';
		}
	}

    return curApp;
}

THREAD_NODE* Archive::getThread(APP_NODE* pAppNode, LOG_REC* p)
{
	LOG_REC_BASE_DATA* pLogData = p->getLogData();

    if (curThread && curThread->tid == pLogData->tid && pAppNode->pid == pLogData->tid)
        return curThread;

    curThread = (THREAD_NODE*)pAppNode->lastChild;
    while (curThread)
    {
        if (curThread->tid == pLogData->tid)
            break;
        curThread = (THREAD_NODE*)curThread->prevSibling;
    }

    if (!curThread)
    {
        curThread = addThread(p, pAppNode);
    }

    return curThread;
}

LOG_NODE* Archive::addFlow(THREAD_NODE* pThreadNode, LOG_REC *pLogRec, int bookmark, bool fromImport)
{
	LOG_REC_BASE_DATA* pLogData = pLogRec->getLogData();

    int cb_fn_name = pLogData->cb_fn_name;
    const char* fnName = pLogRec->fnName();
    if (fnName[0] == '^')
    {
        fnName++;
        cb_fn_name--;
    }

    FLOW_NODE* pNode = (FLOW_NODE*)m_pNodes->Add(sizeof(FLOW_NODE) + cb_fn_name + pLogData->cb_module_name + 1, true);
    if (!pNode)
        return nullptr;

#ifdef _NN_TEST
	pNode->prev_nn = g_prev_nn;
	pNode->pack_nn = g_pack_nn;
	pNode->retry_nn = g_retry_nn;
	pNode->buff_nn = g_buff_nn;
#endif
	pNode->data_type = FLOW_DATA_TYPE;
    pNode->nn = pLogData->nn;
    pNode->log_type = pLogData->log_type;
	pNode->log_flags = pLogData->log_flags;
	pNode->sec = pLogData->sec;
	pNode->msec = pLogData->msec;
    pNode->this_fn = pLogData->this_fn;
    pNode->call_site = pLogData->call_site;
    pNode->fn_line = pLogData->fn_line;
    pNode->call_line = pLogData->call_line;
    if (bookmark) {
        bookmarkNumber++;
        pNode->bookmark = bookmark;
    }

    pNode->cb_fn_name = cb_fn_name;
    memcpy(pNode->fnName(), fnName, cb_fn_name);
    pNode->cb_short_fn_name_offset = 0xFFFF;

    if (pLogData->cb_module_name)
    {
        pNode->cb_module_name = pLogData->cb_module_name;
        memcpy(pNode->fnName() + cb_fn_name, pLogRec->moduleName(), pNode->cb_module_name);
    }

    pNode->threadNode = pThreadNode;

    ((FLOW_NODE*)pNode)->addToTree();

    return pNode;

}

LOG_NODE* Archive::addTrace(THREAD_NODE* pThreadNode, LOG_REC_BASE_DATA* pLogData, int bookmark, char* trace, int cb_trace, const char* fnName, const char* moduleName)
{
	bool endsWithNewLine = (cb_trace > 0 && trace[cb_trace - 1] == '\n' || trace[cb_trace - 1] == '\r');
	if (endsWithNewLine)
		cb_trace--;

	//stdlog("cb: %d color: %d %s\n", cb_trace, color, trace);
#ifdef _DEBUG
    //if (221 == pLogData->call_line)
    //if (0 != strstr((char*)trace, "tsc.c;3145"))
    //    int k = 0;
#endif

    bool newChank = false;
    // check if we can append to previous trace
    if (pThreadNode->latestTrace)
    {
        if (pThreadNode->latestTrace->hasNewLine == 0)
        {
            if (pThreadNode->latestTrace->cb_trace + cb_trace < MAX_LOG_LEN)
                newChank = true;
            else
                pThreadNode->latestTrace->hasNewLine = 1;

			if (endsWithNewLine)
				pThreadNode->latestTrace->hasNewLine = 1;
		}
    }

	if (cb_trace <= 0)
	{
		ATLASSERT(cb_trace == 0);
		return pThreadNode->latestTrace;
	}

    if (newChank)
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
    else
    {
        //add new trace
        int cb_fn_name = pLogData->cb_fn_name;
        if (fnName[0] == '^')
        {
            fnName++;
            cb_fn_name--;
        }
        TRACE_NODE* pNode = (TRACE_NODE*)m_pNodes->Add(sizeof(TRACE_NODE) + cb_fn_name + pLogData->cb_module_name + sizeof(TRACE_CHANK) + cb_trace, true);
        if (!pNode)
            return nullptr;

        pNode->data_type = TRACE_DATA_TYPE;
		 
#ifdef _NN_TEST
		pNode->prev_nn = g_prev_nn;
		pNode->pack_nn = g_pack_nn;
		pNode->retry_nn = g_retry_nn;
		pNode->buff_nn = g_buff_nn;
#endif
		pNode->color = pLogData->color;
		pNode->priority = pLogData->priority;
		pNode->nn = pLogData->nn;
        pNode->log_type = pLogData->log_type;
		pNode->log_flags = pLogData->log_flags;
		pNode->sec = pLogData->sec;
		pNode->msec = pLogData->msec;
		pNode->call_line = pLogData->call_line;
		pNode->call_site = pLogData->call_site;
		if (bookmark) {
            bookmarkNumber++;
            pNode->bookmark = bookmark;
        }

		if (pLogData->priority)
		{
			switch (pLogData->priority)
			{
			case FLOW_LOG_ERROR:
			case FLOW_LOG_FATAL:
				pNode->color = 31;	//Red
				break;
			case FLOW_LOG_WARN:
				pNode->color = 33;	//Yellow
				break;
			case FLOW_LOG_INFO:
				pNode->color = 32;	//Green
				break;
			}
		}

        pNode->cb_fn_name = cb_fn_name;
        memcpy(pNode->fnName(), fnName, cb_fn_name);
        pNode->cb_short_fn_name_offset = 0xFFFF;

        if (pLogData->cb_module_name)
        {
            pNode->cb_module_name = pLogData->cb_module_name;
            memcpy(pNode->fnName() + cb_fn_name, moduleName, pNode->cb_module_name);
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
		pThreadNode->latestTrace->color = pLogData->color;
	if (pThreadNode->latestTrace->priority != 0)
		pThreadNode->latestTrace->priority = pLogData->priority;
	if (cb_trace)
		pThreadNode->latestTrace->lengthCalculated = 0;

    return pThreadNode->latestTrace;
}

int parceCollor(char** c)
{
	int color = 0;
	if (isdigit(**c))
	{
		color = (**c) - '0';
		(*c)++;
		if (isdigit(**c))
		{
			color = (10 * color) + ((**c) - '0');
			(*c)++;
		}
		if (!((color >= 30 && color <= 37) || (color >= 40 && color <= 47)))
			color = 0;
	}
	return color;
}

LOG_NODE* Archive::addTrace(THREAD_NODE* pThreadNode, LOG_REC *pLogRec, int bookmark, bool fromImport)
{
	LOG_REC_BASE_DATA* pLogData = pLogRec->getLogData();
	const char* fnName = pLogRec->fnName();
	const char* moduleName = pLogRec->moduleName();
	int cb_trace = pLogData->cb_trace;
	char* trace = (char*)pLogRec->trace();
	LOG_NODE* pNode = nullptr;
	if (fromImport || pLogRec->getLogData()->log_flags & LOG_FLAG_COLOR_PARCED)
	{
		pNode = addTrace(pThreadNode, pLogData, bookmark, trace, (int)(cb_trace), fnName, moduleName);
	}
	else
	{
		char last_char = trace[cb_trace];
		trace[cb_trace] = 0;

#define ADD_TRACE(cb_trace, trace) pNode = addTrace(pThreadNode, pLogData, bookmark, trace, (int)(cb_trace), fnName, moduleName)

		int old_color = pLogData->color;
		char *start = trace;
		char *end = trace;
		while (*end) {
			while (*(end) >= ' ')
				end++;
			if (*end == '\n' || *end == '\r') {
				old_color = pLogData->color;
				ADD_TRACE(end - start + 1, start);
				while (*end == '\n' || *end == '\r')
					end++;
				start = end;
			}
			else if (*end == '\033' && *(end + 1) == '[') {
				int c1 = 0, c2 = 0, c3 = 0;
				char* colorPos = end;
				end += 2;
				c1 = parceCollor(&end);
				if (*end == ';') {
					end++;
					c2 = parceCollor(&end);
				}
				if (*end == ';') {
					end++;
					c3 = parceCollor(&end);
				}
				if (*end == 'm')
				{
					end++;
					if (!pLogData->color) pLogData->color = c1;
					if (!pLogData->color) pLogData->color = c2;
					if (!pLogData->color) pLogData->color = c3;
				}
				if (colorPos > start) {
					ADD_TRACE(colorPos - start, start);
				}
				old_color = pLogData->color;
				start = end;
			}
			else if (*end) {
				if (*end < ' ') {
					if (*end == '\t') {
						*end = ' ';
					}
					else {
						*end = '?';
					}
				}
				end++;
			}
		}
		if (end > start)
		{
			ADD_TRACE(end - start, start);
		}
		else if (old_color != pLogData->color) {
			ADD_TRACE(0, end);
		}

		trace[cb_trace] = last_char;
	}
	return pNode;

}

void Archive::Log(LOG_REC* rec)
{
	LOG_REC_BASE_DATA* pLogData = rec->getLogData();
	stdlog(" rec-> len: %d, type: %d, flags: %d, nn: %d, "
    "cb_app: %d, cb_module: %d, cb_fn: %d, cb_trace: %d, "
    "pid: %d, tid: %d, sec: %d, msec: %u, "
    "this_fn: %x, call_site: %x, fn_line: %d, call_line: %d, data: %s\n\n",
		pLogData->len, pLogData->log_type, pLogData->log_flags, pLogData->nn,
		pLogData->cb_app_name, pLogData->cb_module_name, pLogData->cb_fn_name, pLogData->cb_trace,
		pLogData->pid, pLogData->tid, pLogData->sec, pLogData->msec,
		pLogData->this_fn, pLogData->call_site, pLogData->fn_line, pLogData->call_line, rec->appName());
}

int Archive::append(LOG_REC_ADB_DATA* pLogData, sockaddr_in *p_si_other, int bookmark, NET_PACK_INFO* pack)
{
	LOG_REC_ADB rec(pLogData);
	if (!rec.isValid()) {
		ATLASSERT(0);
		return 0;
	}
	appendRec(&rec, p_si_other, false, bookmark, pack);
	return 1;
}

int Archive::append(LOG_REC_NET_DATA* pLogData, sockaddr_in *p_si_other, bool fromImport, int bookmark, NET_PACK_INFO* pack)
{
	LOG_REC_NET rec(pLogData);
	if (!rec.isValid()) {
		ATLASSERT(0);
		return 0;
	}
	appendRec(&rec, p_si_other, fromImport, bookmark, pack);
	return 1;
}

void Archive::appendRec(LOG_REC* rec, sockaddr_in *p_si_other, bool fromImport, int bookmark, NET_PACK_INFO* pack)
{
	LOG_REC_BASE_DATA* pLogData = rec->getLogData();
	//Log(rec);
#ifdef _LOSE_TEST
	static DWORD archiveNN = INFINITE;
	static DWORD lstNN = INFINITE;
	static DWORD lstApp = INFINITE;
	if (pLogData->nn)
	{
		if (archiveNN != archiveNumber || lstApp != pLogData->tid)
		{
			lstNN = INFINITE;
			lstApp = pLogData->tid;
			m_lost = 0;
			archiveNN = archiveNumber;
			stdlog("lstApp = %d\n", lstApp);
		}
		if (lstNN != pLogData->nn && lstNN != INFINITE && !fromImport)
		{
			int lost = (pLogData->nn - lstNN > 0) ? pLogData->nn - lstNN : lstNN - pLogData->nn;
			m_lost += lost;
			Helpers::UpdateStatusBar();
			stdlog("lstApp = %d lstNN = %d pLogData->nn = %d\n", lstApp, lstNN, pLogData->nn);
		}
		lstNN = pLogData->nn + 1;
	}
	return;
#endif

	//if (pLogData->log_flags & LOG_FLAG_JAVA)
	//	return;

//	static APP_NODE* curApp0 = curApp;
	int log_type = rec->getLogData()->log_type;
	APP_NODE* pAppNode = getApp(rec, p_si_other);
    if (!pAppNode)
        return;
	if (log_type == LOG_TYPE_APP)
	{
		pAppNode->psNN = rec->getLogData()->nn;
		return;
	}

	if (pack && pack->pack_nn > 0 && !fromImport)
	{
		if (pack->pack_nn <= pAppNode->lastPackNN)
		{
			return;
		}
		pAppNode->lastPackNN = pack->pack_nn;
		pack->pack_nn = -pack->pack_nn; //stop checking package number
	}

    //if (pLogData->nn == 110586)
    //    int iiii = 0;
    if (pLogData->nn && pAppNode->lastRecNN != INFINITE && !fromImport)
    {
		if ((pAppNode->lastRecNN != pLogData->nn - 1) && (pAppNode->lastRecNN != pLogData->nn)) {
			DWORD lost = (pLogData->nn > pAppNode->lastRecNN) ? pLogData->nn - pAppNode->lastRecNN : pAppNode->lastRecNN - pLogData->nn;
			pAppNode->lost += lost;
			m_lost += lost;
			Helpers::UpdateStatusBar();
		}
	}
#ifdef _NN_TEST
	g_prev_nn = (pAppNode->lastRecNN != INFINITE) ? (pAppNode->lastRecNN) : (pLogData->nn - 1);
	if (pack) {
		g_pack_nn = -pack->pack_nn;
		g_retry_nn = pack->retry_nn;
		g_buff_nn = pack->buff_nn-1;
	}
#endif
	if (pLogData->nn && !fromImport)
		pAppNode->lastRecNN = pLogData->nn;
	
	THREAD_NODE* pThreadNode = getThread(pAppNode, rec);
    if (!pThreadNode)
		return;
	if (log_type == LOG_TYPE_THREAD)
	{
		pThreadNode->psNN = rec->getLogData()->nn;
		int cbName = std::min((int)(rec->getLogData()->cb_fn_name), MAX_APP_NAME);
		pThreadNode->cb_thread_name = cbName;
		memcpy(pThreadNode->threadName, rec->fnName(), cbName);
		pThreadNode->threadName[cbName] = 0;
		return;
	}

	if (pLogData->log_flags & LOG_FLAG_JAVA)
	{
		if (pLogData->log_flags & LOG_FLAG_OUTER_LOG)
		{
			pThreadNode->java_outer_this_fn = pLogData->this_fn;
			pThreadNode->java_outer_fn_line = pLogData->fn_line;
			return;
		}
		if (pLogData->log_type == LOG_TYPE_ENTER)
		{
			if (pThreadNode->java_outer_this_fn == pLogData->this_fn)
				pLogData->call_line = pThreadNode->java_outer_fn_line;
		}
	}
	pThreadNode->java_outer_this_fn = 0;

	LOG_NODE* pNode = nullptr;
    if (pLogData->log_type == LOG_TYPE_TRACE)
    {
		pNode = addTrace(pThreadNode, rec, bookmark, fromImport);
	}
    else
    {
		pNode = addFlow(pThreadNode, rec, bookmark, fromImport);
	}
	//if (pLogData->log_type == LOG_TYPE_TRACE)//if (curApp0 != curApp)
	//{
	//	stdlog("curApp %p, pThreadNode %p, pNode %p nodePid %d recPid %d nodeTid %d recTid %d\n", 
	//		curApp, pThreadNode, pNode, pNode->getPid(), pLogData->pid, pNode->getTid(), pLogData->tid);
	//	curApp0 = curApp;
	//}
}

void ListedNodes::updateList(BOOL flowTraceHiden)
{
	DWORD count = gArchive.getNodeCount();
	for (DWORD i = archiveCount; i < count; i++)
	{
		addNode(gArchive.getNode(i), flowTraceHiden);
	}
	archiveCount = count;
}

void ListedNodes::applyFilter(BOOL flowTraceHiden)
{
    Free();
    archiveCount = gArchive.getNodeCount();
    //stdlog("%u\n", GetTickCount());
    for (DWORD i = 0; i < archiveCount; i++)
    {
        addNode(gArchive.getNode(i), flowTraceHiden);
    }
    //stdlog("%u\n", GetTickCount());
}
