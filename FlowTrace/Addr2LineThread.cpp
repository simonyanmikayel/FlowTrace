#include "stdafx.h"
#include "Addr2LineThread.h"

Addr2LineThread::Addr2LineThread()
{
  m_hWorkEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  server = "10.1.3.45";
}

void Addr2LineThread::Terminate()
{
  SOCKET s0 = conn;
  conn = INVALID_SOCKET;
  if (s0 != INVALID_SOCKET)
    closesocket(s0);
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
        bool OK = false;
        HRESULT hr = S_OK;
        char appPath[MAX_PATH + 1];
        char encodedAppPath[2*MAX_PATH + 1];
        DWORD cchEscaped;
        if (appData->cb_app_path <= MAX_PATH && appData->cb_app_path > 0)
        {
          memcpy(appPath, appData->appPath(), appData->cb_app_path);
          appPath[appData->cb_app_path] = 0;
          cchEscaped = 2 * MAX_PATH;
          if (S_OK == (hr = UrlEscapeA(appPath, encodedAppPath, &cchEscaped, 0)))
          {
            //"%2Fbe%2Fbanksys%2Femv%2FCTAPapp"
            readUrl2(encodedAppPath);
          }
        }
        if(!OK)
        {
          appData->cb_addr_info = 0;
        }
        
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
    closesocket(conn);
    conn = INVALID_SOCKET;
    return false;
  }

  server_sockaddr.sin_addr.s_addr = *((unsigned long*)hp->h_addr);
  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(portNum);
  if (connect(conn, (struct sockaddr*)&server_sockaddr, sizeof(server_sockaddr)))
  {
    closesocket(conn);
    conn = INVALID_SOCKET;
    return false;
  }

  return true;
}

char *Addr2LineThread::readUrl2(char *szfilepath)
{
  const int bufSize = 1024;
  char readBuffer[bufSize + 1], sendBuffer[256];
  char* leinEnd;
  long thisReadSize, readSize;
  DWORD addr;
  DWORD line;
  char src[bufSize + 1];
  int total = 0;

  if (!connectToServer((char*)server.c_str(), 80))
    return NULL;

  char *szReq = "GET /cgi-bin/examineAddressesForFlowTrace.sh?app_name=%s&addresses=0 HTTP/1.1\r\nHost: %s\r\n\r\n";
  sprintf(sendBuffer, szReq, szfilepath, "10.1.3.45");
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
  return NULL;
}
