#include "stdafx.h"
#include "Archive.h"
#include "Settings.h"

bool LOG_NODE::isSynchronized(LOG_NODE* pSyncNode)
{
  FLOW_NODE* pPeer = getPeer();
  return pSyncNode && (this == pSyncNode || pPeer == pSyncNode || isParent(pSyncNode) || (pPeer && pPeer->isParent(pSyncNode)));
}

void FLOW_NODE::addToTree()
{
  PROC_DATA* pProcData = proc->getData();

  if (pProcData->curentFlow == NULL)
  {
    proc->add_proc_child(this, ROOT_CHILD);
  }
  else
  {
    FLOW_DATA* pFlowData = (FLOW_DATA*)data;
    FLOW_DATA* pLatestFlowData = pProcData->curentFlow->getData();
    if (pFlowData->isEnter())
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
      if (pLatestFlowData->peer == NULL && pLatestFlowData->isEnter() && pLatestFlowData->this_fn == pFlowData->this_fn && pLatestFlowData->call_site == pFlowData->call_site)
      {
        pLatestFlowData->peer = this;
        pFlowData->peer = pProcData->curentFlow;
        if (pProcData->curentFlow->parent != proc) {
          pProcData->curentFlow = (FLOW_NODE*)(pProcData->curentFlow->parent);
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
  else
  {
    ATLASSERT(isFlow());
    FLOW_NODE* This = (FLOW_NODE*)this;
    return This->getData()->peer ? 3 : (This->getData()->isEnter() ? 4 : 5);//IDI_ICON_TREE_PAIRED, IDI_ICON_TREE_ENTER, IDI_ICON_TREE_EXIT
  }
}

int LOG_NODE::getListImage(bool selected)
{
  if (isRoot())
  {
    return 6;//IDI_ICON_EMPTY
  }
  else if (isApp())
  {
    return 6;//IDI_ICON_EMPTY
  }
  else if (isProc())
  {
    return 6;//IDI_ICON_EMPTY
  }
  else
  {
    if (isFlow())
    {
      FLOW_NODE* This = (FLOW_NODE*)this;
      FLOW_DATA* pData = This->getData();
      if (selected)
        return (pData->isEnter()) ? 3 : 4;//IDI_ICON_LIST_ENTER_SEL, IDI_ICON_LIST_EXIT_SEL
      else
        return (pData->isEnter()) ? 0 : 1;//IDI_ICON_LIST_ENTER, IDI_ICON_LIST_EXIT
    }
    else
    {
      return selected ? 5 : 2;//IDI_ICON_LIST_TRACE_SEL, IDI_ICON_LIST_TRACE
    }
  }
}

LOG_NODE* LOG_NODE::getSyncNode()
{
  LOG_NODE* pNode = this;
#ifdef NATIVE_TREE
  while (pNode && pNode->htreeitem == 0 && (pNode->getPeer() || pNode->parent))
  {
    if (pNode->getPeer())
    {
      pNode = pNode->getPeer();
      break; //peer should have htreeitem
    }
    else
    {
      pNode = pNode->parent;
    }
  }
  if (pNode && !pNode->htreeitem)
  {
    pNode = NULL;
  }
#else
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
#endif
  return pNode;
}

TCHAR* LOG_NODE::getTreeText(int* cBuf, bool extened)
{
  const int cMaxBuf = 255;
  static TCHAR pBuf[cMaxBuf + 1];
  int cb = 0;
  TCHAR* ret = pBuf;
#ifdef  NATIVE_TREE
  int NN = getNN();
#else
  int NN = getNN();
#ifdef _DEBUG
  //cb += _sntprintf(pBuf + cb, cMaxBuf, TEXT("[%d %d %d]"), GetExpandCount(), line, lastChild ? lastChild->index : 0);
#endif
#endif
  if (this == rootNode)
  {
    cb += _sntprintf(pBuf + cb, cMaxBuf, TEXT("..."));
  }
  else if (isApp())
  {
    APP_DATA* pAppData = (APP_DATA*)data;
    cb = pAppData->cb_app_name;
    memcpy(pBuf, pAppData->appName(), cb);
    pBuf[cb] = 0;
    if (pAppData->lost)
    {
      cb += _sntprintf(pBuf + cb, cMaxBuf - cb, TEXT(" (Lost: %d)"), pAppData->lost);
    }
    if (gSettings.GetShowAppIp() && pAppData->ip_address[0])
    {
      cb += _sntprintf(pBuf + cb, cMaxBuf - cb, TEXT(" - %s"), pAppData->ip_address);
    }
  }
  else if (isProc())
  {
    cb += _sntprintf(pBuf + cb, cMaxBuf, TEXT("[%d]"), getPid());
  }
  else if (isFlow())
  {
    FLOW_NODE* This = (FLOW_NODE*)this;
    FLOW_DATA* ThisData = (FLOW_DATA*)data;
    int cb_fn_name = ThisData->cb_fn_name;
    memcpy(pBuf + cb, ThisData->fnName(), cb_fn_name);
    cb += cb_fn_name;
    if (extened)
    {
      if (gSettings.GetColNN() && NN)
        cb += _sntprintf(pBuf + cb, cMaxBuf, TEXT(" (%d)"), NN); //gArchive.index(this) NN
      if (gSettings.GetShowElapsedTime() && This->getPeer())
      {
        _int64 sec1 = This->getTimeSec();
        _int64 msec1 = This->getTimeMSec();
        _int64 sec2 = (This->getPeer())->getTimeSec();
        _int64 msec2 = (This->getPeer())->getTimeMSec();
        cb += _sntprintf(pBuf + cb, cMaxBuf, TEXT(" (%lldms)"), (sec2 - sec1) * 1000 + (msec2 - msec1));
      }
    }
    if (gSettings.GetColCallAddr())
    {
      void* p = ThisData->call_site;
      //void* p = This->getCallAddr();
      //cb += _sntprintf(pBuf + cb, cMaxBuf, TEXT(" (%lluX)"), (DWORD64)p);
      cb += _sntprintf(pBuf + cb, cMaxBuf, TEXT(" (%p)"), p);
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


TCHAR* LOG_NODE::getListText(int *cBuf, LIST_COL col, int iItem)
{
  const int MAX_BUF_LEN = MAX_TRCAE_LEN - 1;
  static TCHAR pBuf[MAX_BUF_LEN + 1];
  int& cb = *cBuf;
  TCHAR* ret = pBuf;

  cb = 0;
  pBuf[0] = 0;

  if (col == LINE_NN_COL)
  {
    cb = _sntprintf(pBuf, MAX_BUF_LEN, TEXT("%d"), iItem);
  }
  else if (col == NN_COL)
  {
    int NN = getNN(); // gArchive.index(this); // getNN();
    if (NN)
      cb = _sntprintf(pBuf, MAX_BUF_LEN, TEXT("%d"), NN);
  }
  else if (col == APP_COLL)
  {
    cb = proc->getData()->pAppNode->getData()->cb_app_name;
    memcpy(pBuf, proc->getData()->pAppNode->getData()->appName(), cb);
    pBuf[cb] = 0;
  }
  else if (col == PROC_COL)
  {
    cb = _sntprintf(pBuf, MAX_BUF_LEN, TEXT("[%d]"), getPid());
  }
  else if (col == TIME_COL)
  {
    if (isInfo())
    {
      struct tm * timeinfo;
      DWORD sec = getTimeSec();
      DWORD msec = getTimeMSec();

      time_t rawtime = sec;
      timeinfo = localtime(&rawtime);
      if (timeinfo)
        cb = strftime(pBuf, MAX_BUF_LEN, "%H:%M:%S", timeinfo);

      //struct tm * timeinfo2;
      //time_t rawtime2 = _time32(NULL);
      //timeinfo2 = localtime(&rawtime2);
      //cb += strftime(pBuf + cb, MAX_BUF_LEN, " %H.%M.%S", timeinfo2);
      //int i = 4294967290 / 10000000;
      //        6553500000
      cb += _sntprintf(pBuf + cb, MAX_BUF_LEN - cb, TEXT(".%03d"), msec);
    }
  }
  else if (col == FUNC_COL)
  {
    if (isTrace())
    {
      cb = ((TRACE_DATA*)data)->cb_fn_name;
      memcpy(pBuf, ((TRACE_DATA*)data)->fnName(), cb);
      pBuf[cb] = 0;
    }
    else
    {
      cb = ((FLOW_DATA*)data)->cb_fn_name;
      memcpy(pBuf, ((FLOW_DATA*)data)->fnName(), cb);
      pBuf[cb] = 0;
    }
  }
  else if (col == CALL_LINE_COL)
  {
    int n = getCallLine();
    if (n)
      cb = _sntprintf(pBuf, MAX_BUF_LEN, TEXT("%d"), n);
  }
  else if (col == LOG_COL)
  {
    if (isFlow())
    {
      cb = ((FLOW_DATA*)data)->cb_fn_name;
      memcpy(pBuf, ((FLOW_DATA*)data)->fnName(), cb);
      pBuf[cb] = 0;
    }
    else if (isTrace())
    {
      cb = 0;
      TRACE_CHANK* pChank = ((TRACE_DATA*)data)->getFirestChank();
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
  return isInfo() ? ((INFO_DATA*)data)->nn : 0;
}
LONG LOG_NODE::getTimeSec()
{
  return isInfo() ? (gSettings.GetUsePcTime() ? ((INFO_DATA*)data)->pc_sec : ((INFO_DATA*)data)->term_sec) : 0LL;
}
LONG LOG_NODE::getTimeMSec()
{
  return isInfo() ? (gSettings.GetUsePcTime() ? ((INFO_DATA*)data)->pc_msec : ((INFO_DATA*)data)->term_msec) : 0LL;
}
int LOG_NODE::getPid()
{
  return  isProc() ? ((PROC_DATA*)data)->threadNN : (isInfo() ? ((PROC_DATA*)(proc->data))->threadNN : 0);
}
void* LOG_NODE::getCallAddr()
{
  return isFlow() ? ((FLOW_DATA*)data)->call_site : 0;
}
int LOG_NODE::getCallLine()
{
  return isInfo() ? ((INFO_DATA*)data)->call_line : 0;
}

#ifndef NATIVE_TREE

void LOG_NODE::CollapseExpand(BOOL expand)
{
  expanded = expand;
  CalcLines();
}

void LOG_NODE::CalcLines()
{
  LOG_NODE* pNode = rootNode;
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
        if (pNode == rootNode)
          break;
        if (pNode->nextSibling)
        {
          pNode = pNode->nextSibling;
          break;
        }
      }
    }
  } while (rootNode != pNode);
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

#endif

