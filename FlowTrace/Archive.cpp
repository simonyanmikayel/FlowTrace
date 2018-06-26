#include "stdafx.h"
#include "Archive.h"
#include "Helpers.h"
#include "Settings.h"
#include "Addr2LineThread.h"

Archive    gArchive;
DWORD Archive::archiveNumber = 0;

Archive::Archive()
{
    ZeroMemory(this, sizeof(*this));
    m_pTraceBuf = new MemBuf(MAX_BUF_SIZE, 256 * 1024 * 1024);
    m_listedNodes = new ListedNodes();
    clearArchive();
}

Archive::~Archive()
{
    //we intentionaly do not free memory for this single instance application quick exit
    //delete m_pMemBuf;
}

void Archive::clearArchive(bool closing)
{
    archiveNumber++;
	bookmarkNumber = 0;
    if (m_pAddr2LineThread)
    {
        m_pAddr2LineThread->StopWork();
        delete m_pAddr2LineThread;
        m_pAddr2LineThread = nullptr;
    }

    curApp = 0;
    curThread = 0;
    gArchive.getSNAPSHOT().clear();

    delete m_pNodes;
    m_pNodes = nullptr;
    m_rootNode = nullptr;
    m_pTraceBuf->Free();
    m_listedNodes->Free();
    if (!closing)
    {
        m_pNodes = new PtrArray<LOG_NODE>(m_pTraceBuf);
        m_rootNode = (ROOT_NODE*)m_pNodes->Add(sizeof(ROOT_NODE), true);
        m_rootNode->data_type = ROOT_DATA_TYPE;
        ATLASSERT(m_pNodes && m_rootNode);
        m_pAddr2LineThread = new Addr2LineThread();
        m_pAddr2LineThread->StartWork();
    }
}

size_t Archive::UsedMemory() 
{
    return m_pTraceBuf->UsedMemory() + m_listedNodes->UsedMemory(); 
}

void Archive::resolveAddr(LOG_NODE* pSelectedNode, bool loop)
{
    if (gSettings.GetResolveAddr())
    {
        if (m_pAddr2LineThread)
            m_pAddr2LineThread->Resolve(pSelectedNode, loop);
    }
}

void Archive::resolveAddrAsync(LOG_NODE* pNode)
{
    if (gSettings.GetResolveAddr())
    {
        if (m_pAddr2LineThread)
            m_pAddr2LineThread->ResolveAsync(pNode);
    }
}

APP_NODE* Archive::addApp(char* app_path, int cb_app_path, DWORD app_sec, DWORD app_msec, DWORD nn, sockaddr_in *p_si_other)
{
    APP_NODE* pNode = (APP_NODE*)m_pNodes->Add(sizeof(APP_NODE) + cb_app_path, true);
    if (!pNode)
        return nullptr;

    pNode->data_type = APP_DATA_TYPE;
    pNode->app_sec = app_sec;
    pNode->app_msec = app_msec;
    pNode->cb_app_path = cb_app_path;
    pNode->NN = nn;
    pNode->cb_addr_info = INFINITE;

    char* appPath = pNode->appPath();
    memcpy(appPath, app_path, cb_app_path);
    appPath[cb_app_path] = 0;//do not rely on this in other functions. This will be ovewriten with next data
    char* name_part = strrchr(appPath, '/');
    if (name_part)
    {
        char* dot_part = strrchr(name_part, ':'); //like com.worldline.spica.tp:token_app
        if (dot_part)
            name_part = dot_part;
        name_part++;
    }
    else
    {
        name_part = appPath;
    }
    pNode->cb_app_name = (int)strlen(name_part);

    Helpers::GetTime(pNode->local_sec, pNode->local_msec);

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
    resolveAddrAsync();
    return pNode;
}

THREAD_NODE* Archive::addThread(APP_NODE* pAppNode, int tid)
{
    THREAD_NODE* pNode = (THREAD_NODE*)m_pNodes->Add(sizeof(THREAD_NODE), true);
    if (!pNode)
        return nullptr;

    pNode->threadNN = ++(pAppNode->threadCount);
    pNode->data_type = THREAD_DATA_TYPE;
    pNode->pAppNode = pAppNode;
    pNode->tid = tid;

    pAppNode->add_child(pNode);
    pNode->hasCheckBox = 1;
    pNode->checked = 1;
    return pNode;
}

APP_NODE* Archive::getApp(ROW_LOG_REC* p, sockaddr_in *p_si_other)
{
    if (curApp)
    {
        if (curApp && (0 == memcmp(curApp->appPath(), p->appPath(), p->cb_app_path)) && (curApp->app_sec == p->app_sec && curApp->app_msec == p->app_msec))
            return curApp;
    }

    curApp = (APP_NODE*)gArchive.getRootNode()->lastChild;
    //stdlog("curApp 1 %p\n", curApp);
    while (curApp)
    {
        if ((0 == memcmp(curApp->appPath(), p->appPath(), p->cb_app_path)) && (curApp->app_sec == p->app_sec && curApp->app_msec == p->app_msec))
            break;
        curApp = (APP_NODE*)curApp->prevSibling;
    }
    //stdlog("curApp 2 %p\n", curApp);
    if (!curApp)
    {
        curApp = addApp(p->appPath(), p->cb_app_path, p->app_sec, p->app_msec, p->nn, p_si_other);
        //curApp->getData()->Log();
    }

    return curApp;
}

THREAD_NODE* Archive::getThread(APP_NODE* pAppNode, ROW_LOG_REC* p)
{
    if (curThread && curThread->tid == p->tid)
        return curThread;

    curThread = NULL;
    curThread = (THREAD_NODE*)pAppNode->lastChild;
    while (curThread)
    {
        if (curThread->tid == p->tid)
            break;
        curThread = (THREAD_NODE*)curThread->prevSibling;
    }

    if (!curThread)
    {
        curThread = addThread(pAppNode, p->tid);
    }

    return curThread;
}

void SetTime(THREAD_NODE* pThreadNode, INFO_NODE* pInfoNode, ROW_LOG_REC* pRec, DWORD pc_sec, DWORD pc_msec)
{
    pInfoNode->term_sec = pRec->sec;
    pInfoNode->term_msec = pRec->msec;
    pInfoNode->pc_sec = pc_sec;
    pInfoNode->pc_msec = pc_msec;
}

LOG_NODE* Archive::addFlow(THREAD_NODE* pThreadNode, ROW_LOG_REC *pLogRec, DWORD pc_sec, DWORD pc_msec)
{
    int cb_fn_name = pLogRec->cb_fn_name;
    char* fnName = pLogRec->fnName();
    if (fnName[0] == '^')
    {
        fnName++;
        cb_fn_name--;
    }
    FLOW_NODE* pNode = (FLOW_NODE*)m_pNodes->Add(sizeof(FLOW_NODE) + cb_fn_name, true);
    if (!pNode)
        return nullptr;

    pNode->data_type = FLOW_DATA_TYPE;
    pNode->nn = pLogRec->nn;
    pNode->log_type = pLogRec->log_type;
    SetTime(pThreadNode, pNode, pLogRec, pc_sec, pc_msec);
    pNode->this_fn = pLogRec->this_fn;
    pNode->call_site = pLogRec->call_site;
    pNode->call_line = pLogRec->call_line;

    pNode->cb_fn_name = cb_fn_name;
    memcpy(pNode->fnName(), fnName, cb_fn_name);

    pNode->thread = pThreadNode;

    ((FLOW_NODE*)pNode)->addToTree();

    return pNode;

}

static int getCollor(char* pBuf, int &iSkip, int cb)
{
    int color = 0;
    if (iSkip < cb && isdigit(pBuf[iSkip]))
    {
        color = pBuf[iSkip] - '0';
        iSkip++;
        if (iSkip < cb && isdigit(pBuf[iSkip]))
        {
            color = (10 * color) + (pBuf[iSkip] - '0');
            iSkip++;
        }
    }
    return color;
}

static bool setCollor(THREAD_NODE* pThreadNode, unsigned char* pTrace, int i, int &cb_trace, int& color)
{
    char* pBuf = pThreadNode->COLOR_BUF;
    bool bRet = false;
    if (pTrace[i] == '\033')
    {
        pThreadNode->cb_color_buf = 0;
    }
    else if (pThreadNode->cb_color_buf == 0)
    {
        // no color in buffer and in trace
        return false;
    }
    bool reset_buffer = false;
    int cb_color_old = pThreadNode->cb_color_buf;
    int cb_color = min(cb_trace - i, (int)sizeof(pThreadNode->COLOR_BUF) - pThreadNode->cb_color_buf - 1);
    if (cb_color <= 0)
    {
        pThreadNode->cb_color_buf = 0;
        return false;
    }
    memcpy(pBuf + pThreadNode->cb_color_buf, pTrace + i, cb_color);
    pThreadNode->cb_color_buf += cb_color;
    pBuf[pThreadNode->cb_color_buf] = 0;

    int iSkip = 0, color1 = 0, color2 = 0;
    if (pBuf[0] == '\033' && pBuf[1] == '[')
    {
        iSkip = 2;
        color1 = getCollor(pBuf, iSkip, pThreadNode->cb_color_buf);
        if (pBuf[iSkip] == ';')
        {
            iSkip++;
            color2 = getCollor(pBuf, iSkip, pThreadNode->cb_color_buf);
        }
        if (pBuf[iSkip] == ';') //getting third color as color2
        {
            iSkip++;
            color2 = getCollor(pBuf, iSkip, pThreadNode->cb_color_buf);
        }

        if (iSkip < pThreadNode->cb_color_buf && pBuf[iSkip] == 'm')
        {
            iSkip++;
            bRet = true;
        }
    }

    if (iSkip - cb_color_old != cb_trace - i)
    {
        reset_buffer = true; //we have data after collor, so will wait for new one
    }

    if (iSkip)
    {
        iSkip -= cb_color_old;
        if (iSkip > 0)
        {
            if (cb_trace - i - iSkip >= 0)
            {
                memmove(pTrace + i, pTrace + i + iSkip, cb_trace - i - iSkip);
                cb_trace -= iSkip;
            }
            else
            {
                ATLASSERT(FALSE);
                reset_buffer = true;
                bRet = false;
            }
        }
    }

    if (bRet)
    {
        if (!color)
            color = color1 ? color1 : color2;
        reset_buffer = true;
    }

    if (reset_buffer)
    {
        pThreadNode->cb_color_buf = 0;
    }

    return bRet;
}

LOG_NODE* Archive::addTrace(THREAD_NODE* pThreadNode, ROW_LOG_REC *pLogRec, int& prcessed, DWORD pc_sec, DWORD pc_msec)
{
    if (prcessed >= pLogRec->cb_trace)
        return NULL;

    bool endsWithNewLine = false;
    unsigned char* pTrace = (unsigned char*)pLogRec->trace();
#ifdef _DEBUG
    //if (221 == pLogRec->call_line)
    if (0 != strstr((char*)pTrace, "tsc.c;3145"))
        int k = 0;
#endif
    int i = prcessed;
    int cWhite = 0;
    int color = 0;
    setCollor(pThreadNode, pTrace, i, pLogRec->cb_trace, color);
    for (; i < pLogRec->cb_trace; i++)
    {
        if (pTrace[i] == '\033')
        {
            if (setCollor(pThreadNode, pTrace, i, pLogRec->cb_trace, color))
            {
                i--;
                continue;
            }
        }

        if (pTrace[i] == '\n')
        {
            i++;
            break;
        }
        //remove contraol c
        if (pTrace[i] < ' ')
        {
            cWhite++;
            pTrace[i] = ' ';
        }
    }
    endsWithNewLine = (i > 0 && pTrace[i - 1] == '\n');
    i -= prcessed;
    if (i < 0)
    {
        ATLASSERT(FALSE);
        i = 0;
    }
    int cb = (endsWithNewLine && i > 0) ? (i - 1) : i;

    //do not add blank lines
    if (cb == cWhite)
    {
        if (pThreadNode->latestTrace && endsWithNewLine)
            pThreadNode->latestTrace->hasNewLine = 1;
        if (!pThreadNode->emptLineColor && color)
            pThreadNode->emptLineColor = color;
        prcessed += i;
        return NULL;
    }

    if (pThreadNode->emptLineColor && !color)
    {
        color = pThreadNode->emptLineColor;
    }
    pThreadNode->emptLineColor = 0;

    bool newChank = false;
    // check if we need append to previous trace
    if (pThreadNode->latestTrace) // && (pThreadNode->latestTrace->parent == pThreadNode->curentFlow)
    {
        if (pThreadNode->latestTrace->hasNewLine == 0)
        {
            if (pThreadNode->latestTrace->cb_trace + cb < MAX_TRCAE_LEN)
                newChank = true;
            else
                pThreadNode->latestTrace->hasNewLine = 1;
        }
        if (endsWithNewLine && newChank)
            pThreadNode->latestTrace->hasNewLine = 1;
        if (cb == 0 && pLogRec->call_line == pThreadNode->latestTrace->call_line)
            newChank = true;
    }

    if (newChank)
    {
        if (cb)
        {
            TRACE_CHANK* pLastChank = pThreadNode->latestTrace->getLastChank();
            pLastChank->next_chank = (TRACE_CHANK*)Alloc(sizeof(TRACE_CHANK) + cb);
            if (!pLastChank->next_chank)
                return nullptr;
            TRACE_CHANK* pChank = pLastChank->next_chank;
            pChank->len = cb;
            pChank->next_chank = 0;
            memcpy(pChank->trace, pLogRec->trace() + prcessed, cb);
            pChank->trace[cb] = 0;
            pThreadNode->latestTrace->cb_trace += cb;
        }
    }
    else
    {
        //add new trace
        int cb_fn_name = pLogRec->cb_fn_name;
        char* fnName = pLogRec->fnName();
        if (fnName[0] == '^')
        {
            fnName++;
            cb_fn_name--;
        }
        TRACE_NODE* pNode = (TRACE_NODE*)m_pNodes->Add(sizeof(TRACE_NODE) + cb_fn_name + sizeof(TRACE_CHANK) + cb, true);
        if (!pNode)
            return nullptr;

        pNode->data_type = TRACE_DATA_TYPE;

        pNode->nn = pLogRec->nn;
        pNode->log_type = pLogRec->log_type;
        SetTime(pThreadNode, pNode, pLogRec, pc_sec, pc_msec);
        pNode->call_line = pLogRec->call_line;

        pNode->cb_fn_name = cb_fn_name;
        memcpy(pNode->fnName(), fnName, cb_fn_name);

        TRACE_CHANK* pChank = pNode->getFirestChank();
        pChank->len = cb;
        pChank->next_chank = 0;
        memcpy(pChank->trace, pLogRec->trace() + prcessed, cb);
        pChank->trace[cb] = 0;

        pNode->thread = pThreadNode;
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

    //update other fieds
    if (!pThreadNode->latestTrace->color)
        pThreadNode->latestTrace->color = color;

    prcessed += i;
    pThreadNode->latestTrace->lengthCalculated = 0;
    return pThreadNode->latestTrace;
}

bool Archive::append(ROW_LOG_REC* rec, DWORD pc_sec, DWORD pc_msec, sockaddr_in *p_si_other)
{
    if (!rec->isValid())
        return false;

    APP_NODE* pAppNode = getApp(rec, p_si_other);
    if (!pAppNode)
        return false;
    DWORD& NN = pAppNode->NN;
    if (NN != rec->nn)
    {
        pAppNode->lost += rec->nn - NN;
    }
    NN = rec->nn + 1;

    THREAD_NODE* pThreadNode = getThread(pAppNode, rec);
    if (!pThreadNode)
        return false;

    if (rec->isFlow())
    {
        if (!addFlow(pThreadNode, rec, pc_sec, pc_msec))
            return false;
    }
    else
    {
        int prcessed = 0;
        while (prcessed < rec->cb_trace)
        {
            int prcessed0 = prcessed;
            int cb_trace0 = rec->cb_trace;
            addTrace(pThreadNode, rec, prcessed, pc_sec, pc_msec);
            if (prcessed0 >= prcessed && cb_trace0 <= rec->cb_trace && prcessed < rec->cb_trace)
            {
                ATLASSERT(FALSE);
                break;
            }
        }
    }

    return true;
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