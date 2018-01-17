#include "stdafx.h"
#include "Addr2LineThread.h"
#include "Archive.h"

Addr2LineThread::Addr2LineThread()
{
  conn = INVALID_SOCKET;
  last_info = NULL;
  m_hWorkEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
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
  SetEvent(m_hWorkEvent);
  CloseHandle(m_hWorkEvent);
}

void Addr2LineThread::Resolve()
{
  SetEvent(m_hWorkEvent);
}

void Addr2LineThread::Work(LPVOID pWorkParam)
{
  while (WAIT_OBJECT_0 == WaitForSingleObject(m_hWorkEvent, INFINITE) && IsWorking())
  {
    APP_NODE* appNode = (APP_NODE*)rootNode->lastChild;
    while (appNode)
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
}

bool Addr2LineThread::connectToServer(char *szServerName, WORD portNum)
{
  struct hostent *hp;
  unsigned int addr;
  struct sockaddr_in server_sockaddr;

  CloseSocket();
  conn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (conn == INVALID_SOCKET)
    return false;

  if (inet_addr(szServerName) == INADDR_NONE)
  {
    hp = gethostbyname(szServerName);
  }
  else
  {
    addr = inet_addr(szServerName);
    hp = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET);
  }

  if (hp == NULL)
  {
    return false;
  }

  server_sockaddr.sin_addr.s_addr = *((unsigned long*)hp->h_addr);
  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(portNum);
  if (connect(conn, (struct sockaddr*)&server_sockaddr, sizeof(server_sockaddr)))
  {
    return false;
  }

  return true;
}

DWORD Addr2LineThread::readUrl2(APP_DATA* appData)
{
  const int bufSize = 1024;
  char readBuffer[bufSize + 1], sendBuffer[256];
  char* leinEnd;
  long thisReadSize, readSize;
  DWORD addr;
  DWORD line;
  char src[bufSize + 1];
  int total = 0;

  if (appData->cb_app_path <= MAX_PATH && appData->cb_app_path > 0)
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
  stdlog("sendBuffer\n%s:\n:\n", sendBuffer);

  // Receive until the peer closes the connection
  readSize = 0;
  while (1)
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

        p_addr_info->addr = addr;
        p_addr_info->line = line;
        p_addr_info->pPrev = last_info;
        last_info = p_addr_info;
        total++;
        stdlog(TEXT("addr: %X  \t line: %d \t %s\n"), addr, line, src);
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
  return total;
}
