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
  void ResolveAsync(LOG_NODE* pNode = NULL);
  void Resolve(LOG_NODE* pSelectedNode, bool loop);

private:
  void _Resolve(LOG_NODE* pSelectedNode, bool bNested, bool loop);
  bool connectToServer(char *szServerName, WORD portNum);
  DWORD readUrl2(APP_NODE* appNode);
  void CloseSocket();
  SOCKET conn;
  LOG_NODE* m_pNode;
  HANDLE m_hParseEvent;
  HANDLE m_hResolveEvent;
  ADDR_INFO *m_pFakeInfo;
};
