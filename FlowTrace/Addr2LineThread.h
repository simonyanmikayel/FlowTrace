#pragma once
#include "WorkerThread.h"
#include "LogData.h"

class Addr2LineThread :
  public WorkerThread
{
public:
  Addr2LineThread();
  virtual void Terminate();
  void Work(LPVOID pWorkParam);
  void Resolve(LOG_NODE* pNode = NULL, bool bNested = false);

private:
  bool connectToServer(char *szServerName, WORD portNum);
  DWORD readUrl2(APP_DATA* appData);
  void CloseSocket();
  SOCKET conn;
  LOG_NODE* m_pNode;
  bool m_bNested;
  HANDLE m_hParseEvent;
  HANDLE m_hResolveEvent;
  ADDR_INFO *m_pFakeInfo;
};
