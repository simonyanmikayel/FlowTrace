#include "stdafx.h"
#include "Addr2LineThread.h"
#include "Archive.h"
#include "Helpers.h"

extern HWND       hwndMain;

Addr2LineThread::Addr2LineThread()
{
  conn = INVALID_SOCKET;
  m_pNode = NULL;
  m_hParseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  m_hResolveEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

void Addr2LineThread::CloseSocket()
{
  SOCKET s0 = conn;
  conn = INVALID_SOCKET;
  if (s0 != INVALID_SOCKET)
    closesocket(s0);
}

void Addr2LineThread::Terminate()
{
  CloseSocket();
  SetEvent(m_hParseEvent);
  CloseHandle(m_hParseEvent);
  CloseHandle(m_hResolveEvent);
}

void Addr2LineThread::Resolve(LOG_NODE* pNode)
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
      APP_NODE* appNode = (APP_NODE*)rootNode->lastChild;
      while (appNode && IsWorking())
      {
        APP_DATA* appData = appNode->getData();
        if (appData->cb_addr_info == INFINITE)
        {
          appData->cb_addr_info = readUrl2(appData);
          CloseSocket();
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
        if (!pNode)
          continue;
        if (pNode->isTrace())
          pNode = pNode->getSyncNode();
        if (pNode && pNode->parent == NULL)
          pNode = pNode->getPeer();
        if (!pNode || !pNode->isFlow())
          continue;
        ADDR_INFO *p_addr_info = pNode->p_addr_info;
        if (p_addr_info)
          continue;
        APP_NODE* appNode = pNode->getApp();
        if (!appNode)
          continue;
        APP_DATA* appData = appNode->getData();
        if (!appData)
          continue;
        if (appData->cb_addr_info == INFINITE || appData->cb_addr_info == 0 || appData->p_addr_info == NULL)
          continue;

        while (pNode && pNode->isFlow() && IsWorking())
        {
          if (pNode->p_addr_info == 0)
          {
            FLOW_DATA* flowData = ((FLOW_NODE*)pNode)->getData();

            DWORD addr = (DWORD)flowData->call_site;
            DWORD nearest_pc = 0;
            ADDR_INFO *p_addr_info = appData->p_addr_info;
            pNode->p_addr_info = p_addr_info; //initial bad value
            char* fn = flowData->fnName();
            while (p_addr_info && IsWorking())
            {
              if (addr >= p_addr_info->addr && p_addr_info->addr >= nearest_pc)
              {
                nearest_pc = p_addr_info->addr;
                pNode->p_addr_info = p_addr_info;
              }

              p_addr_info = p_addr_info->pPrev;
            }
          }
          pNode = pNode->parent;
        }
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


DWORD Addr2LineThread::readUrl2(APP_DATA* appData)
{
  const int bufSize = 1024;
  char readBuffer[bufSize + 1], sendBuffer[256];
  char* leinEnd;
  long thisReadSize, readSize;
  DWORD addr, maxAddr = 0;
  DWORD line;
  ADDR_INFO* last_info = 0;
  char src[bufSize + 1];
  int total = 0;

  if (appData->cb_app_path >= MAX_PATH || appData->cb_app_path == 0)
    return 0;

  HRESULT hr = S_OK;
  char appPath[MAX_PATH + 1];
  char encodedAppPath[2 * MAX_PATH + 1];
  DWORD cchEscaped;
  memcpy(appPath, appData->appPath(), appData->cb_app_path);
  appPath[appData->cb_app_path] = 0;
  cchEscaped = 2 * MAX_PATH;
  if (S_OK != (hr = UrlEscapeA(appPath, encodedAppPath, &cchEscaped, 0)))
    return 0;

  if (!connectToServer(appData->ip_address, 80))
    return 0;

  char *szReq = "GET /cgi-bin/examineAddressesForFlowTrace.sh?app_name=%s&addresses=0 HTTP/1.1\r\nHost: %s\r\n\r\n";
  sprintf(sendBuffer, szReq, encodedAppPath, appData->ip_address);
  send(conn, sendBuffer, strlen(sendBuffer), 0);
  //stdlog("sendBuffer\n%s:\n:\n", sendBuffer);

  // Receive until the peer closes the connection
  readSize = 0;
  while (IsWorking())
  {
    thisReadSize = recv(conn, readBuffer + readSize, bufSize - readSize, 0);

    if (thisReadSize <= 0)
      break;
    //char* c = "12\n45\n7";
    //strcpy(readBuffer, c);
    //thisReadSize = strlen(c);

    readSize += thisReadSize;
    readBuffer[readSize] = 0;
    while(leinEnd = strchr(readBuffer, '\n'))
    {
      int cb = leinEnd - readBuffer;
      readBuffer[cb] = 0;
      cb++;
      //stdlog("lineBuffer: %s\n", readBuffer);
      if (3 == sscanf(readBuffer, TEXT("addr: %X  \t line: %d \t %s"), &addr, &line, src))
      {
        int cbSrc = strlen(src);
        ADDR_INFO* p_addr_info = NULL;
        if (last_info && 0 == strcmp(last_info->src, src))
        {
          if (p_addr_info = (ADDR_INFO*)gArchive.reservDataBuf(sizeof(ADDR_INFO)))
            p_addr_info->src = last_info->src;
        }
        else
        {
          if (p_addr_info = (ADDR_INFO*)gArchive.reservDataBuf(sizeof(ADDR_INFO) + cbSrc + 1))
          {
            p_addr_info->src = ((char*)p_addr_info) + sizeof(ADDR_INFO);
            memcpy(p_addr_info->src, src, cbSrc + 1);
          }
        }
        if (p_addr_info == NULL)
          break;

        maxAddr = max(maxAddr, addr);
        p_addr_info->addr = addr;
        p_addr_info->line = line;
        p_addr_info->pPrev = last_info;
        last_info = p_addr_info;
        appData->p_addr_info = p_addr_info;
        total++;
        //stdlog(TEXT("addr: %X  \t line: %d \t %s\n"), addr, line, src);
      }

      memmove(readBuffer, readBuffer + cb, readSize - cb);
      readSize -= cb;
      readBuffer[readSize] = 0;
    }
    if (bufSize - readSize < 10)
    {
      readSize = 0;
    }
     
    //stdlog("readBuffer\n%s:\n:\n", readBuffer);
  }

  // add a fake info for unknown
  if (appData->p_addr_info)
  {
    ADDR_INFO *p = (ADDR_INFO*)gArchive.reservDataBuf(sizeof(ADDR_INFO));
    if (p)
    {
      p->addr = maxAddr + 1000;
      p->line = INFINITE;
      p->src = "?";
      p->pPrev = NULL;
      p->pPrev = appData->p_addr_info;
      appData->p_addr_info = p;
      total++;
    }
  }

  return total;
}

bool Addr2LineThread::connectToServer(char *szServerName, WORD portNum)
{
  CloseSocket();

  if ((conn = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
  {
    //printf("Could not create socket : %d", WSAGetLastError());
    return false;
  }
 
  struct sockaddr_in server;
  server.sin_addr.s_addr = inet_addr(szServerName);
  server.sin_family = AF_INET;
  server.sin_port = htons(portNum);

  //Connect to remote server
  if (connect(conn, (struct sockaddr *)&server, sizeof(server)) < 0)
  {
    //puts("connect error");
    CloseSocket();
    return false;
  }

  return true;
}
