#include "stdafx.h"
#include "Archive.h"
#include "Helpers.h"
#include "Settings.h"
#include "Addr2LineThread.h"

Archive    gArchive;
ROOT_NODE* rootNode;
LIST_NODES* listNodes;
SNAPSHOT snapshot;
DWORD Archive::archiveNumber = 0;
#define ONE_GIGABYTE (1024 * 1024 * 1024)

Archive::Archive()
{
  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    Helpers::SysErrMessageBox(TEXT("WSAStartup failed"));
  }

  InitializeCriticalSectionAndSpinCount(&cs, 0x00000400);
  c_alloc = ONE_GIGABYTE;
  while (c_alloc && 0 == (buf = (char*)malloc(c_alloc)))
  {
    c_alloc = ((c_alloc / 5) * 4) + 1;
  }

  int cbRec = sizeof(LOG_NODE) + sizeof(FLOW_DATA) + sizeof(int) + 15;
  c_max_rec = c_alloc / cbRec;
  listNodes = (LIST_NODES*)(buf + c_alloc - sizeof(LIST_NODES) - (c_max_rec * sizeof(int)));
  rootNode = (ROOT_NODE*)(((char*)listNodes) - sizeof(ROOT_NODE) - 1);
  node_array = rootNode - 1;
  m_pAddr2LineThread = NULL;
  clearArchive();
}

Archive::~Archive()
{
  //we intentionaly do not free buf for application quick exit
  //if (buf)
  //   free(buf); 

  //DeleteCriticalSection(&cs);
}

void Archive::clearArchive()
{
  archiveNumber++;
  if (m_pAddr2LineThread)
  {
    m_pAddr2LineThread->StopWork();
    delete m_pAddr2LineThread;
    m_pAddr2LineThread = NULL;
  }

  c_rec = 0;
  rec_end = buf;
  rootNode->init(0);
  curApp = 0;
  curProc = 0;
  listNodes->init();
  snapshot.clear();
  m_pAddr2LineThread = new Addr2LineThread();
  m_pAddr2LineThread->StartWork();
}

bool Archive::haveDataBuf(DWORD cb)
{
  const int maxReserveSize = 4 * sizeof(LOG_NODE) + sizeof(APP_DATA) + sizeof(PROC_DATA) + sizeof(FLOW_DATA) + sizeof(TRACE_DATA);
  return (rec_end + cb + maxReserveSize) < (char*)(node_array - c_rec - 1) && (c_rec < c_max_rec);
}

void Archive::resolveAddr(LOG_NODE* pNode)
{
  if (gSettings.GetResolveAddr())
  {
    if (m_pAddr2LineThread)
      m_pAddr2LineThread->Resolve(pNode);
  }
}

char* Archive::reservDataBuf(DWORD cb)
{
  char* buf = NULL;
  EnterCriticalSection(&cs);
  if (haveDataBuf(cb))
  {
    buf = rec_end;
    rec_end = rec_end + cb;
  }
  LeaveCriticalSection(&cs);
  return buf;
}

APP_NODE* Archive::addApp(char* app_path, int cb_app_path, DWORD app_sec, DWORD app_msec, DWORD nn, sockaddr_in *p_si_other)
{
  APP_NODE* pNode = (APP_NODE*)(node_array - c_rec);
  APP_DATA* pAppData = (APP_DATA*)rec_end;
  rec_end = rec_end + sizeof(APP_DATA) + cb_app_path;

  pAppData->init();
  pAppData->data_type = APP_DATA_TYPE;
  pAppData->app_sec = app_sec;
  pAppData->app_msec = app_msec;
  pAppData->cb_app_path = cb_app_path;
  pAppData->NN = nn;
  pAppData->threadCount = 0;
  pAppData->lost = 0;
  pAppData->ip_address[0] = 0;
  pAppData->p_addr_info = 0;
  pAppData->cb_addr_info = INFINITE;

  char* appPath = pAppData->appPath();
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
  pAppData->cb_app_name = strlen(name_part);

  pNode->init(pAppData);

  Helpers::GetTime(pAppData->local_sec, pAppData->local_msec);

  if (p_si_other)
  {
    char* str_ip = inet_ntoa(p_si_other->sin_addr);
    if (str_ip)
    {
      strncpy(pAppData->ip_address, str_ip, sizeof(pAppData->ip_address) - 1);
      pAppData->ip_address[sizeof(pAppData->ip_address) - 1] = 0;
    }
  }

  rootNode->add_child(pNode);
#ifdef _DEBUG
  pNode->index = gArchive.index(pNode);
#endif
  pNode->hasCheckBox = 1;
  pNode->checked = 1;
  c_rec++;
  resolveAddr(NULL);
  return pNode;
}

PROC_NODE* Archive::addProc(APP_NODE* pAppNode, int tid)
{
  PROC_NODE* pNode = (PROC_NODE*)(node_array - c_rec);
  PROC_DATA* pProcData = (PROC_DATA*)rec_end;
  rec_end = rec_end + sizeof(PROC_DATA);

  pProcData->threadNN = ++(pAppNode->getData()->threadCount);
  pProcData->init();
  pProcData->data_type = PROC_DATA_TYPE;
  pProcData->pAppNode = pAppNode;
  pProcData->tid = tid;
  pProcData->latestTrace = NULL;
  pProcData->emptLineColor = 0;
  pProcData->curentFlow = NULL;
  pProcData->cb_color_buf = 0;
  pNode->init(pProcData);

  pAppNode->add_child(pNode);
#ifdef _DEBUG
  pNode->index = gArchive.index(pNode);
#endif
  pNode->hasCheckBox = 1;
  pNode->checked = 1;
  c_rec++;
  return pNode;
}

APP_NODE* Archive::getApp(ROW_LOG_REC* p, sockaddr_in *p_si_other)
{
  if (curApp)
  {
    APP_DATA* pAppData = curApp->getData();
    if (curApp && (0 == memcmp(pAppData->appPath(), p->appPath(), p->cb_app_path)) && (pAppData->app_sec == p->app_sec && pAppData->app_msec == p->app_msec))
      return curApp;
  }

  curApp = (APP_NODE*)rootNode->lastChild;
  //stdlog("curApp 1 %p\n", curApp);
  while (curApp)
  {
    APP_DATA* pAppData = curApp->getData();
    //pAppData->Log(); p->Log();
    if ((0 == memcmp(pAppData->appPath(), p->appPath(), p->cb_app_path)) && (pAppData->app_sec == p->app_sec && pAppData->app_msec == p->app_msec))
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

PROC_NODE* Archive::getProc(APP_NODE* pAppNode, ROW_LOG_REC* p)
{
  if (curProc && curProc->getData()->tid == p->tid)
    return curProc;

  curProc = NULL;
  curProc = (PROC_NODE*)pAppNode->lastChild;
  while (curProc)
  {
    if (curProc->getData()->tid == p->tid)
      break;
    curProc = (PROC_NODE*)curProc->prevSibling;
  }

  if (!curProc)
  {
    curProc = addProc(pAppNode, p->tid);
  }

  return curProc;
}

void SetTime(PROC_NODE* pProcNode, INFO_DATA* pInfoData, ROW_LOG_REC* pRec, DWORD pc_sec, DWORD pc_msec)
{
  pInfoData->term_sec = pRec->sec;
  pInfoData->term_msec = pRec->msec;
  pInfoData->pc_sec = pc_sec;
  pInfoData->pc_msec = pc_msec;
}

LOG_NODE* Archive::addFlow(PROC_NODE* pProcNode, ROW_LOG_REC *pLogRec, DWORD pc_sec, DWORD pc_msec)
{
  LOG_DATA* pData = (LOG_DATA*)rec_end;
  FLOW_DATA* pFlowData = (FLOW_DATA*)rec_end;
  pFlowData->init();
  pFlowData->data_type = FLOW_DATA_TYPE;
  pFlowData->nn = pLogRec->nn;
  pFlowData->peer = NULL;
  pFlowData->log_type = pLogRec->log_type;
  SetTime(pProcNode, pFlowData, pLogRec, pc_sec, pc_msec);
  pFlowData->this_fn = pLogRec->this_fn;
  pFlowData->call_site = pLogRec->call_site;
  pFlowData->call_line = pLogRec->call_line;

  int cb_fn_name = pLogRec->cb_fn_name;
  char* fnName = pLogRec->fnName();
  if (fnName[0] == '^')
  {
    fnName++;
    cb_fn_name--;
  }
  pFlowData->cb_fn_name = cb_fn_name;
  memcpy(pFlowData->fnName(), fnName, cb_fn_name);

  rec_end += (sizeof(FLOW_DATA) + pFlowData->cb_fn_name);

  LOG_NODE* pNode = node_array - c_rec;
  pNode->init(pData);
  pNode->proc = pProcNode;

  ((FLOW_NODE*)pNode)->addToTree();

#ifdef _DEBUG
  pNode->index = gArchive.index(pNode);
#endif
  c_rec++;
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

static bool setCollor(PROC_NODE* pProcNode, unsigned char* pTrace, int i, int &cb_trace, int& color)
{
  PROC_DATA* pProcData = pProcNode->getData();
  char* pBuf = pProcData->COLOR_BUF;
  bool bRet = false;
  if (pTrace[i] == '\033')
  {
    pProcData->cb_color_buf = 0;
  }
  else if (pProcData->cb_color_buf == 0)
  {
    // no color in buffer and in trace
    return false;
  }
  bool reset_buffer = false;
  int cb_color_old = pProcData->cb_color_buf;
  int cb_color = min(cb_trace - i, (int)sizeof(pProcData->COLOR_BUF) - pProcData->cb_color_buf - 1);
  if (cb_color <= 0)
  {
    pProcData->cb_color_buf = 0;
    return false;
  }
  memcpy(pBuf + pProcData->cb_color_buf, pTrace + i, cb_color);
  pProcData->cb_color_buf += cb_color;
  pBuf[pProcData->cb_color_buf] = 0;

  int iSkip = 0, color1 = 0, color2 = 0;
  if (pBuf[0] == '\033' && pBuf[1] == '[')
  {
    iSkip = 2;
    color1 = getCollor(pBuf, iSkip, pProcData->cb_color_buf);
    if (pBuf[iSkip] == ';')
    {
      iSkip++;
      color2 = getCollor(pBuf, iSkip, pProcData->cb_color_buf);
    }
    if (pBuf[iSkip] == ';') //getting third color as color2
    {
      iSkip++;
      color2 = getCollor(pBuf, iSkip, pProcData->cb_color_buf);
    }

    if (iSkip < pProcData->cb_color_buf && pBuf[iSkip] == 'm')
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
    pProcData->cb_color_buf = 0;
  }

  return bRet;
}

LOG_NODE* Archive::addTrace(PROC_NODE* pProcNode, ROW_LOG_REC *pLogRec, int& prcessed, DWORD pc_sec, DWORD pc_msec)
{
  if (prcessed >= pLogRec->cb_trace)
    return NULL;

  LOG_DATA* pData = NULL;
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
  setCollor(pProcNode, pTrace, i, pLogRec->cb_trace, color);
  for (; i < pLogRec->cb_trace; i++)
  {
    if (pTrace[i] == '\033')
    {
      if (setCollor(pProcNode, pTrace, i, pLogRec->cb_trace, color))
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

  PROC_DATA* pProcData = pProcNode->getData();

  //do not add blank lines
  if (cb == cWhite)
  {
    if (pProcData->latestTrace && endsWithNewLine)
      pProcData->latestTrace->hasNewLine = 1;
    if (!pProcData->emptLineColor && color)
      pProcData->emptLineColor = color;
    prcessed += i;
    return NULL;
  }

  if (pProcData->emptLineColor && !color)
  {
    color = pProcData->emptLineColor;
  }
  pProcData->emptLineColor = 0;

  bool newChank = false;
  // check if we need append to previous trace
  if (pProcData->latestTrace) // && (pProcData->latestTrace->parent == pProcData->curentFlow)
  {
    TRACE_DATA* latestTraceData = pProcData->latestTrace->getData();
    //if ((latestTraceData->cb_fn_name != pLogRec->cb_fn_name) || (pLogRec->cb_fn_name && 0 != memcmp(latestTraceData->fnName(), pLogRec->fnName(), pLogRec->cb_fn_name)))
    //{
    //  pProcData->latestTrace->hasNewLine = 1; //trace from new function
    //}
    if (pProcData->latestTrace->hasNewLine == 0)
    {
      if (latestTraceData->cb_trace + cb < MAX_TRCAE_LEN)
        newChank = true;
      else
        pProcData->latestTrace->hasNewLine = 1;
    }
    if (endsWithNewLine && newChank)
      pProcData->latestTrace->hasNewLine = 1;
    if (cb == 0 && pLogRec->call_line == latestTraceData->call_line)
      newChank = true;
  }

  TRACE_DATA* pTraceData;
  if (newChank)
  {
    pTraceData = pProcData->latestTrace->getData();
    if (cb)
    {
      TRACE_CHANK* pLastChank = pTraceData->getLastChank();
      pLastChank->next_chank = (TRACE_CHANK*)rec_end;
      TRACE_CHANK* pChank = pLastChank->next_chank;
      pChank->len = cb;
      pChank->next_chank = 0;
      memcpy(pChank->trace, pLogRec->trace() + prcessed, cb);
      pChank->trace[cb] = 0;
      pTraceData->cb_trace += cb;
      rec_end += (sizeof(TRACE_CHANK) + cb);
    }
  }
  else
  {
    //add new trace
    pData = (LOG_DATA*)rec_end;
    pTraceData = (TRACE_DATA*)rec_end;
    pTraceData->init();
    pTraceData->data_type = TRACE_DATA_TYPE;
    pTraceData->cb_trace = cb;
    pTraceData->color = 0;

    pTraceData->nn = pLogRec->nn;
    pTraceData->log_type = pLogRec->log_type;
    SetTime(pProcNode, pTraceData, pLogRec, pc_sec, pc_msec);
    pTraceData->call_line = pLogRec->call_line;

    int cb_fn_name = pLogRec->cb_fn_name;
    char* fnName = pLogRec->fnName();
    if (fnName[0] == '^')
    {
      fnName++;
      cb_fn_name--;
    }
    pTraceData->cb_fn_name = cb_fn_name;
    memcpy(pTraceData->fnName(), fnName, cb_fn_name);

    TRACE_CHANK* pChank = pTraceData->getFirestChank();
    pChank->len = cb;
    pChank->next_chank = 0;
    memcpy(pChank->trace, pLogRec->trace() + prcessed, cb);
    pChank->trace[cb] = 0;
    rec_end += (sizeof(TRACE_DATA) + cb_fn_name + sizeof(TRACE_CHANK) + cb);
  }

  //update other fieds
  if (!pTraceData->color)
    pTraceData->color = color;

  prcessed += i;
  pTraceData->lengthCalculated = 0;

  if (pData)
  {
    LOG_NODE* pNode = node_array - c_rec;
    pNode->init(pData);
    pNode->proc = pProcNode;
    if (endsWithNewLine)
      pNode->hasNewLine = 1;

    PROC_DATA* pProc = pProcNode->getData();

    PROC_DATA* pProcData = pProcNode->getData();
    if (pProcData->curentFlow && pProcData->curentFlow->isOpenEnter())
    {
      pNode->parent = pProcData->curentFlow;
    }
    else
    {
      pNode->parent = pProcNode;
    }

    pProc->latestTrace = (TRACE_NODE*)pNode;

#ifdef _DEBUG
    pNode->index = gArchive.index(pNode);
#endif
    c_rec++;
    return pNode;
  }
  else
  {
    return NULL;
  }
}

bool Archive::append(ROW_LOG_REC* rec, DWORD pc_sec, DWORD pc_msec, sockaddr_in *p_si_other)
{
  bool ret = false;

  EnterCriticalSection(&cs);
  if (rec->isValid())
  {
    if (haveDataBuf(rec->len))
    {
      APP_NODE* pAppNode = getApp(rec, p_si_other);
      DWORD& NN = ((APP_DATA*)(pAppNode->data))->NN;
      if (NN != rec->nn)
      {
        ((APP_DATA*)(pAppNode->data))->lost += rec->nn - NN;
      }
      NN = rec->nn + 1;

      PROC_NODE* pProcNode = getProc(pAppNode, rec);

      if (rec->isFlow())
      {
        addFlow(pProcNode, rec, pc_sec, pc_msec);
      }
      else
      {
        int prcessed = 0;
        while (prcessed < rec->cb_trace)
        {
          int prcessed0 = prcessed;
          int cb_trace0 = rec->cb_trace;
          addTrace(pProcNode, rec, prcessed, pc_sec, pc_msec);
          if (prcessed0 >= prcessed && cb_trace0 <= rec->cb_trace && prcessed < rec->cb_trace)
          {
            ATLASSERT(FALSE);
            break;
          }

        }
      }

      ret = true;
    }
  }
  LeaveCriticalSection(&cs);

  return ret;
}

DWORD Archive::getCount()
{
  return c_rec;
}

void LIST_NODES::updateList(BOOL flowTraceHiden)
{
  DWORD count = gArchive.getCount();
  for (DWORD i = archiveCount; i < count; i++)
  {
    listNodes->addNode(gArchive.getNode(i), flowTraceHiden);
  }
  archiveCount = count;
}

void LIST_NODES::applyFilter(BOOL flowTraceHiden)
{
  init();
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